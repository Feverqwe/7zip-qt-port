#include "CPP/7zip/UI/Agent/Agent.h"
#include "CPP/7zip/UI/FileManager/FileFolderPluginOpen.h"
#include "CPP/7zip/UI/FileManager/ProgressDialog2.h"
#include "CPP/7zip/UI/FileManager/UpdateCallback100.h"
#include "archivecodeccatalog.h"
#include "qtarchivefileresource.h"
#include "qtarchivefoldermanager.h"
#include <QApplication>
#include <QFile>
#include <QMessageBox>
#include <QPushButton>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QtTest>

namespace {
class CCommentCallback final : public IFolderArchiveUpdateCallback, public CMyUnknownImp {
    Z7_COM_UNKNOWN_IMP_1(IFolderArchiveUpdateCallback)
  public:
    bool Cancel = false;
    Z7_COM7F_IMF(SetTotal(UInt64)) override { return Cancel ? E_ABORT : S_OK; }
    Z7_COM7F_IMF(SetCompleted(const UInt64 *)) override { return Cancel ? E_ABORT : S_OK; }
    Z7_COM7F_IMF(CompressOperation(const wchar_t *)) override { return S_OK; }
    Z7_COM7F_IMF(DeleteOperation(const wchar_t *)) override { return E_FAIL; }
    Z7_COM7F_IMF(OperationResult(Int32)) override { return S_OK; }
    Z7_COM7F_IMF(UpdateErrorMessage(const wchar_t *)) override { return E_FAIL; }
    Z7_COM7F_IMF(SetNumFiles(UInt64)) override { return S_OK; }
};
class CCommentWorker final : public CProgressThreadVirt {
    CMyComPtr<IFolderOperations> operations_;
    UInt32 index_;
    QString value_;
    HRESULT ProcessVirt() override {
        if (SourceProgress) {
            auto *progress = new CUpdateCallback100Imp;
            CMyComPtr<IFolderArchiveUpdateCallback> owner = progress;
            progress->ProgressDialog = this;
            progress->Init();
            NWindows::NCOM::CPropVariant value(value_.toStdWString().c_str());
            return operations_->SetProperty(index_, kpidComment, &value, owner);
        }
        auto *progress = new CCommentCallback;
        CMyComPtr<IFolderArchiveUpdateCallback> owner = progress;
        progress->Cancel = Cancel;
        NWindows::NCOM::CPropVariant value(value_.toStdWString().c_str());
        return operations_->SetProperty(index_, kpidComment, &value, owner);
    }

  public:
    bool Cancel = false;
    bool SourceProgress = false;
    CCommentWorker(IFolderOperations *operations, UInt32 index, const QString &value)
        : operations_(operations), index_(index), value_(value) {}
};
QByteArray read(const QString &path) {
    QFile file(path);
    return file.open(QIODevice::ReadOnly) ? file.readAll() : QByteArray();
}
} // namespace

