#include "CPP/7zip/UI/Agent/Agent.h"
#include "CPP/7zip/UI/FileManager/FileFolderPluginOpen.h"
#include "CPP/7zip/UI/FileManager/UpdateCallback100.h"
#include "archive_backend_test_support.h"
#include "archivecodeccatalog.h"
#include "qtarchivefileresource.h"
#include "qtarchivefoldermanager.h"
#include <QApplication>
#include <QScopeGuard>
#include <QStandardPaths>

namespace {
QByteArray readFile(const QString &path) {
    QFile file(path);
    return file.open(QIODevice::ReadOnly) ? file.readAll() : QByteArray();
}
class CCodecUpdateWorker final : public CProgressThreadVirt {
    CMyComPtr<IFolderOperations> _operations;
    QString _source;
    HRESULT ProcessVirt() override {
        CMyComPtr2_Create<IFolderArchiveUpdateCallback, CUpdateCallback100Imp> ui;
        ui->ProgressDialog = this;
        ui->Init();
        if (UseSourcePlan) {
            const std::wstring parent = QFileInfo(_source).absolutePath().toStdWString();
            const std::wstring name = QFileInfo(_source).fileName().toStdWString();
            const wchar_t *names[] = {name.c_str()};
            return _operations->CopyFrom(0, parent.c_str(), names, 1, ui);
        }
        return _operations->CopyFromFile(0, _source.toStdWString().c_str(), ui);
    }

  public:
    bool UseSourcePlan = false;
    CCodecUpdateWorker(IFolderOperations *operations, const QString &source)
        : _operations(operations), _source(source) {}
};
} // namespace

