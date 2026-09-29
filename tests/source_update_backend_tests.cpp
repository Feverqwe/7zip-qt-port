#include "7zip/Common/FileStreams.h"
#include "CPP/7zip/UI/Agent/Agent.h"
#include "CPP/7zip/UI/Agent/UpdateCallbackAgent.h"
#include "CPP/7zip/UI/Common/WorkDir.h"
#include "CPP/7zip/UI/FileManager/FileFolderPluginOpen.h"
#include "CPP/7zip/UI/FileManager/ProgressDialog2.h"
#include "CPP/7zip/UI/FileManager/UpdateCallback100.h"
#include "Common/StringConvert.h"
#include "archivebackend.h"
#include "archiveprogresstransport.h"
#include "qtarchivebackendrequest.h"
#include "qtarchivefileresource.h"
#include "qtarchivefoldermanager.h"
#include "qtarchivesourceupdate.h"
#include "qtpanelfolder.h"
#include <QDir>
#include <QFile>
#include <QProcess>
#include <QSignalSpy>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QtTest>

namespace {
QString enginePath() { return QStandardPaths::findExecutable("7zz"); }
bool writeFile(const QString &path, const QByteArray &data) {
    if (!QDir().mkpath(QFileInfo(path).absolutePath()))
        return false;
    QFile file(path);
    return file.open(QIODevice::WriteOnly) && file.write(data) == data.size();
}
QByteArray readFile(const QString &path) {
    QFile file(path);
    return file.open(QIODevice::ReadOnly) ? file.readAll() : QByteArray();
}
bool runEngine(const QString &directory, const QStringList &args) {
    QProcess process;
    process.setWorkingDirectory(directory);
    process.start(enginePath(), args);
    return process.waitForFinished(10000) && process.exitStatus() == QProcess::NormalExit &&
           process.exitCode() == 0;
}
class CBackendWorker final : public CProgressThreadVirt {
    ArchiveBackend &_backend;
    HRESULT ProcessVirt() override {
        return RunArchiveBackendRequest(_backend, Start, nullptr, &Sync);
    }

  public:
    std::function<void()> Start;
    explicit CBackendWorker(ArchiveBackend &backend) : _backend(backend) {}
};
class CNativeUpdateWorker final : public CProgressThreadVirt {
    QString _archive, _disk, _output;
    CMyComPtr<IUnknown> _resource;
    QtArchiveFileResource *_resourceSpec;
    HRESULT ProcessVirt() override {
        CObjectVector<CPanelItem> items;
        for (const QString &name :
             {QStringLiteral("inside/исходное имя.bin"), QStringLiteral("untouched.bin")}) {
            CPanelItem item;
            item.RelPath = name;
            items.Add(item);
        }
        CMyComPtr<IFolderFolder> props =
            new QtPanelFolder({}, items, QtPanelFolder::PropertyCatalog::ArchiveTransport);
        CMyComPtr2_Create<IUnknown, CAgent> agent;
        RINOK(agent->Init({}, props, UString("zip"), _resource))
        CMyComPtr<IFolderFolder> root, inside;
        RINOK(agent->BindToRootFolder(&root))
        RINOK(root->BindToFolder(L"inside", &inside))
        RINOK(inside->LoadItems())
        RINOK(agent->SetFolder(inside))
        CMyComPtr2_Create<IFolderArchiveUpdateCallback, CUpdateCallback100Imp> ui;
        ui->ProgressDialog = this;
        ui->Init();
        const UInt32 selected = 0;
        if (Publish) {
            CMyComPtr<IFolderOperations> operations;
            RINOK(inside.QueryInterface(IID_IFolderOperations, &operations))
            RINOK(operations->CopyFromFile(selected, _disk.toStdWString().c_str(), ui))
            RINOK(inside->LoadItems())
            NWindows::NCOM::CPropVariant size;
            RINOK(inside->GetProperty(0, kpidSize, &size))
            if (size.vt != VT_UI8)
                return E_FAIL;
            ReopenedSize = size.uhVal.QuadPart;
            return S_OK;
        }
        auto *outputSpec = new COutFileStream;
        CMyComPtr<ISequentialOutStream> output = outputSpec;
        if (!outputSpec->Create_NEW(us2fs(_output.toStdWString().c_str())))
            return E_FAIL;
        RINOK(agent->UpdateOneFile(output, &selected, 1, _disk.toStdWString().c_str(), ui))
        return outputSpec->Close();
    }

