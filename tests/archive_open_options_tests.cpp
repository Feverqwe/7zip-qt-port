#include "7zip/UI/FileManager/ProgressDialog2Res.h"
#include "CPP/7zip/UI/FileManager/ExtractCallback.h"
#include "CPP/7zip/UI/FileManager/ProgressDialog2.h"
#include "Common/StringConvert.h"
#include "archivebackend.h"
#include "archivecodeccatalog.h"
#include "archivenativeopen.h"
#include "qtshellarchiveservices.h"
#include "rar_memory_fixture.h"
#include "source_extract_fixture.h"
#include <QApplication>
#include <QMessageBox>
#include <QPushButton>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTimer>
#include <QtTest>

namespace {
class FinalMessageDismissal {
    QTimer timer;

  public:
    FinalMessageDismissal() {
        QObject::connect(&timer, &QTimer::timeout, [] {
            for (auto *widget : QApplication::topLevelWidgets()) {
                if (auto *box = qobject_cast<QMessageBox *>(widget); box && box->isVisible())
                    box->accept();
                if (widget->objectName() == "progressDialog" && widget->isVisible()) {
                    auto *close = widget->findChild<QPushButton *>("IDCANCEL");
                    if (close && close->property("languageID").toUInt() == IDS_CLOSE)
                        close->click();
                }
            }
        });
        timer.start(1);
    }
};
class CancelOpen final : public IOpenCallbackUI {
  public:
    HRESULT Open_CheckBreak() override { return E_ABORT; }
    HRESULT Open_SetTotal(const UInt64 *, const UInt64 *) override { return E_ABORT; }
    HRESULT Open_SetCompleted(const UInt64 *, const UInt64 *) override { return E_ABORT; }
    HRESULT Open_Finished() override { return E_ABORT; }
    HRESULT Open_CryptoGetTextPassword(BSTR *) override { return E_ABORT; }
};
} // namespace