class CodecUpdateTests final : public QObject, protected ArchiveBackendTestSupport {
    Q_OBJECT
    QString engine_, module_;
    quint32 version_ = 0;
  private slots:
    void initTestCase() {
#ifndef SEVENZIP_CODEC_MODULE
        QSKIP("Official module unavailable; native writer transactions unverified");
#else
        engine_ = QStandardPaths::findExecutable("7zz");
        if (engine_.isEmpty())
            QSKIP("Official engine unavailable; native writer transactions unverified");
        module_ = QString::fromUtf8(SEVENZIP_CODEC_MODULE);
        ArchiveBackend backend(nullptr, engine_);
        QSignalSpy ready(&backend, &ArchiveBackend::codecCatalogReady);
        QVERIFY(backend.discoverCodecCatalog(module_));
        QTRY_COMPARE(ready.size(), 1);
        QVERIFY2(ready[0][0].toBool(), qPrintable(ready[0][1].toString()));
        version_ = backend.codecCatalog()->version();
#endif
    }
    void sourceCopyFromFilePublishesNewBytes_data() {
        QTest::addColumn<QString>("format");
        QTest::addColumn<QByteArray>("mode");
        QTest::addColumn<bool>("sourcePlan");
        for (const QString &format : {"7z", "zip", "gzip", "bzip2", "xz", "tar", "wim"})
            for (const QByteArray &mode :
                 {QByteArray("success"), QByteArray("fail"), QByteArray("cancel")})
                for (bool sourcePlan : {false, true})
                    QTest::newRow(qPrintable(format + QString::fromLatin1(mode) +
                                             (sourcePlan ? " plan" : " file")))
                        << format << mode << sourcePlan;
    }
    void sourceCopyFromFilePublishesNewBytes() {
        QFETCH(QString, format);
        QFETCH(QByteArray, mode);
        QFETCH(bool, sourcePlan);
        QTemporaryDir fixture;
        QVERIFY(fixture.isValid());
        const QString name = QStringLiteral("исходник 空 with spaces.bin");
        const QString source = fixture.filePath(name);
        writeFile(source, "original");
        // Even handlers without a stored filename derive this name from the
        // unknown archive suffix through the source DefaultName implementation.
        const QString archive = fixture.filePath(name + ".unknown");
        QProcess creator;
        creator.setWorkingDirectory(fixture.path());
        creator.start(engine_, {"a", "-t" + format, "--", archive, name});
        QVERIFY(creator.waitForFinished(10000));
        QCOMPARE(creator.exitCode(), 0);
        const QByteArray original = readFile(archive);
        QVERIFY(!original.isEmpty());
        ArchiveBackend backend(nullptr, engine_);
        QCOMPARE(backend.loadCodecCatalog(module_, version_), S_OK);
        CMyComPtr<IFolderManager> manager = new QtArchiveFolderManager(backend);
        CFfpOpen opening(manager);
        QCOMPARE(opening.OpenFileFolderPlugin(nullptr, us2fs(archive.toStdWString().c_str()),
                                              UString(format.toStdWString().c_str()), nullptr),
                 S_OK);
        auto *folder = dynamic_cast<CAgentFolder *>(opening.Folder.Interface());
        QVERIFY(folder);
        auto *resource = dynamic_cast<QtArchiveFileResource *>(folder->GetEngineResource());
        QVERIFY(resource && resource->HasUpdateService());
        QCOMPARE(folder->LoadItems(), S_OK);
        UInt32 count = 0;
        QCOMPARE(folder->GetNumberOfItems(&count), S_OK);
        QCOMPARE(count, UInt32(1));
        CMyComPtr<IFolderOperations> operations;
        QCOMPARE(opening.Folder.QueryInterface(IID_IFolderOperations, &operations), S_OK);
        const QByteArray replacement("changed\0binary bytes", 20);
        writeFile(source, replacement);
        if (sourcePlan) {
            CompressionOptions options;
            options.format = format;
            options.level = format == "tar" || format == "wim" ? 0 : 5;
            resource->SetCompressionOptions(options);
        }
        std::unique_ptr<ArchiveBackend> faultBackend;
        const QByteArray previousHelper = qgetenv("SEVENZIP_TEST_HELPER");
        const auto restoreHelper = qScopeGuard([&] {
            if (previousHelper.isNull())
                qunsetenv("SEVENZIP_TEST_HELPER");
            else
                qputenv("SEVENZIP_TEST_HELPER", previousHelper);
        });
        bool observedPartial = false;
        if (mode != "success") {
            qputenv("SEVENZIP_TEST_HELPER", mode);
            faultBackend =
                std::make_unique<ArchiveBackend>(nullptr, QCoreApplication::applicationFilePath());
            QCOMPARE(faultBackend->loadCodecCatalog(module_, version_), S_OK);
            resource->AttachUpdateService(*faultBackend, format);
            connect(faultBackend.get(), &ArchiveBackend::progress, this, [&](int percent) {
                if (percent == 50) {
                    observedPartial = true;
                    if (mode == "cancel")
                        faultBackend->cancel();
                }
            });
        }
        CCodecUpdateWorker worker(operations, source);
        worker.UseSourcePlan = sourcePlan;
        worker.DisableUserQuestions = true;
        auto transport = resource->CreateProgressTransport(worker);
        QCOMPARE(worker.Create(L"Native codec update", nullptr), S_OK);
        if (mode != "success") {
            QCOMPARE(worker.Result, mode == "cancel" ? E_ABORT : E_FAIL);
            QVERIFY(observedPartial);
            QCOMPARE(readFile(archive), original);
            QCOMPARE(readFile(source), replacement);
            QVERIFY(QDir(fixture.path())
                        .entryList({"*.tmp", ".sevenzip-*"}, QDir::Files | QDir::Hidden)
                        .isEmpty());
            return;
        }
        QCOMPARE(worker.Result, S_OK);
        QVERIFY(readFile(archive) != original);
        QProcess extractor;
        extractor.start(engine_,
                        {"x", "-y", "-t" + format, "-o" + fixture.filePath("out"), "--", archive});
        QVERIFY(extractor.waitForFinished(10000));
        QCOMPARE(extractor.exitCode(), 0);
        QCOMPARE(readFile(QDir(fixture.filePath("out")).filePath(name)), replacement);
        QCOMPARE(readFile(source), replacement);
    }
};
int main(int argc, char **argv) {
    const QByteArray mode = qgetenv("SEVENZIP_TEST_HELPER");
    if (mode == "fail" || mode == "cancel") {
        QCoreApplication app(argc, argv);
        const QString prefix = "-up1q1r2x2y2z2w2!";
        QString output;
        for (const QString &argument : app.arguments())
            if (argument.startsWith(prefix))
                output = argument.mid(prefix.size());
        if (output.isEmpty() && (app.arguments().contains("a") || app.arguments().contains("d"))) {
            const int separator = app.arguments().indexOf("--");
            if (separator >= 0 && separator + 1 < app.arguments().size()) {
                const QString candidate = app.arguments()[separator + 1];
                if (QFileInfo(candidate).fileName().startsWith("result."))
                    output = candidate;
            }
        }
        if (output.isEmpty())
            return 3;
        QFile partial(output);
        if (!partial.open(QIODevice::WriteOnly))
            return 4;
        partial.write("partial output");
        partial.close();
        writeOutput("50%\n");
        if (mode == "fail")
            return 2;
        QThread::msleep(30000);
        return 0;
    }
    QApplication app(argc, argv);
    CodecUpdateTests tests;
    return QTest::qExec(&tests, argc, argv);
}
#include "codec_update_tests.moc"