  public:
    bool Publish = false;
    UInt64 ReopenedSize = 0;
    CNativeUpdateWorker(ArchiveBackend &backend, const QString &archive, const QString &disk,
                        const QString &output)
        : _archive(archive), _disk(disk), _output(output),
          _resourceSpec(new QtArchiveFileResource(archive)) {
        _resource = _resourceSpec;
        _resourceSpec->AttachUpdateService(backend, "zip");
    }
    std::unique_ptr<IQtProgressTransport> CreateProgressTransport() {
        return _resourceSpec->CreateProgressTransport(*this);
    }
};
} // namespace

class SourceUpdateBackendTests final : public QObject {
    Q_OBJECT
  private slots:
    void initTestCase() {
        QVERIFY2(!enginePath().isEmpty(), "7zz required for local source-update fixture");
    }
    void separateOutputPreservesOriginalAndUnchangedEntries_data() {
        QTest::addColumn<QString>("format");
        QTest::newRow("zip") << QStringLiteral("zip");
        QTest::newRow("7z") << QStringLiteral("7z");
    }
    void nativeWorkerWaitsForSeparateEngineOutput() {
        QTemporaryDir fixture;
        QVERIFY(fixture.isValid());
        const QString root = fixture.filePath("source");
        const QString archive = fixture.filePath("original.zip");
        const QString output = fixture.filePath("updated.zip");
        QVERIFY(writeFile(QDir(root).filePath("input.bin"), "old"));
        QVERIFY(runEngine(root, {"a", "-tzip", "--", archive, "input.bin"}));
        const QByteArray original = readFile(archive);
        QVERIFY(writeFile(QDir(root).filePath("input.bin"), "new"));
        ArchiveBackend backend(nullptr, enginePath());
        CBackendWorker worker(backend);
        worker.Start = [&] {
            backend.updateSourceFile(archive, "input.bin", root, output, "zip", {});
        };
        ArchiveProgressTransport transport(worker, backend);
        QCOMPARE(worker.Create(L"input.bin", nullptr), S_OK);
        QCOMPARE(worker.Result, S_OK);
        QVERIFY(!backend.busy());
        QVERIFY(QFile::exists(output));
        QCOMPARE(readFile(archive), original);
    }
    void nativeCallbackKeepsArchiveNameAndClosesInputStreams_data() {
        QTest::addColumn<bool>("publish");
        QTest::newRow("separate-output") << false;
        QTest::newRow("native-workdir-publish-reopen") << true;
    }
    void nativeCallbackKeepsArchiveNameAndClosesInputStreams() {
        QFETCH(bool, publish);
        QTemporaryDir fixture;
        QVERIFY(fixture.isValid());
        const QString root = fixture.filePath("original");
        const QString entry = QStringLiteral("inside/исходное имя.bin");
        const QString archive = fixture.filePath("original.zip");
        const QString output = fixture.filePath("updated.zip");
        const QString disk = fixture.filePath("different disk name.bin");
        QVERIFY(writeFile(QDir(root).filePath(entry), "old"));
        QVERIFY(writeFile(QDir(root).filePath("untouched.bin"), "keep"));
        QVERIFY(runEngine(root, {"a", "-tzip", "--", archive, entry, "untouched.bin"}));
        const QByteArray original = readFile(archive);
        QVERIFY(writeFile(disk, "replacement"));
        ArchiveBackend backend(nullptr, enginePath());
        CNativeUpdateWorker worker(backend, archive, disk, output);
        worker.Publish = publish;
        auto transport = worker.CreateProgressTransport();
        QVERIFY(transport);
        QCOMPARE(worker.Create(L"Source update", nullptr), S_OK);
        QCOMPARE(worker.Result, S_OK);
        if (publish)
            QCOMPARE(worker.ReopenedSize, UInt64(11));
        else
            QCOMPARE(readFile(archive), original);
        const QString extracted = fixture.filePath("extracted");
        QVERIFY(runEngine(fixture.path(),
                          {"x", "-y", "-o" + extracted, "--", publish ? archive : output}));
        QCOMPARE(readFile(QDir(extracted).filePath(entry)), QByteArray("replacement"));
        QCOMPARE(readFile(QDir(extracted).filePath("untouched.bin")), QByteArray("keep"));
        QVERIFY(!QFile::exists(QDir(extracted).filePath("different disk name.bin")));
    }
    void separateOutputPreservesOriginalAndUnchangedEntries() {
        QFETCH(QString, format);
        QTemporaryDir fixture;
        QVERIFY(fixture.isValid());
        const QString entry = QStringLiteral("inside/исходное имя.bin");
        const QString originalRoot = fixture.filePath("original");
        const QString sourceRoot = fixture.filePath("replacement");
        const QString archive = fixture.filePath("original." + format);
        const QString output = fixture.filePath("updated." + format);
        QVERIFY(writeFile(QDir(originalRoot).filePath(entry), "original"));
        QVERIFY(writeFile(QDir(originalRoot).filePath("untouched.bin"), "keep"));
        QVERIFY(writeFile(QDir(sourceRoot).filePath(entry), "updated data"));
        QVERIFY(runEngine(originalRoot, {"a", "-t" + format, "--", archive, "."}));
        const QByteArray originalBytes = readFile(archive);
        QVERIFY(!originalBytes.isEmpty());
        ArchiveBackend backend(nullptr, enginePath());
        QSignalSpy finished(&backend, &ArchiveBackend::finished);
        backend.updateSourceFile(archive, entry, sourceRoot, output, format, {});
        QTRY_COMPARE_WITH_TIMEOUT(finished.count(), 1, 10000);
        QVERIFY2(finished[0][0].toBool(), qPrintable(finished[0][1].toString()));
        QCOMPARE(backend.lastResult(), S_OK);
        QVERIFY(!backend.busy());
        QCOMPARE(readFile(archive), originalBytes);
        const QString extracted = fixture.filePath("extracted");
        QVERIFY(runEngine(fixture.path(), {"x", "-y", "-o" + extracted, "--", output}));
        QCOMPARE(readFile(QDir(extracted).filePath(entry)), QByteArray("updated data"));
        QCOMPARE(readFile(QDir(extracted).filePath("untouched.bin")), QByteArray("keep"));
    }
    void cancellationInNativeTransactionKeepsOriginalArchive() {
        QTemporaryDir fixture;
        QVERIFY(fixture.isValid());
        const QString root = fixture.filePath("original");
        const QString entry = QStringLiteral("inside/исходное имя.bin");
        const QString archive = fixture.filePath("original.zip");
        const QString disk = fixture.filePath("different disk name.bin");
        QVERIFY(writeFile(QDir(root).filePath(entry), "old"));
        QVERIFY(writeFile(QDir(root).filePath("untouched.bin"), "keep"));
        QVERIFY(runEngine(root, {"a", "-tzip", "--", archive, entry, "untouched.bin"}));
        const QByteArray original = readFile(archive);
        QVERIFY(writeFile(disk, QByteArray(4 * 1024 * 1024, 'x')));
        ArchiveBackend backend(nullptr, enginePath());
        CNativeUpdateWorker worker(backend, archive, disk, {});
        worker.Publish = true;
        auto transport = worker.CreateProgressTransport();
        QVERIFY(transport);
        QSignalSpy started(&backend, &ArchiveBackend::engineStarted);
        connect(&backend, &ArchiveBackend::engineStarted, &backend,
                [&worker] { worker.Sync.Set_Stopped(true); });
        QCOMPARE(worker.Create(L"Cancel source update", nullptr), S_OK);
        QCOMPARE(worker.Result, E_ABORT);
        QCOMPARE(started.count(), 1);
        QVERIFY(!backend.busy());
        QCOMPARE(readFile(archive), original);
        QCOMPARE(worker.ReopenedSize, UInt64(0));
    }
    void panelChangedFileUsesNativeFolderTransaction() {
        QTemporaryDir fixture;
        QVERIFY(fixture.isValid());
        const QString root = fixture.filePath("original");
        const QString entry = QStringLiteral("inside/исходное имя.bin");
        const QString archive = fixture.filePath("original.zip");
        const QString disk = fixture.filePath("different disk name.bin");
        QVERIFY(writeFile(QDir(root).filePath(entry), "old"));
        QVERIFY(writeFile(QDir(root).filePath("untouched.bin"), "keep"));
        QVERIFY(runEngine(root, {"a", "-tzip", "--", archive, entry, "untouched.bin"}));
        QVERIFY(writeFile(disk, "source panel replacement"));
        ArchiveBackend backend(nullptr, enginePath());
        CMyComPtr<IFolderManager> manager = new QtArchiveFolderManager(backend);
        CFfpOpen opening(manager);
        QCOMPARE(opening.OpenFileFolderPlugin(nullptr, us2fs(archive.toStdWString().c_str()),
                                              UString("zip"), nullptr),
                 S_OK);
        CMyComPtr<IFolderFolder> inside;
        QCOMPARE(opening.Folder->BindToFolder(L"inside", &inside), S_OK);
        QCOMPARE(inside->LoadItems(), S_OK);
        CPanel panel(inside);
        CTempFileInfo temp;
        temp.FileIndex = 99; // Original item index became stale during the external edit.
        temp.RelPath = L"исходное имя.bin";
        temp.FilePath = us2fs(disk.toStdWString().c_str());
        const UString prefix = panel.GetCurrentFolderPrefix();
        const auto original = readFile(archive);
        QSignalSpy started(&backend, &ArchiveBackend::engineStarted);
        QVERIFY(!panel.OnOpenItemChanged(temp, UString(L"different folder/"), false, UString()));
        QCOMPARE(started.size(), 0);
        QCOMPARE(readFile(archive), original);
        temp.RelPath = L"removed.bin";
        QVERIFY(!panel.OnOpenItemChanged(temp, prefix, false, UString()));
        QCOMPARE(started.size(), 0);
        QCOMPARE(readFile(archive), original);
        temp.RelPath = L"исходное имя.bin";
        QVERIFY(panel.OnOpenItemChanged(temp, prefix, false, UString()));
        QVERIFY(started.size() > 0);
        QVERIFY(!backend.busy());
        NWindows::NCOM::CPropVariant size;
        QCOMPARE(panel.GetFolder()->GetProperty(0, kpidSize, &size), S_OK);
        QCOMPARE(size.vt, VARTYPE(VT_UI8));
        QCOMPARE(size.uhVal.QuadPart, UInt64(24));
        const QString extracted = fixture.filePath("extracted");
        QVERIFY(runEngine(fixture.path(), {"x", "-y", "-o" + extracted, "--", archive}));
        QCOMPARE(readFile(QDir(extracted).filePath(entry)), QByteArray("source panel replacement"));
        QCOMPARE(readFile(QDir(extracted).filePath("untouched.bin")), QByteArray("keep"));
        panel.ReleaseFolder();
        QVERIFY(!panel.OnOpenItemChanged(temp, prefix, false, UString()));
    }
    void existingDestinationAndUnsafeEntryAreRejected() {
        QTemporaryDir fixture;
        QVERIFY(fixture.isValid());
        const QString archive = fixture.filePath("original.zip");
        const QString output = fixture.filePath("existing.zip");
        QVERIFY(writeFile(archive, "original"));
        QVERIFY(writeFile(output, "destination"));
        QVERIFY(writeFile(fixture.filePath("input.bin"), "new"));
        ArchiveBackend backend(nullptr, enginePath());
        QSignalSpy finished(&backend, &ArchiveBackend::finished);
        backend.updateSourceFile(archive, "input.bin", fixture.path(), output, "zip", {});
        QCOMPARE(finished.count(), 1);
        QVERIFY(!finished[0][0].toBool());
        QCOMPARE(readFile(output), QByteArray("destination"));
        backend.updateSourceFile(archive, "../input.bin", fixture.path(),
                                 fixture.filePath("new.zip"), "zip", {});
        QCOMPARE(finished.count(), 2);
        QVERIFY(!finished[1][0].toBool());
        QCOMPARE(readFile(archive), QByteArray("original"));
        QVERIFY(!QFile::exists(fixture.filePath("new.zip")));
        QVERIFY(!backend.busy());
    }
    void sourceNamesRequiringCorrectionAreRejected() {
        for (const QString &path :
             {QString(), QStringLiteral("../escape"), QStringLiteral("/absolute"),
              QStringLiteral("a/../b"), QStringLiteral("a/./b"), QStringLiteral("a//b")})
            QVERIFY2(!ArchiveBackend::sourceUpdatePathIsSafe(path), qPrintable(path));
        QVERIFY(
            !ArchiveBackend::sourceUpdatePathIsSafe(QStringLiteral("safe") + QChar(0) + "tail"));
        QVERIFY(
            ArchiveBackend::sourceUpdatePathIsSafe(QStringLiteral("inside/имя с пробелом.bin")));
    }
    void cancellationAndFailedStartPreserveOriginal() {
        QTemporaryDir fixture;
        QVERIFY(fixture.isValid());
        const QString archive = fixture.filePath("original.zip");
        const QString output = fixture.filePath("updated.zip");
        const QString sourceRoot = fixture.filePath("source");
        QVERIFY(
            writeFile(QDir(sourceRoot).filePath("input.bin"), QByteArray(4 * 1024 * 1024, 'x')));
        QVERIFY(runEngine(sourceRoot, {"a", "-tzip", "--", archive, "input.bin"}));
        const QByteArray original = readFile(archive);
        ArchiveBackend backend(nullptr, enginePath());
        QSignalSpy finished(&backend, &ArchiveBackend::finished);
        HRESULT paused = E_FAIL;
        connect(&backend, &ArchiveBackend::engineStarted, this, [&] {
            paused = backend.setPaused(true);
            backend.cancel();
        });
        backend.updateSourceFile(archive, "input.bin", sourceRoot, output, "zip", {});
        QTRY_COMPARE_WITH_TIMEOUT(finished.count(), 1, 10000);
        QCOMPARE(paused, S_OK);
        QVERIFY(!finished[0][0].toBool());
        QCOMPARE(backend.lastResult(), E_ABORT);
        QVERIFY(!backend.busy());
        QVERIFY(!QFile::exists(output));
        QCOMPARE(readFile(archive), original);
        ArchiveBackend failed(nullptr, fixture.filePath("missing-engine"));
        QSignalSpy failedFinished(&failed, &ArchiveBackend::finished);
        failed.updateSourceFile(archive, "input.bin", sourceRoot, output, "zip", {});
        QTRY_COMPARE_WITH_TIMEOUT(failedFinished.count(), 1, 3000);
        QVERIFY(!failedFinished[0][0].toBool());
        QCOMPARE(failed.lastResult(), E_FAIL);
        QVERIFY(!failed.busy());
        QVERIFY(!QFile::exists(output));
        QCOMPARE(readFile(archive), original);
    }
};

QTEST_MAIN(SourceUpdateBackendTests)
#include "source_update_backend_tests.moc"