class NativeCommentTests final : public QObject {
    Q_OBJECT
    QString engine_, module_;
    quint32 version_ = 0;
    bool run(const QStringList &arguments, const QString &directory = {}) {
        QProcess process;
        process.setWorkingDirectory(directory);
        process.start(engine_, arguments);
        return process.waitForFinished(10000) && process.exitCode() == 0;
    }
  private slots:
    void initTestCase() {
#ifndef SEVENZIP_CODEC_MODULE
        QSKIP("Official module unavailable; source comment consumer unverified");
#else
        engine_ = QStandardPaths::findExecutable("7z");
        QVERIFY(!engine_.isEmpty());
        module_ = QString::fromUtf8(SEVENZIP_CODEC_MODULE);
        ArchiveBackend backend(nullptr, engine_);
        QSignalSpy ready(&backend, &ArchiveBackend::codecCatalogReady);
        QVERIFY(backend.discoverCodecCatalog(module_));
        QTRY_COMPARE(ready.size(), 1);
        QVERIFY2(ready[0][0].toBool(), qPrintable(ready[0][1].toString()));
        version_ = backend.codecCatalog()->version();
#endif
    }
    void sourceSetPropertyPublishesAndReopens_data() {
        QTest::addColumn<bool>("encrypted");
        QTest::addColumn<QString>("outcome");
        for (bool encrypted : {false, true})
            for (const QString &outcome : {"success", "failure", "cancel"})
                QTest::newRow(qPrintable(QString("%1-%2").arg(encrypted).arg(outcome)))
                    << encrypted << outcome;
    }
    void nativeWriterResumesOrCancelsWhilePaused_data() {
        QTest::addColumn<bool>("cancel");
        QTest::newRow("resume") << false;
        QTest::newRow("cancel-paused") << true;
    }
    void nativeWriterResumesOrCancelsWhilePaused() {
        QFETCH(bool, cancel);
        QTemporaryDir fixture;
        QVERIFY(fixture.isValid());
        const QString archive = fixture.filePath("archive.zip");
        QFile source(fixture.filePath(QStringLiteral("файл с пробелами.txt")));
        QVERIFY(source.open(QIODevice::WriteOnly));
        QCOMPARE(source.write("original"), qint64(8));
        source.close();
        QVERIFY(run({"a", "-tzip", "--", archive, QFileInfo(source).fileName()}, fixture.path()));
        const QByteArray original = read(archive);
        ArchiveBackend backend(nullptr, engine_);
        QCOMPARE(backend.loadCodecCatalog(module_, version_), S_OK);
        CMyComPtr<IFolderManager> manager = new QtArchiveFolderManager(backend);
        CFfpOpen opening(manager);
        QCOMPARE(opening.OpenFileFolderPlugin(nullptr, us2fs(archive.toStdWString().c_str()),
                                              UString(L"zip"), nullptr),
                 S_OK);
        auto *folder = dynamic_cast<CAgentFolder *>(opening.Folder.Interface());
        QVERIFY(folder);
        QCOMPARE(folder->LoadItems(), S_OK);
        auto *resource = dynamic_cast<QtArchiveFileResource *>(folder->GetEngineResource());
        QVERIFY(resource);
        CMyComPtr<IFolderOperations> operations;
        QCOMPARE(opening.Folder.QueryInterface(IID_IFolderOperations, &operations), S_OK);
        CCommentWorker worker(operations, 0, "paused native comment");
        worker.SourceProgress = true;
        worker.DisableUserQuestions = true;
        // Start paused so the original ZIP callback waits deterministically,
        // even for this tiny archive. No large fixture or timing race is needed.
        worker.Sync.Set_Paused(true);
        auto transport = resource->CreateProgressTransport(worker, true);
        bool observed = false, resumed = false, unchanged = false;
        QTimer responder;
        connect(&responder, &QTimer::timeout, this, [&] {
            if (auto *box = qobject_cast<QMessageBox *>(QApplication::activeModalWidget())) {
                if (box->button(QMessageBox::Yes))
                    box->button(QMessageBox::Yes)->click();
                else
                    box->accept();
                return;
            }
            if (QApplication::activeModalWidget() != &worker || observed)
                return;
            observed = true;
            unchanged = read(archive) == original && !backend.busy() && worker.Sync.Get_Paused();
            if (cancel)
                QTimer::singleShot(0, &worker,
                                   [&] { worker.findChild<QPushButton *>("IDCANCEL")->click(); });
            else {
                worker.findChild<QPushButton *>("IDB_PAUSE")->click();
                resumed = !worker.Sync.Get_Paused();
                if (!resumed)
                    worker.Sync.Set_Paused(
                        false); // Rescue a failed assertion without a hung worker.
            }
        });
        responder.start(2);
        QCOMPARE(worker.Create(L"Native comment pause", nullptr), S_OK);
        QVERIFY(observed);
        QVERIFY(unchanged);
        QCOMPARE(worker.Result, cancel ? E_ABORT : S_OK);
        if (cancel)
            QCOMPARE(read(archive), original);
        else {
            QVERIFY(resumed);
            NWindows::NCOM::CPropVariant actual;
            QCOMPARE(folder->LoadItems(), S_OK);
            QCOMPARE(folder->GetProperty(0, kpidComment, &actual), S_OK);
            QCOMPARE(actual.vt, VARTYPE(VT_BSTR));
            QCOMPARE(QString::fromWCharArray(actual.bstrVal),
                     QStringLiteral("paused native comment"));
        }
        QCOMPARE(read(source.fileName()), QByteArray("original"));
    }
    void sourceSetPropertyPublishesAndReopens() {
        QFETCH(bool, encrypted);
        QFETCH(QString, outcome);
        QTemporaryDir fixture;
        QVERIFY(fixture.isValid());
        const QString name = QStringLiteral("имя 空 with spaces.bin");
        const QByteArray payload("binary\0payload\xff", 15);
        QFile source(fixture.filePath(name));
        QVERIFY(source.open(QIODevice::WriteOnly));
        QCOMPARE(source.write(payload), qint64(payload.size()));
        source.close();
        const QString archive = fixture.filePath("archive.unknown");
        QStringList args{"a", "-tzip"};
        if (encrypted)
            args << "-plocal fixture password" << "-mem=AES256";
        args << "--" << archive << name;
        QVERIFY(run(args, fixture.path()));
        const QByteArray original = read(archive);
        ArchiveBackend backend(nullptr, engine_);
        QCOMPARE(backend.loadCodecCatalog(module_, version_), S_OK);
        CMyComPtr<IFolderManager> manager = new QtArchiveFolderManager(backend);
        CFfpOpen opening(manager);
        QCOMPARE(opening.OpenFileFolderPlugin(nullptr, us2fs(archive.toStdWString().c_str()),
                                              UString(L"zip"), nullptr),
                 S_OK);
        auto *folder = dynamic_cast<CAgentFolder *>(opening.Folder.Interface());
        QVERIFY(folder);
        auto *resource = dynamic_cast<QtArchiveFileResource *>(folder->GetEngineResource());
        QVERIFY(resource && resource->HasCommentService());
        QCOMPARE(folder->LoadItems(), S_OK);
        CMyComPtr<IFolderOperations> operations;
        QCOMPARE(opening.Folder.QueryInterface(IID_IFolderOperations, &operations), S_OK);
        QString comment = outcome == "failure" ? QString(65536, 'x')
                                               : QStringLiteral("Комментарий 空 with spaces");
        CCommentWorker worker(operations, 0, comment);
        worker.Cancel = outcome == "cancel";
        worker.DisableUserQuestions = true;
        auto transport = resource->CreateProgressTransport(worker);
        QCOMPARE(worker.Create(L"Source ZIP comment", nullptr), S_OK);
        QCOMPARE(worker.Result, outcome == "success"  ? S_OK
                                : outcome == "cancel" ? E_ABORT
                                                      : E_INVALIDARG);
        QCOMPARE(read(source.fileName()), payload);
        QVERIFY(QDir(fixture.path())
                    .entryList({"*.tmp", ".sevenzip-*"}, QDir::Files | QDir::Hidden)
                    .isEmpty());
        if (outcome != "success") {
            QCOMPARE(read(archive), original);
            return;
        }
        QVERIFY(resource->HasCommentService());
        QCOMPARE(folder->LoadItems(), S_OK);
        NWindows::NCOM::CPropVariant actual;
        QCOMPARE(folder->GetProperty(0, kpidComment, &actual), S_OK);
        QCOMPARE(actual.vt, VARTYPE(VT_BSTR));
        QCOMPARE(QString::fromWCharArray(actual.bstrVal), comment);
        const QString extracted = fixture.filePath("extracted");
        QVERIFY(run({"x", "-tzip", "-plocal fixture password", "-o" + extracted, "--", archive}));
        QCOMPARE(read(extracted + "/" + name), payload);
        CCommentWorker clear(operations, 0, QString());
        clear.DisableUserQuestions = true;
        auto clearTransport = resource->CreateProgressTransport(clear);
        QCOMPARE(clear.Create(L"Clear ZIP comment", nullptr), S_OK);
        QCOMPARE(clear.Result, S_OK);
        QCOMPARE(folder->LoadItems(), S_OK);
        QCOMPARE(folder->GetProperty(0, kpidComment, &actual), S_OK);
        QVERIFY(actual.vt == VT_EMPTY ||
                (actual.vt == VT_BSTR && QString::fromWCharArray(actual.bstrVal).isEmpty()));
    }
};
QTEST_MAIN(NativeCommentTests)
#include "native_comment_tests.moc"
