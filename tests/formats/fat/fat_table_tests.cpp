#include "Windows/PropVariant.h"
#include "formats/fat/support/fat_test_support.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QLibrary>
#include <QProcess>
#include <QtEndian>
#include <QtTest>
#ifdef SEVENZIP_FAT_TABLE_PROMPT
#include "archive/resources/qtfilesystemreadprompt.h"
#include "localization/qtlanguageservices.h"
#include "ui/support/layout_inspection.h"
#include <QLabel>
#include <QLayout>
#include <QMessageBox>
#include <QPushButton>
#include <QTimer>
#endif

using namespace FatTestSupport;

class TableCallback final : public IArchiveOpenCallback, public CMyUnknownImp {
    Z7_COM_UNKNOWN_IMP_1(IArchiveOpenCallback)
  public:
    Input *source = nullptr;
    HRESULT failure = E_ABORT;
    Z7_COM7F_IMF(SetTotal(const UInt64 *, const UInt64 *)) override { return S_OK; }
    Z7_COM7F_IMF(SetCompleted(const UInt64 *, const UInt64 *)) override {
        return source->reachedFault ? failure : S_OK;
    }
};

class FatTableTests final : public QObject {
    Q_OBJECT
    QLibrary module_{QString::fromUtf8(SEVENZIP_FAT_MODULE)};
    CMyComPtr<IInArchive> archive_;
    QByteArray bytes_;
    QJsonObject fixture_;
    QString limitations_;
    void fixture(const QString &kind) {
        QProcess process;
        process.start(QString::fromUtf8(SEVENZIP_FAT_PYTHON),
                      {QString::fromUtf8(SEVENZIP_FAT_FIXTURE), "--table=" + kind, "--bundle"});
        QVERIFY(process.waitForFinished(10000));
        QCOMPARE(process.exitCode(), 0);
        fixture_ = QJsonDocument::fromJson(process.readAllStandardOutput()).object();
        QByteArray packed(4, '\0');
        qToBigEndian(quint32(fixture_["size"].toInt()), packed.data());
        packed += QByteArray::fromBase64(fixture_["image"].toString().toLatin1());
        bytes_ = qUncompress(packed);
        QVERIFY(!bytes_.isEmpty());
    }
    UInt32 index(const QString &name) {
        UInt32 count = 0;
        if (archive_->GetNumberOfItems(&count) != S_OK)
            return UINT32_MAX;
        for (UInt32 i = 0; i < count; ++i) {
            NWindows::NCOM::CPropVariant path;
            if (archive_->GetProperty(i, kpidPath, &path) == S_OK && path.vt == VT_BSTR &&
                QString::fromWCharArray(path.bstrVal).replace('\\', '/') == name)
                return i;
        }
        return UINT32_MAX;
    }
    void checkFiles() {
        QVector<UInt32> indices;
        QVector<Int32> results;
        QByteArray bytes;
        const QString main = fixture_["name"].toString();
        const auto files = fixture_["files"].toObject();
        for (auto it = files.begin(); it != files.end(); ++it) {
            const UInt32 id = index(it.key());
            QVERIFY2(id != UINT32_MAX, qPrintable(it.key()));
            const auto data = QByteArray::fromBase64(it.value().toString().toLatin1());
            const bool incomplete =
                it.key() == main && data.size() < fixture_["declared_file_size"].toInteger();
            NWindows::NCOM::CPropVariant error;
            QCOMPARE(archive_->GetProperty(id, kpidError, &error), S_OK);
            QCOMPARE(error.vt, VARTYPE(incomplete ? VT_BSTR : VT_EMPTY));
            indices.append(id);
            results.append(incomplete ? NArchive::NExtract::NOperationResult::kDataError
                                      : NArchive::NExtract::NOperationResult::kOK);
            bytes += data;
        }
        CMyComPtr2<IArchiveExtractCallback, Extract> callback;
        callback.Create_if_Empty();
        QCOMPARE(archive_->Extract(indices.constData(), UInt32(indices.size()), 0, callback), S_OK);
        QCOMPARE(callback->output->bytes, bytes);
        QCOMPARE(callback->results, results);
        callback->results.clear();
        QCOMPARE(archive_->Extract(indices.constData(), UInt32(indices.size()), 1, callback), S_OK);
        QCOMPARE(callback->results, results);
        if (fixture_["directory_limited"].toBool()) {
            const UInt32 folder = index("FOLDER");
            QVERIFY(folder != UINT32_MAX);
            NWindows::NCOM::CPropVariant error;
            QCOMPARE(archive_->GetProperty(folder, kpidError, &error), S_OK);
            QCOMPARE(error.vt, VARTYPE(VT_BSTR));
        }
    }
    void checkLimitations() {
        limitations_.clear();
        UInt32 count = 0;
        QCOMPARE(archive_->GetNumberOfArchiveProperties(&count), S_OK);
        bool found = false;
        for (UInt32 i = 0; i < count; ++i) {
            CMyComBSTR name;
            PROPID id;
            VARTYPE type;
            QCOMPARE(archive_->GetArchivePropertyInfo(i, &name, &id, &type), S_OK);
            if (!name || QString::fromWCharArray(name) != "Read-only limitations")
                continue;
            found = true;
            NWindows::NCOM::CPropVariant value;
            QCOMPARE(archive_->GetArchiveProperty(id, &value), S_OK);
            QCOMPARE(value.vt, VARTYPE(fixture_["table_limited"].toBool() ? VT_BSTR : VT_EMPTY));
            if (value.vt == VT_BSTR) {
                limitations_ = QString::fromWCharArray(value.bstrVal);
                QVERIFY(limitations_.contains("FAT table entries"));
            }
        }
        QVERIFY(found);
    }
    void fault(Input *source, bool seek, HRESULT failure) {
        source->failAt = UInt64(fixture_["table_fault_offset"].toInteger());
        source->failEnd = source->failAt + 512;
        source->partialBytes = UInt32(fixture_["table_partial_bytes"].toInt());
        source->failSeek = seek;
        source->failure = failure;
    }
  private slots:
    void initTestCase() {
        QVERIFY(module_.load());
        const auto property =
            reinterpret_cast<Func_GetHandlerProperty2>(module_.resolve("GetHandlerProperty2"));
        const auto create = reinterpret_cast<Func_CreateObject>(module_.resolve("CreateObject"));
        QVERIFY(property && create);
        NWindows::NCOM::CPropVariant clsid;
        QCOMPARE(property(0, NArchive::NHandlerPropID::kClassID, &clsid), S_OK);
        GUID guid;
        memcpy(&guid, clsid.bstrVal, sizeof(guid));
        QCOMPARE(create(&guid, &IID_IInArchive, reinterpret_cast<void **>(&archive_)), S_OK);
    }
    void localizedTableFaults_data() {
        QTest::addColumn<QString>("kind");
        QTest::addColumn<bool>("seek");
        QTest::addColumn<int>("failure");
        for (int bits : {12, 16, 32}) {
            QStringList modes{"first", "middle", "partial", "entry-prefix", "padding", "directory"};
            if (bits == 32)
                modes.append("large");
            for (const QString &mode : modes)
                for (int failure : {int(S_OK), int(E_FAIL)})
                    QTest::newRow(qPrintable(mode + '-' + QString::number(bits) + '-' +
                                             QString::number(failure)))
                        << mode + '-' + QString::number(bits) << false << failure;
            QTest::newRow(qPrintable("seek-first-" + QString::number(bits)))
                << "first-" + QString::number(bits) << true << int(E_FAIL);
        }
    }
    void localizedTableFaults() {
        QFETCH(QString, kind);
        QFETCH(bool, seek);
        QFETCH(int, failure);
        fixture(kind);
        CMyComPtr<IInStream> clean = new Input(bytes_);
        QCOMPARE(archive_->Open(clean, nullptr, nullptr), S_OK);
        NWindows::NCOM::CPropVariant before;
        QCOMPARE(archive_->GetArchiveProperty(kpidFreeSpace, &before), S_OK);
        QCOMPARE(before.vt, VARTYPE(VT_UI8));
        CMyComPtr2<IInStream, Input> source;
        source.SetFromCls(new Input(bytes_));
        fault(source.ClsPtr(), seek, HRESULT(failure));
        QCOMPARE(archive_->Open(source, nullptr, nullptr), S_OK);
        QVERIFY(source->reachedFault);
        checkFiles();
        checkLimitations();
        NWindows::NCOM::CPropVariant free;
        QCOMPARE(archive_->GetArchiveProperty(kpidFreeSpace, &free), S_OK);
        QCOMPARE(free.vt, VARTYPE(fixture_["table_limited"].toBool() ? VT_EMPTY : VT_UI8));
        if (free.vt == VT_UI8)
            QCOMPARE(free.uhVal.QuadPart, before.uhVal.QuadPart);
        source->failAt = UINT64_MAX;
        QCOMPARE(archive_->Open(source, nullptr, nullptr), S_OK);
        QCOMPARE(archive_->GetArchiveProperty(kpidFreeSpace, &free), S_OK);
        QCOMPARE(free.vt, VARTYPE(VT_UI8));
        QCOMPARE(free.uhVal.QuadPart, before.uhVal.QuadPart);
    }
    void cancellationAndCallbackFailureDoNotPublishPartialArchives() {
        for (int bits : {12, 16, 32}) {
            fixture("first-" + QString::number(bits));
            for (HRESULT failure : {E_FAIL, E_ABORT, E_OUTOFMEMORY}) {
                for (bool seek : {false, true}) {
                    CMyComPtr2<IInStream, Input> source;
                    source.SetFromCls(new Input(bytes_));
                    fault(source.ClsPtr(), seek, failure);
                    CMyComPtr2<IArchiveOpenCallback, TableCallback> callback;
                    callback.Create_if_Empty();
                    callback->source = source.ClsPtr();
                    callback->failure = failure;
                    QCOMPARE(archive_->Open(source, nullptr, callback), failure);
                    UInt32 count = 99;
                    QCOMPARE(archive_->GetNumberOfItems(&count), S_OK);
                    QCOMPARE(count, UInt32(0));
                    source->failAt = UINT64_MAX;
                    QCOMPARE(archive_->Open(source, nullptr, nullptr), S_OK);
                }
            }
        }
    }
#ifdef SEVENZIP_FAT_TABLE_PROMPT
    void limitedTableConsent_data() {
        QTest::addColumn<QString>("language");
        QTest::addColumn<int>("bits");
        QTest::addColumn<bool>("accept");
        for (const QString &language : {"en", "ru"})
            for (int bits : {12, 32})
                for (bool accept : {false, true})
                    QTest::newRow(qPrintable(language + '-' + QString::number(bits) + '-' +
                                             QString::number(accept)))
                        << language << bits << accept;
    }
    void limitedTableConsent() {
        QFETCH(QString, language);
        QFETCH(int, bits);
        QFETCH(bool, accept);
        QtLoadTranslation(language == "en" ? "-" : "ru");
        fixture((bits == 12 ? "middle-" : "first-") + QString::number(bits));
        CMyComPtr2<IInStream, Input> source;
        source.SetFromCls(new Input(bytes_));
        fault(source.ClsPtr(), false, E_FAIL);
        QCOMPARE(archive_->Open(source, nullptr, nullptr), S_OK);
        checkLimitations();
        bool handled = false, geometry = true;
        QTimer automation;
        connect(&automation, &QTimer::timeout, [&] {
            if (handled)
                return;
            for (auto *widget : QApplication::topLevelWidgets()) {
                auto *message = qobject_cast<QMessageBox *>(widget);
                if (!message || !message->isVisible() ||
                    message->objectName() != "filesystemReadLimitationsWarning")
                    continue;
                message->layout()->activate();
                for (auto *label : message->findChildren<QLabel *>())
                    if (label->isVisible())
                        geometry &= UiInspection::fullyVisible(*label, *message) &&
                                    UiInspection::labelFits(*label);
                message->setProperty("uiInspection_language", language);
                geometry &= UiInspection::writeReport(*message, "fat-table-" + language + '-' +
                                                                    QString::number(bits));
                handled = true;
                if (accept)
                    message->findChild<QPushButton *>("deviceOpenWithoutRecoveryButton")->click();
                else
                    QTest::keyClick(message, Qt::Key_Return);
            }
        });
        automation.start(10);
        QCOMPARE(ConfirmLimitedFilesystemRead(nullptr, limitations_), accept);
        automation.stop();
        QVERIFY(handled);
        QVERIFY(geometry);
        if (accept)
            checkFiles();
        QCOMPARE(archive_->Close(), S_OK);
    }
#endif
    void cleanupTestCase() { archive_.Release(); }
};
#ifdef SEVENZIP_FAT_TABLE_PROMPT
QTEST_MAIN(FatTableTests)
#else
QTEST_GUILESS_MAIN(FatTableTests)
#endif
#include "fat_table_tests.moc"