class ArchiveOpenOptionsTests final : public QObject {
    Q_OBJECT
    QString module;
    UInt32 version = 0;
    void load(ArchiveBackend &backend) {
        QCOMPARE(backend.loadCodecCatalog(module, version), S_OK);
    }
    bool extract(ArchiveBackend &backend, const QString &archive, const QString &output,
                 const UString &type, bool excludeZip) {
        QtShellArchiveServices service(backend, nullptr, {}, {});
        CObjectVector<COpenType> types;
        if (!ParseOpenTypes(service.Codecs(), type, types))
            return false;
        CIntVector excluded;
        if (excludeZip)
            excluded.Add(backend.codecCatalog()->formatIndex(QStringLiteral("zip")));
        UStringVector paths, fullPaths;
        paths.Add(archive.toStdWString().c_str());
        fullPaths = paths;
        NWildcard::CCensorNode censor;
        censor.Add_Wildcard();
        CExtractOptions options;
        options.OutputDir = us2fs(UString(QDir::toNativeSeparators(output).toStdWString().c_str()));
        options.OutDirMode = NExtractOutDirMode::k_Direct;
        options.YesToAll = true;
        CMyComPtr<IFolderArchiveExtractCallback> callback = new CExtractCallbackImp;
        auto *ui = static_cast<CExtractCallbackImp *>(callback.Interface());
        ui->YesToAll = true;
        ui->Init();
        bool displayed = false;
        FinalMessageDismissal dismiss;
        const HRESULT result =
            service.ExtractGUI(service.Codecs(), types, excluded, paths, fullPaths, censor, options,
                               nullptr, false, displayed, *ui, type);
        return result == S_OK && ui->IsOK();
    }
  private slots:
    void initTestCase() {
#ifndef SEVENZIP_CODEC_MODULE
        QSKIP("Official codec module is unavailable");
#else
        QVERIFY(!QStandardPaths::findExecutable("7zz").isEmpty());
        module = QString::fromUtf8(SEVENZIP_CODEC_MODULE);
        ArchiveCodecCatalog probe(module, 0);
        QCOMPARE(probe.result(), S_OK);
        version = probe.version();
        QVERIFY(version);
#endif
    }
    void excludedZipIsNotOpened_data() {
        QTest::addColumn<bool>("excluded");
        QTest::newRow("allowed") << false;
        QTest::newRow("excluded") << true;
    }
    void excludedZipIsNotOpened() {
        QFETCH(bool, excluded);
        QTemporaryDir fixture;
        QVERIFY(fixture.isValid());
        const QByteArray bytes("archive payload\0", 16);
        const QString archive = SourceExtractFixture::makeArchive(fixture.path(), bytes);
        QVERIFY(!archive.isEmpty());
        const QByteArray before = SourceExtractFixture::readFile(archive);
        ArchiveBackend backend;
        load(backend);
        QSignalSpy started(&backend, &ArchiveBackend::engineStarted);
        const QString output = fixture.filePath(QStringLiteral("выход 空"));
        QCOMPARE(extract(backend, archive, output, UString(), excluded), !excluded);
        if (excluded) {
            QCOMPARE(started.size(), 0);
            QVERIFY(!QFileInfo::exists(QDir(output).filePath(SourceExtractFixture::kName)));
        } else
            QCOMPARE(
                SourceExtractFixture::readFile(QDir(output).filePath(SourceExtractFixture::kName)),
                bytes);
        QCOMPARE(SourceExtractFixture::readFile(archive), before);
    }
    void originalTypeChain() {
        QTemporaryDir fixture;
        QVERIFY(fixture.isValid());
        const QByteArray bytes("nested source types");
        QVERIFY(
            SourceExtractFixture::writeFile(fixture.filePath(SourceExtractFixture::kName), bytes));
        QProcess engine;
        engine.setWorkingDirectory(fixture.path());
        const QString name = QStringLiteral("цепочка 空 with spaces.zip");
        engine.start(QStandardPaths::findExecutable("7zz"),
                     {"a", "-tzip", "-v128b", name, SourceExtractFixture::kName});
        QVERIFY(engine.waitForFinished(10000));
        QCOMPARE(engine.exitCode(), 0);
        const QString archive = fixture.filePath(name + ".001");
        engine.start(QStandardPaths::findExecutable("7zz"), {"l", "-tzip.split", archive});
        QVERIFY(engine.waitForFinished(10000));
        QVERIFY2(engine.exitCode() == 0, engine.readAll().constData());
        QMap<QString, QByteArray> before;
        const auto volumes = QDir(fixture.path()).entryList({name + ".*"}, QDir::Files);
        QVERIFY(volumes.size() > 1);
        for (const auto &volume : volumes)
            before.insert(volume, SourceExtractFixture::readFile(fixture.filePath(volume)));
        ArchiveBackend backend;
        load(backend);
        const QString output = fixture.filePath("typed output 空");
        QVERIFY(extract(backend, archive, output, UString("zip.split"), false));
        QCOMPARE(SourceExtractFixture::readFile(QDir(output).filePath(SourceExtractFixture::kName)),
                 bytes);
        for (auto i = before.cbegin(); i != before.cend(); ++i)
            QCOMPARE(SourceExtractFixture::readFile(fixture.filePath(i.key())), i.value());
    }
    void originalOpenPropertiesAndCancellation() {
        QTemporaryDir fixture;
        QVERIFY(fixture.isValid());
        const QString archive = fixture.filePath(QStringLiteral("архив 空.rar"));
        const QByteArray bytes = RarMemoryFixture::archive(0);
        QVERIFY(SourceExtractFixture::writeFile(archive, bytes));
        ArchiveBackend backend;
        load(backend);
        CObjectVector<COpenType> types;
        CIntVector excluded;
        CObjectVector<CProperty> properties;
        COpenOptions options;
        options.types = &types;
        options.excludedFormats = &excluded;
        options.props = &properties;
        options.filePath = archive.toStdWString().c_str();
        std::shared_ptr<ArchiveNativeOpen> reader;
        QCOMPARE(backend.openNativeArchive(archive, {}, nullptr, reader, &options), S_OK);
        QVERIFY(reader);
        reader.reset();
        CProperty &property = properties.AddNew();
        property.Name = L"memx";
        property.Value = L"invalid";
        QCOMPARE(backend.openNativeArchive(archive, {}, nullptr, reader, &options), E_INVALIDARG);
        QVERIFY(!reader);
        properties.Clear();
        CancelOpen cancel;
        QCOMPARE(backend.openNativeArchive(archive, {}, &cancel, reader, &options), E_ABORT);
        QVERIFY(!reader);
        QCOMPARE(SourceExtractFixture::readFile(archive), bytes);
    }
};
QTEST_MAIN(ArchiveOpenOptionsTests)
#include "archive_open_options_tests.moc"
