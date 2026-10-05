#include "7zip/Common/FileStreams.h"
#include "CPP/7zip/UI/Agent/Agent.h"
#include "CPP/7zip/UI/Agent/UpdateCallbackAgent.h"
#include "CPP/7zip/UI/Common/EnumDirItems.h"
#include "CPP/7zip/UI/Common/WorkDir.h"
#include "CPP/7zip/UI/FileManager/FileFolderPluginOpen.h"
#include "CPP/7zip/UI/FileManager/ProgressDialog2.h"
#include "CPP/7zip/UI/FileManager/UpdateCallback100.h"
#include "CPP/7zip/UI/GUI/UpdateGUI.h"
#include "Common/StringConvert.h"
#include "archivebackend.h"
#include "archiveprogresstransport.h"
#include "qtarchivebackendrequest.h"
#include "qtarchivefileresource.h"
#include "qtarchivefoldermanager.h"
#include "qtarchivesourceupdate.h"
#include "qtpanelfolder.h"
#include "technical_listing_fixture.h"
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QProcess>
#include <QSignalSpy>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QtTest>
#include <vector>

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
class CNativeMutationWorker final : public CProgressThreadVirt {
    CMyComPtr<IUnknown> _resource;
    QtArchiveFileResource *_resourceSpec;
    QString _format;
    HRESULT ProcessVirt() override {
        CObjectVector<CPanelItem> items;
        for (const QString &name :
             {QStringLiteral("inside/имя с пробелом.bin"), QStringLiteral("keep.bin")}) {
            CPanelItem item;
            item.RelPath = name;
            items.Add(item);
        }
        CMyComPtr<IFolderFolder> props =
            new QtPanelFolder({}, items, QtPanelFolder::PropertyCatalog::ArchiveTransport);
        CMyComPtr2_Create<IUnknown, CAgent> agent;
        RINOK(agent->Init({}, props, ToUString(_format), _resource))
        CMyComPtr<IFolderFolder> root, inside;
        RINOK(agent->BindToRootFolder(&root))
        RINOK(root->BindToFolder(L"inside", &inside))
        CMyComPtr<IFolderFolder> folder = RootFolder ? root : inside;
        RINOK(folder->LoadItems())
        CMyComPtr<IFolderOperations> operations;
        RINOK(folder.QueryInterface(IID_IFolderOperations, &operations))
        CMyComPtr2_Create<IFolderArchiveUpdateCallback, CUpdateCallback100Imp> ui;
        ui->ProgressDialog = this;
        ui->Init();
        if (!Password.isEmpty()) {
            ui->PasswordIsDefined = true;
            ui->Password = ToUString(Password);
        }
        const UInt32 selected = 0;
        switch (Operation) {
        case ArchiveBackend::SourceMutation::Delete:
            return operations->Delete(&selected, 1, ui);
        case ArchiveBackend::SourceMutation::Rename:
            return operations->Rename(selected, L"новое имя.bin", ui);
        case ArchiveBackend::SourceMutation::CreateFolder:
            return operations->CreateFolder(L"новая папка", ui);
        }
        return E_FAIL;
    }

  public:
    ArchiveBackend::SourceMutation Operation = ArchiveBackend::SourceMutation::Delete;
    bool RootFolder = false;
    QString Password;
    CNativeMutationWorker(ArchiveBackend &backend, const QString &archive, const QString &format)
        : _resourceSpec(new QtArchiveFileResource(archive)), _format(format) {
        _resource = _resourceSpec;
        _resourceSpec->AttachUpdateService(backend, format);
    }
    std::unique_ptr<IQtProgressTransport> CreateProgressTransport() {
        return _resourceSpec->CreateProgressTransport(*this);
    }
};

class CDeletionTrace final : public IFolderArchiveUpdateCallback, public CMyUnknownImp {
    Z7_COM_UNKNOWN_IMP_1(IFolderArchiveUpdateCallback)
    CMyComPtr<IFolderArchiveUpdateCallback> callback_;
    QStringList &deleted_;

  public:
    CDeletionTrace(IFolderArchiveUpdateCallback *callback, QStringList &deleted)
        : callback_(callback), deleted_(deleted) {}
    Z7_COM7F_IMF(SetTotal(UInt64 value)) override { return callback_->SetTotal(value); }
    Z7_COM7F_IMF(SetCompleted(const UInt64 *value)) override {
        return callback_->SetCompleted(value);
    }
    Z7_COM7F_IMF(CompressOperation(const wchar_t *name)) override {
        return callback_->CompressOperation(name);
    }
    Z7_COM7F_IMF(DeleteOperation(const wchar_t *name)) override {
        deleted_.append(QString::fromWCharArray(name));
        return callback_->DeleteOperation(name);
    }
    Z7_COM7F_IMF(OperationResult(Int32 value)) override {
        return callback_->OperationResult(value);
    }
    Z7_COM7F_IMF(UpdateErrorMessage(const wchar_t *message)) override {
        return callback_->UpdateErrorMessage(message);
    }
    Z7_COM7F_IMF(SetNumFiles(UInt64 count)) override { return callback_->SetNumFiles(count); }
};

class CNativeAddWorker final : public CProgressThreadVirt {
    QString _source, _format;
    CMyComPtr<IUnknown> _resource;
    CMyComPtr<IFolderFolder> _openedFolder;
    QtArchiveFileResource *_resourceSpec;
    HRESULT ProcessVirt() override {
        CMyComPtr<IFolderFolder> folder = _openedFolder;
        if (!folder) {
            CObjectVector<CPanelItem> items;
            const QStringList archivePaths = ArchivePaths.isEmpty()
                                                 ? QStringList{QStringLiteral("inside/keep.bin")}
                                                 : ArchivePaths;
            for (const QString &path : archivePaths) {
                CPanelItem item;
                item.RelPath = path;
                item.IsFolder = ArchiveDirectories.contains(path);
                item.MTimeText = ArchiveMTimeText;
                items.Add(item);
            }
            CMyComPtr<IFolderFolder> props =
                new QtPanelFolder({}, items, QtPanelFolder::PropertyCatalog::ArchiveTransport);
            CMyComPtr2_Create<IUnknown, CAgent> agent;
            RINOK(agent->Init({}, props, ToUString(_format), _resource))
            CMyComPtr<IFolderFolder> root, inside;
            RINOK(agent->BindToRootFolder(&root))
            folder = root;
            if (!RootFolder) {
                RINOK(root->BindToFolder(L"inside", &inside))
                folder = inside;
            }
            RINOK(folder->LoadItems())
        }
        CMyComPtr<IFolderOperations> operations;
        RINOK(folder.QueryInterface(IID_IFolderOperations, &operations))
        CMyComPtr2_Create<IFolderArchiveUpdateCallback, CUpdateCallback100Imp> ui;
        ui->ProgressDialog = this;
        ui->Init();
        if (!Password.isEmpty()) {
            ui->PasswordIsDefined = true;
            ui->Password = ToUString(Password);
        }
        if (PathMode) {
            CompressionOptions options;
            options.PathMode = *PathMode;
            _resourceSpec->SetCompressionOptions(options);
        }
        const QStringList sources = Sources.isEmpty() ? QStringList{_source} : Sources;
        CMyComPtr<IFolderArchiveUpdateCallback> callback = ui.Interface();
        if (TraceDeletion)
            callback = new CDeletionTrace(ui.Interface(), DeletedNames);
        const std::wstring parent = QFileInfo(sources.first()).absolutePath().toStdWString();
        std::vector<std::wstring> names;
        names.reserve(size_t(sources.size()));
        for (const QString &source : sources)
            names.push_back(
                (PathMode ? QFileInfo(source).absoluteFilePath() : QFileInfo(source).fileName())
                    .toStdWString());
        std::vector<const wchar_t *> pointers;
        pointers.reserve(names.size());
        for (const std::wstring &name : names)
            pointers.push_back(name.c_str());
        if (ActionSet) {
            auto *agentFolder = dynamic_cast<CAgentFolder *>((IFolderFolder *)folder);
            if (!agentFolder)
                return E_NOINTERFACE;
            if (PathMode)
                return agentFolder->UpdateFromPaths(pointers.data(), UInt32(pointers.size()),
                                                    *ActionSet, *PathMode, callback);
            return agentFolder->UpdateFrom(parent.c_str(), pointers.data(), UInt32(pointers.size()),
                                           *ActionSet, callback);
        }
        return operations->CopyFrom(MoveMode, parent.c_str(), pointers.data(),
                                    UInt32(pointers.size()), callback);
    }

  public:
    bool MoveMode = false;
    bool RootFolder = false;
    QStringList ArchivePaths;
    QStringList ArchiveDirectories;
    QString ArchiveMTimeText;
    const NUpdateArchive::CActionSet *ActionSet = nullptr;
    std::optional<NWildcard::ECensorPathMode> PathMode;
    QString Password;
    QStringList Sources;
    bool TraceDeletion = false;
    QStringList DeletedNames;
    CNativeAddWorker(ArchiveBackend &backend, const QString &archive, const QString &source,
                     const QString &format)
        : _source(source), _format(format), _resourceSpec(new QtArchiveFileResource(archive)) {
        _resource = _resourceSpec;
        _resourceSpec->AttachUpdateService(backend, format);
    }
    std::unique_ptr<IQtProgressTransport> CreateProgressTransport() {
        return _resourceSpec->CreateProgressTransport(*this);
    }
    void SetOpenedFolder(IFolderFolder *folder) {
        _openedFolder = folder;
        auto *agent = dynamic_cast<CAgentFolder *>(folder);
        _resourceSpec =
            agent ? dynamic_cast<QtArchiveFileResource *>(agent->GetEngineResource()) : nullptr;
        Q_ASSERT(_resourceSpec);
        _resource = _resourceSpec;
    }
};
} // namespace

class SourceUpdateBackendTests final : public QObject {
    Q_OBJECT
  private slots:
    void directoryPlanRetainsSubtreeAndRollsBack_data() {
        QTest::addColumn<QString>("format");
        QTest::addColumn<bool>("cancel");
        for (const QString &format : {QString("zip"), QString("7z")}) {
            QTest::newRow(qPrintable(format + "-success")) << format << false;
            QTest::newRow(qPrintable(format + "-cancel")) << format << true;
        }
    }
    void directoryPlanRetainsSubtreeAndRollsBack() {
        QFETCH(QString, format);
        QFETCH(bool, cancel);
        QTemporaryDir fixture;
        QVERIFY(fixture.isValid());
        const QString root = fixture.filePath("source");
        const QString archive = fixture.filePath("original." + format);
        const QString output = fixture.filePath("output." + format);
        QVERIFY(writeFile(QDir(root).filePath("tree/keep/child.txt"), "kept"));
        QVERIFY(writeFile(QDir(root).filePath("tree/keep/drop/old.txt"), "dropped"));
        QVERIFY(runEngine(root, {"a", "-t" + format, "--", archive, "tree"}));
        const QByteArray original = readFile(archive);
        ArchiveBackend::SourceUpdatePlan plan;
        plan.removed = {"tree/keep/drop/old.txt"};
        plan.removedDirectories = {"tree", "tree/keep/drop"};
        plan.retainedPaths = {"tree/keep", "tree/keep/child.txt"};
        ArchiveBackend backend(nullptr, enginePath());
        QSignalSpy finished(&backend, &ArchiveBackend::finished);
        int stages = 0;
        connect(&backend, &ArchiveBackend::engineStarted, &backend, [&] {
            if (++stages == 2 && cancel)
                backend.cancel();
        });
        backend.updateSourcePlan(archive, plan, output, format, {});
        QTRY_COMPARE_WITH_TIMEOUT(finished.count(), 1, 30000);
        QCOMPARE(finished.takeFirst().first().toBool(), !cancel);
        QCOMPARE(readFile(archive), original);
        if (cancel) {
            QCOMPARE(backend.lastResult(), E_ABORT);
            QVERIFY(!QFileInfo::exists(output));
        } else {
            QProcess listing;
            listing.start(enginePath(), {"l", "-slt", "-sccUTF-8", "--", output});
            QVERIFY(listing.waitForFinished(10000));
            QCOMPARE(listing.exitCode(), 0);
            const QByteArray contents = NormalizeTechnicalListing(listing.readAllStandardOutput());
            QVERIFY(contents.contains("Path = tree/keep\n"));
            QVERIFY(!contents.contains("Path = tree\n"));
            QVERIFY(!contents.contains("Path = tree/keep/drop\n"));
            const QString extracted = fixture.filePath("extracted");
            QVERIFY(QDir().mkpath(extracted));
            QVERIFY(runEngine(extracted, {"x", "-y", "--", output}));
            QCOMPARE(readFile(QDir(extracted).filePath("tree/keep/child.txt")), QByteArray("kept"));
        }
        plan.retainedPaths = {"../unsafe"};
        const QString rejected = fixture.filePath("rejected." + format);
        backend.updateSourcePlan(archive, plan, rejected, format, {});
        QCOMPARE(finished.count(), 1);
        QVERIFY(!finished.takeFirst().first().toBool());
        QVERIFY(!QFileInfo::exists(rejected));
        QCOMPARE(readFile(archive), original);
    }
    void nativeSyncRemovesDirectoryRecordsAndKeepsChildren_data() {
        QTest::addColumn<QString>("format");
        QTest::newRow("zip") << QString("zip");
        QTest::newRow("7z") << QString("7z");
    }
    void nativeSyncRemovesDirectoryRecordsAndKeepsChildren() {
        QFETCH(QString, format);
        QTemporaryDir fixture;
        QVERIFY(fixture.isValid());
        const QString oldRoot = fixture.filePath("old");
        const QString diskRoot = fixture.filePath("disk");
        const QString selectedFolder = QStringLiteral("папка с пробелом");
        const QString source = QDir(diskRoot).filePath(selectedFolder + "/nested/keep.txt");
        QVERIFY(writeFile(source, "older disk bytes"));
        NWildcard::CCensor censor;
        censor.AddPreItem_NoWildcard(ToUString(source));
        censor.AddPathsToCensor(NWildcard::k_FullPath);
        CDirItems enumerated;
        QCOMPARE(EnumerateItems(censor, NWildcard::k_FullPath, UString(), enumerated), S_OK);
        QCOMPARE(enumerated.Items.Size(), 1u);
        const QString fullName = QString::fromWCharArray(enumerated.GetLogPath(0));
        const QString folder =
            QDir::fromNativeSeparators(fullName).chopped(QStringLiteral("/nested/keep.txt").size());
        const QString child = folder + "/nested/keep.txt";
        const QString removed = folder + "/nested/remove.txt";
        const QString archive = fixture.filePath("directories." + format);
        QVERIFY(writeFile(QDir(oldRoot).filePath(selectedFolder + "/nested/keep.txt"),
                          "retained archive bytes"));
        QVERIFY(
            writeFile(QDir(oldRoot).filePath(selectedFolder + "/nested/remove.txt"), "removed"));
        QVERIFY(runEngine(oldRoot, {"a", "-t" + format, "--", archive, selectedFolder}));
        QVERIFY(runEngine(oldRoot, {"rn", "--", archive, selectedFolder, folder}));
        const QByteArray original = readFile(archive);
        QFile disk(source);
        QVERIFY(disk.open(QIODevice::ReadWrite));
        QVERIFY(disk.setFileTime(QDateTime::fromSecsSinceEpoch(1500000000, Qt::UTC),
                                 QFileDevice::FileModificationTime));
        disk.close();
        ArchiveBackend backend(nullptr, enginePath());
        CMyComPtr<IFolderManager> manager = new QtArchiveFolderManager(backend);
        CFfpOpen opening(manager);
        QCOMPARE(opening.OpenFileFolderPlugin(nullptr, us2fs(archive.toStdWString().c_str()),
                                              UString(), nullptr),
                 S_OK);
        auto *opened = dynamic_cast<CAgentFolder *>(opening.Folder.Interface());
        QVERIFY(opened);
        IInArchive *properties = opened->GetArchiveProperties();
        QVERIFY(properties);
        UInt32 count = 0;
        QCOMPARE(properties->GetNumberOfItems(&count), S_OK);
        QStringList actualPaths;
        for (UInt32 i = 0; i < count; ++i) {
            NWindows::NCOM::CPropVariant path;
            QCOMPARE(properties->GetProperty(i, kpidPath, &path), S_OK);
            QCOMPARE(path.vt, VARTYPE(VT_BSTR));
            actualPaths.append(QDir::fromNativeSeparators(QString::fromWCharArray(path.bstrVal)));
        }
        QVERIFY2(actualPaths.contains(child), qPrintable(actualPaths.join('\n')));
        CNativeAddWorker worker(backend, archive, source, format);
        worker.SetOpenedFolder(opening.Folder);
        worker.PathMode = NWildcard::k_FullPath;
        worker.ActionSet = &NUpdateArchive::k_ActionSet_Sync;
        worker.TraceDeletion = true;
        auto transport = worker.CreateProgressTransport();
        QCOMPARE(worker.Create(L"Synchronize selected child", nullptr), S_OK);
        QCOMPARE(worker.Result, S_OK);
        QStringList sourceDeleted;
        for (const QString &path : worker.DeletedNames)
            sourceDeleted.append(QDir::fromNativeSeparators(path));
        QVERIFY(sourceDeleted.contains(folder));
        QVERIFY(sourceDeleted.contains(removed));
        QVERIFY(!sourceDeleted.contains(child));
        QVERIFY(readFile(archive) != original);
        QProcess listing;
        listing.start(enginePath(), {"l", "-slt", "-sccUTF-8", "--", archive});
        QVERIFY(listing.waitForFinished(10000));
        QCOMPARE(listing.exitCode(), 0);
        const QByteArray contents = NormalizeTechnicalListing(listing.readAllStandardOutput());
        QVERIFY2(contents.contains("Path = " + child.toUtf8() + '\n'), contents.constData());
        QVERIFY(!contents.contains("Path = " + folder.toUtf8() + '\n'));
        QVERIFY(!contents.contains("Path = " + (folder + "/nested").toUtf8() + '\n'));
        QVERIFY(!contents.contains("Path = " + removed.toUtf8() + '\n'));
        const QString extracted = fixture.filePath("extracted");
        QVERIFY(QDir().mkpath(extracted));
        QVERIFY(runEngine(extracted, {"x", "-y", "--", archive}));
        QCOMPARE(readFile(QDir(extracted).filePath(child)), QByteArray("retained archive bytes"));
    }
    void sourcePathModeLogicalNames() {
        QTemporaryDir fixture;
        QVERIFY(fixture.isValid());
        const QString source = fixture.filePath("selected.txt");
        QVERIFY(writeFile(source, "path mode"));
        QStringList logicalNames;
        for (const auto mode :
             {NWildcard::k_RelatPath, NWildcard::k_FullPath, NWildcard::k_AbsPath}) {
            NWildcard::CCensor censor;
            censor.AddPreItem_NoWildcard(ToUString(source));
            censor.AddPathsToCensor(mode);
            CDirItems items;
            QCOMPARE(EnumerateItems(censor, mode, UString(), items), S_OK);
            QCOMPARE(items.Items.Size(), 1u);
            logicalNames.append(QString::fromWCharArray(items.GetLogPath(0).Ptr()));
        }
        QCOMPARE(logicalNames[0], QStringLiteral("selected.txt"));
#ifndef _WIN32
        QCOMPARE(logicalNames[1], source.mid(1));
#else
        QVERIFY(logicalNames[1].endsWith(QStringLiteral("selected.txt")));
#endif
        QCOMPARE(QDir::fromNativeSeparators(logicalNames[2]), source);
    }
    void nativeGuiPathModes_data() {
        QTest::addColumn<int>("mode");
        QTest::newRow("relative") << int(NWildcard::k_RelatPath);
        QTest::newRow("full") << int(NWildcard::k_FullPath);
        QTest::newRow("absolute") << int(NWildcard::k_AbsPath);
    }
    void nativeGuiPathModes() {
        QFETCH(int, mode);
        QTemporaryDir fixture;
        QVERIFY(fixture.isValid());
        const QString sourceRoot = fixture.filePath("old");
        const QString source = fixture.filePath("nested/selected.txt");
        if (mode == int(NWildcard::k_AbsPath))
            QVERIFY(ArchiveBackend::sourceUpdatePathIsSafe(source, true));
        const QString archive = fixture.filePath("paths.zip");
        QVERIFY(writeFile(QDir(sourceRoot).filePath("keep.txt"), "kept"));
        QVERIFY(runEngine(sourceRoot, {"a", "-tzip", "--", archive, "keep.txt"}));
        QVERIFY(writeFile(source, "path mode"));
        ArchiveBackend backend(nullptr, enginePath());
        CNativeAddWorker worker(backend, archive, source, "zip");
        worker.RootFolder = true;
        worker.ArchivePaths = {"keep.txt"};
        worker.ActionSet = &NUpdateArchive::k_ActionSet_Add;
        worker.PathMode = static_cast<NWildcard::ECensorPathMode>(mode);
        worker.DisableUserQuestions = true;
        auto transport = worker.CreateProgressTransport();
        QCOMPARE(worker.Create(L"Path mode", nullptr), S_OK);
        QCOMPARE(worker.Result, S_OK);
        if (mode == int(NWildcard::k_AbsPath)) {
            QProcess listing;
            listing.start(enginePath(), {"l", "-slt", "-sccUTF-8", "--", archive});
            QVERIFY(listing.waitForFinished(10000));
            QCOMPARE(listing.exitCode(), 0);
            QVERIFY(NormalizeTechnicalListing(listing.readAllStandardOutput())
                        .contains("Path = " + source.toUtf8()));
            return;
        }
        const QString extracted = fixture.filePath("extracted");
        QVERIFY(QDir().mkpath(extracted));
        QVERIFY(runEngine(extracted, {"x", "-y", "--", archive}));
        QString logical = QStringLiteral("selected.txt");
        if (mode == int(NWildcard::k_FullPath)) {
#ifdef _WIN32
            logical = QFileInfo(source).absoluteFilePath().mid(QDir::rootPath().length());
#else
            logical = QFileInfo(source).absoluteFilePath().mid(1);
#endif
        }
        QCOMPARE(readFile(QDir(extracted).filePath(logical)), QByteArray("path mode"));
        QCOMPARE(readFile(QDir(extracted).filePath("keep.txt")), QByteArray("kept"));
    }
    void nativeGuiDirectoryPathModes_data() {
        QTest::addColumn<int>("mode");
        QTest::newRow("relative") << int(NWildcard::k_RelatPath);
        QTest::newRow("full") << int(NWildcard::k_FullPath);
        QTest::newRow("absolute") << int(NWildcard::k_AbsPath);
    }
    void nativeGuiDirectoryPathModes() {
        QFETCH(int, mode);
        QTemporaryDir fixture;
        QVERIFY(fixture.isValid());
        const QString oldRoot = fixture.filePath("old");
        const QString folder = fixture.filePath(QStringLiteral("папка с пробелом"));
        const QString archive = fixture.filePath("directories.zip");
        QVERIFY(writeFile(QDir(oldRoot).filePath("keep.txt"), "keep"));
        QVERIFY(runEngine(oldRoot, {"a", "-tzip", "--", archive, "keep.txt"}));
        QVERIFY(writeFile(QDir(folder).filePath(QStringLiteral("внутри/файл.txt")), "nested"));
        QVERIFY(QDir().mkpath(QDir(folder).filePath("empty")));
        ArchiveBackend backend(nullptr, enginePath());
        CNativeAddWorker worker(backend, archive, folder, "zip");
        worker.RootFolder = true;
        worker.ArchivePaths = {"keep.txt"};
        worker.ActionSet = &NUpdateArchive::k_ActionSet_Add;
        worker.PathMode = static_cast<NWildcard::ECensorPathMode>(mode);
        worker.DisableUserQuestions = true;
        auto transport = worker.CreateProgressTransport();
        QCOMPARE(worker.Create(L"Directory path mode", nullptr), S_OK);
        QCOMPARE(worker.Result, S_OK);
        QString logical = QFileInfo(folder).fileName();
        if (mode == int(NWildcard::k_FullPath)) {
#ifdef _WIN32
            logical = folder.mid(QDir::rootPath().length());
#else
            logical = folder.mid(1);
#endif
        } else if (mode == int(NWildcard::k_AbsPath))
            logical = folder;
        QProcess listing;
        listing.start(enginePath(), {"l", "-slt", "-sccUTF-8", "--", archive});
        QVERIFY(listing.waitForFinished(10000));
        QCOMPARE(listing.exitCode(), 0);
        const QByteArray contents = NormalizeTechnicalListing(listing.readAllStandardOutput());
        const QString listingText =
            QString::fromUtf8(contents).normalized(QString::NormalizationForm_C);
        const QString expected = logical.normalized(QString::NormalizationForm_C);
        QVERIFY(listingText.contains("Path = " + expected + "/внутри/файл.txt"));
        QVERIFY(listingText.contains("Path = " + expected + "/empty"));
        if (mode != int(NWildcard::k_AbsPath)) {
            const QString extracted = fixture.filePath("extracted");
            QVERIFY(QDir().mkpath(extracted));
            QVERIFY(runEngine(extracted, {"x", "-y", "--", archive}));
            QCOMPARE(readFile(QDir(extracted).filePath(logical + "/внутри/файл.txt")),
                     QByteArray("nested"));
            QVERIFY(QFileInfo(QDir(extracted).filePath(logical + "/empty")).isDir());
        }
    }
    void createArchivePathModes_data() {
        QTest::addColumn<QString>("format");
        QTest::addColumn<int>("mode");
        for (const QString &format : {QStringLiteral("zip"), QStringLiteral("7z")})
            for (const auto mode :
                 {NWildcard::k_RelatPath, NWildcard::k_FullPath, NWildcard::k_AbsPath})
                QTest::newRow(qPrintable(format + "-" + QString::number(int(mode))))
                    << format << int(mode);
    }
    void createArchivePathModes() {
        QFETCH(QString, format);
        QFETCH(int, mode);
        QTemporaryDir fixture;
        QVERIFY(fixture.isValid());
        const QString source = fixture.filePath("first/selected.txt");
        const QString second = fixture.filePath("second/other.txt");
        const QString archive = fixture.filePath("created." + format);
        QVERIFY(writeFile(source, "first bytes"));
        QVERIFY(writeFile(second, "second bytes"));
        CompressionOptions options;
        options.PathMode = static_cast<NWildcard::ECensorPathMode>(mode);
        ArchiveBackend backend(nullptr, enginePath());
        QSignalSpy finished(&backend, &ArchiveBackend::finished);
        const QStringList sources =
            mode == int(NWildcard::k_RelatPath) ? QStringList{source, second} : QStringList{source};
        backend.create(archive, sources, {}, false, options);
        QTRY_COMPARE_WITH_TIMEOUT(finished.count(), 1, 30000);
        QVERIFY2(finished.takeFirst().first().toBool(), "path-mode create failed");
        QProcess listing;
        listing.start(enginePath(), {"l", "-slt", "-sccUTF-8", "--", archive});
        QVERIFY(listing.waitForFinished(10000));
        QCOMPARE(listing.exitCode(), 0);
        const QByteArray output = NormalizeTechnicalListing(listing.readAllStandardOutput());
        QString logical = QStringLiteral("selected.txt");
        if (mode == int(NWildcard::k_FullPath)) {
#ifdef _WIN32
            logical = source.mid(QDir::rootPath().length());
#else
            logical = source.mid(1);
#endif
        } else if (mode == int(NWildcard::k_AbsPath))
            logical = source;
        QVERIFY(output.contains("Path = " + logical.toUtf8()));
        if (mode == int(NWildcard::k_RelatPath)) {
            QVERIFY(output.contains("Path = other.txt"));
            const QString extracted = fixture.filePath("extracted");
            QVERIFY(QDir().mkpath(extracted));
            QVERIFY(runEngine(extracted, {"x", "-y", "--", archive}));
            QCOMPARE(readFile(QDir(extracted).filePath("selected.txt")), QByteArray("first bytes"));
            QCOMPARE(readFile(QDir(extracted).filePath("other.txt")), QByteArray("second bytes"));
        }
    }
    void multiRootCreateCancelKeepsSourcesAndNoTarget() {
        QTemporaryDir fixture;
        QVERIFY(fixture.isValid());
        const QString first = fixture.filePath("first/one.txt");
        const QString second = fixture.filePath("second/two.bin");
        const QString archive = fixture.filePath("cancelled.zip");
        QVERIFY(writeFile(first, "first bytes"));
        QVERIFY(writeFile(second, QByteArray(4 * 1024 * 1024, 'x')));
        ArchiveBackend backend(nullptr, enginePath());
        QSignalSpy finished(&backend, &ArchiveBackend::finished);
        int stages = 0;
        connect(&backend, &ArchiveBackend::engineStarted, this, [&] {
            ++stages;
            if (stages == 2)
                backend.cancel();
        });
        backend.create(archive, {first, second});
        QTRY_COMPARE_WITH_TIMEOUT(finished.count(), 1, 30000);
        QCOMPARE(stages, 2);
        QVERIFY(!finished.takeFirst().first().toBool());
        QCOMPARE(backend.lastResult(), E_ABORT);
        QVERIFY(!QFileInfo::exists(archive));
        QCOMPARE(readFile(first), QByteArray("first bytes"));
        QCOMPARE(readFile(second), QByteArray(4 * 1024 * 1024, 'x'));
    }
    void absolutePlanRejectsTraversalAlias() {
        QTemporaryDir fixture;
        QVERIFY(fixture.isValid());
        const QString root = fixture.filePath("source");
        const QString archive = fixture.filePath("original.zip");
        const QString output = fixture.filePath("updated.zip");
        QVERIFY(writeFile(QDir(root).filePath("keep.txt"), "keep"));
        QVERIFY(runEngine(root, {"a", "-tzip", "--", archive, "keep.txt"}));
        const QByteArray original = readFile(archive);
        ArchiveBackend::SourceUpdatePlan plan;
        plan.files = {"keep.txt"};
        plan.fileRoot = root;
        plan.renames = {{"keep.txt", "/tmp/../outside.txt"}};
        plan.allowAbsolute = true;
        ArchiveBackend backend(nullptr, enginePath());
        QSignalSpy finished(&backend, &ArchiveBackend::finished);
        backend.updateSourcePlan(archive, plan, output, "zip", {});
        QCOMPARE(finished.count(), 1);
        QVERIFY(!finished.takeFirst().first().toBool());
        QVERIFY(!QFileInfo::exists(output));
        QCOMPARE(readFile(archive), original);
    }
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
    void sourcePlanDeletesReplacesAndAddsFiles() {
        QTemporaryDir fixture;
        QVERIFY(fixture.isValid());
        const QString oldRoot = fixture.filePath("old");
        const QString fileRoot = fixture.filePath("staged");
        const QString archive = fixture.filePath("original.zip");
        const QString output = fixture.filePath("updated.zip");
        QVERIFY(writeFile(QDir(oldRoot).filePath("keep.txt"), "keep"));
        QVERIFY(writeFile(QDir(oldRoot).filePath("replace.txt"), "old"));
        QVERIFY(writeFile(QDir(oldRoot).filePath("removed.txt"), "remove"));
        QVERIFY(runEngine(oldRoot,
                          {"a", "-tzip", "--", archive, "keep.txt", "replace.txt", "removed.txt"}));
        const QByteArray original = readFile(archive);
        QVERIFY(writeFile(QDir(fileRoot).filePath("replace.txt"), "new"));
        QVERIFY(writeFile(QDir(fileRoot).filePath(QStringLiteral("новый файл.txt")), "added"));
        ArchiveBackend::SourceUpdatePlan plan;
        plan.removed = {"replace.txt", "removed.txt"};
        plan.files = {"replace.txt", QStringLiteral("новый файл.txt")};
        plan.fileRoot = fileRoot;
        ArchiveBackend backend(nullptr, enginePath());
        CBackendWorker worker(backend);
        worker.Start = [&] { backend.updateSourcePlan(archive, plan, output, "zip", {}); };
        ArchiveProgressTransport transport(worker, backend);
        QCOMPARE(worker.Create(L"Update", nullptr), S_OK);
        QCOMPARE(worker.Result, S_OK);
        QCOMPARE(readFile(archive), original);
        const QString extracted = fixture.filePath("extracted");
        QVERIFY(QDir().mkpath(extracted));
        QVERIFY(runEngine(extracted, {"x", "-y", "--", output}));
        QCOMPARE(readFile(QDir(extracted).filePath("keep.txt")), QByteArray("keep"));
        QCOMPARE(readFile(QDir(extracted).filePath("replace.txt")), QByteArray("new"));
        const QString addedPath = QDir(extracted).filePath(QStringLiteral("новый файл.txt"));
        QCOMPARE(readFile(addedPath), QByteArray("added"));
        QVERIFY(!QFileInfo::exists(QDir(extracted).filePath("removed.txt")));
    }
    void sourcePlanStagesAbsoluteNameAndDeletesIt() {
        QTemporaryDir fixture;
        QVERIFY(fixture.isValid());
        const QString originalRoot = fixture.filePath("old");
        const QString stageRoot = fixture.filePath("stage");
        const QString original = fixture.filePath("original.zip");
        const QString updated = fixture.filePath("updated.zip");
        const QString removed = fixture.filePath("removed.zip");
        const QString alias = QStringLiteral("absolute-item-0");
        const QString logical = fixture.filePath("absolute.txt");
        QVERIFY(writeFile(QDir(originalRoot).filePath("keep.txt"), "keep"));
        QVERIFY(runEngine(originalRoot, {"a", "-tzip", "--", original, "keep.txt"}));
        const QByteArray originalBytes = readFile(original);
        QVERIFY(writeFile(QDir(stageRoot).filePath(alias), "absolute bytes"));
        ArchiveBackend::SourceUpdatePlan add;
        add.files = {alias};
        add.fileRoot = stageRoot;
        add.allowAbsolute = true;
        add.renames = {{alias, logical}};
        ArchiveBackend backend(nullptr, enginePath());
        QSignalSpy finished(&backend, &ArchiveBackend::finished);
        backend.updateSourcePlan(original, add, updated, "zip", {});
        QTRY_COMPARE_WITH_TIMEOUT(finished.count(), 1, 30000);
        QVERIFY2(finished.takeFirst().first().toBool(), "absolute rename stage failed");
        QCOMPARE(readFile(original), originalBytes);
        QProcess listing;
        listing.start(enginePath(), {"l", "-slt", "-sccUTF-8", "--", updated});
        QVERIFY(listing.waitForFinished(10000));
        QCOMPARE(listing.exitCode(), 0);
        QVERIFY(NormalizeTechnicalListing(listing.readAllStandardOutput())
                    .contains("Path = " + logical.toUtf8()));
        ArchiveBackend::SourceUpdatePlan drop;
        drop.removed = {logical};
        drop.allowAbsolute = true;
        backend.updateSourcePlan(updated, drop, removed, "zip", {});
        QTRY_COMPARE_WITH_TIMEOUT(finished.count(), 1, 30000);
        QVERIFY2(finished.takeFirst().first().toBool(), "absolute delete stage failed");
        QProcess after;
        after.start(enginePath(), {"l", "-slt", "-sccUTF-8", "--", removed});
        QVERIFY(after.waitForFinished(10000));
        QCOMPARE(after.exitCode(), 0);
        const QByteArray contents = NormalizeTechnicalListing(after.readAllStandardOutput());
        QVERIFY(!contents.contains("Path = " + logical.toUtf8()));
        QVERIFY(contents.contains("Path = keep.txt"));
    }
    void sourcePlanRejectsInvalidOptionsAndRollsBackOnCancel() {
        QTemporaryDir fixture;
        QVERIFY(fixture.isValid());
        const QString sourceRoot = fixture.filePath("source");
        const QString archive = fixture.filePath("original.zip");
        const QString output = fixture.filePath("updated.zip");
        QVERIFY(
            writeFile(QDir(sourceRoot).filePath("input.bin"), QByteArray(4 * 1024 * 1024, 'x')));
        QVERIFY(runEngine(sourceRoot, {"a", "-tzip", "--", archive, "input.bin"}));
        const QByteArray original = readFile(archive);
        ArchiveBackend::SourceUpdatePlan plan;
        plan.files = {"input.bin"};
        plan.removed = {"input.bin"};
        plan.fileRoot = sourceRoot;
        CompressionOptions invalid;
        invalid.Level = 99;
        ArchiveBackend rejected(nullptr, enginePath());
        QSignalSpy rejectedFinished(&rejected, &ArchiveBackend::finished);
        rejected.updateSourcePlan(archive, plan, output, "zip", {}, invalid);
        QCOMPARE(rejectedFinished.count(), 1);
        QVERIFY(!rejectedFinished.takeFirst().first().toBool());
        QVERIFY(!QFile::exists(output));
        QCOMPARE(readFile(archive), original);
        rejected.updateSourcePlan(archive, plan, output, "zip", "password");
        QCOMPARE(rejectedFinished.count(), 1);
        QVERIFY(!rejectedFinished.takeFirst().first().toBool());
        QVERIFY(!QFile::exists(output));
        QCOMPARE(readFile(archive), original);

        ArchiveBackend cancelled(nullptr, enginePath());
        QSignalSpy cancelledFinished(&cancelled, &ArchiveBackend::finished);
        connect(&cancelled, &ArchiveBackend::engineStarted, &cancelled, &ArchiveBackend::cancel);
        cancelled.updateSourcePlan(archive, plan, output, "zip", {});
        QTRY_COMPARE_WITH_TIMEOUT(cancelledFinished.count(), 1, 10000);
        QVERIFY(!cancelledFinished.takeFirst().first().toBool());
        QCOMPARE(cancelled.lastResult(), E_ABORT);
        QVERIFY(!cancelled.busy());
        QVERIFY(!QFile::exists(output));
        QCOMPARE(readFile(archive), original);
    }
    void nativeUpdateModesFollowSourcePairActions_data() {
        QTest::addColumn<QString>("format");
        QTest::addColumn<int>("mode");
        QTest::addColumn<QByteArray>("olderBytes");
        QTest::addColumn<bool>("newPresent");
        QTest::addColumn<bool>("archiveOnlyPresent");
        QTest::newRow("zip-add") << QString("zip") << int(NCompressDialog::NUpdateMode::kAdd)
                                 << QByteArray("disk older") << true << true;
        QTest::newRow("zip-update") << QString("zip") << int(NCompressDialog::NUpdateMode::kUpdate)
                                    << QByteArray("archive older") << true << true;
        QTest::newRow("zip-freshen") << QString("zip") << int(NCompressDialog::NUpdateMode::kFresh)
                                     << QByteArray("archive older") << false << true;
        QTest::newRow("zip-sync") << QString("zip") << int(NCompressDialog::NUpdateMode::kSync)
                                  << QByteArray("archive older") << true << false;
        QTest::newRow("7z-add") << QString("7z") << int(NCompressDialog::NUpdateMode::kAdd)
                                << QByteArray("disk older") << true << true;
        QTest::newRow("7z-update") << QString("7z") << int(NCompressDialog::NUpdateMode::kUpdate)
                                   << QByteArray("archive older") << true << true;
        QTest::newRow("7z-freshen") << QString("7z") << int(NCompressDialog::NUpdateMode::kFresh)
                                    << QByteArray("archive older") << false << true;
        QTest::newRow("7z-sync") << QString("7z") << int(NCompressDialog::NUpdateMode::kSync)
                                 << QByteArray("archive older") << true << false;
    }
    void nativeUpdateModesFollowSourcePairActions() {
        QFETCH(QString, format);
        QFETCH(int, mode);
        QFETCH(QByteArray, olderBytes);
        QFETCH(bool, newPresent);
        QFETCH(bool, archiveOnlyPresent);
        QTemporaryDir fixture;
        QVERIFY(fixture.isValid());
        const QString oldRoot = fixture.filePath("original");
        const QString diskRoot = fixture.filePath("disk");
        const QString archive = fixture.filePath("modes." + format);
        const QStringList archiveNames = {"newer.txt", "older.txt", "archive-only.txt"};
        QVERIFY(writeFile(QDir(oldRoot).filePath("newer.txt"), "archive newer"));
        QVERIFY(writeFile(QDir(oldRoot).filePath("older.txt"), "archive older"));
        QVERIFY(writeFile(QDir(oldRoot).filePath("archive-only.txt"), "archive only"));
        const QDateTime baseline = QDateTime::fromSecsSinceEpoch(1700000000, Qt::UTC);
        for (const QString &name : archiveNames) {
            QFile file(QDir(oldRoot).filePath(name));
            QVERIFY(file.open(QIODevice::ReadWrite));
            QVERIFY(file.setFileTime(baseline, QFileDevice::FileModificationTime));
        }
        QVERIFY(runEngine(oldRoot, {"a", "-t" + format, "--", archive, "newer.txt", "older.txt",
                                    "archive-only.txt"}));
        const QByteArray original = readFile(archive);
        QVERIFY(writeFile(QDir(diskRoot).filePath("newer.txt"), "disk newer"));
        QVERIFY(writeFile(QDir(diskRoot).filePath("older.txt"), "disk older"));
        QVERIFY(writeFile(QDir(diskRoot).filePath("new-file.txt"), "disk new"));
        for (const auto &entry : {qMakePair(QStringLiteral("newer.txt"), 86400),
                                  qMakePair(QStringLiteral("older.txt"), -86400),
                                  qMakePair(QStringLiteral("new-file.txt"), 86400)}) {
            QFile file(QDir(diskRoot).filePath(entry.first));
            QVERIFY(file.open(QIODevice::ReadWrite));
            QVERIFY(file.setFileTime(baseline.addSecs(entry.second),
                                     QFileDevice::FileModificationTime));
        }
        ArchiveBackend backend(nullptr, enginePath());
        CNativeAddWorker worker(backend, archive, QDir(diskRoot).filePath("newer.txt"), format);
        worker.RootFolder = true;
        worker.ArchivePaths = archiveNames;
        worker.ArchiveMTimeText = baseline.toLocalTime().toString("yyyy-MM-dd HH:mm:ss");
        worker.Sources = {QDir(diskRoot).filePath("newer.txt"),
                          QDir(diskRoot).filePath("older.txt"),
                          QDir(diskRoot).filePath("new-file.txt")};
        worker.ActionSet = NCompressDialog::FindUpdateActionSet(
            static_cast<NCompressDialog::NUpdateMode::EEnum>(mode));
        QVERIFY(worker.ActionSet);
        auto transport = worker.CreateProgressTransport();
        QCOMPARE(worker.Create(L"Update", nullptr), S_OK);
        QCOMPARE(worker.Result, S_OK);
        QVERIFY(readFile(archive) != original);
        const QString extracted = fixture.filePath("extracted");
        QVERIFY(QDir().mkpath(extracted));
        QVERIFY(runEngine(extracted, {"x", "-y", "--", archive}));
        QCOMPARE(readFile(QDir(extracted).filePath("newer.txt")), QByteArray("disk newer"));
        QCOMPARE(readFile(QDir(extracted).filePath("older.txt")), olderBytes);
        QCOMPARE(QFileInfo::exists(QDir(extracted).filePath("new-file.txt")), newPresent);
        QCOMPARE(QFileInfo::exists(QDir(extracted).filePath("archive-only.txt")),
                 archiveOnlyPresent);
    }
    void nativeCopyFromAddsToCurrentFolder() {
        QTemporaryDir fixture;
        QVERIFY(fixture.isValid());
        const QString root = fixture.filePath("original");
        const QString archive = fixture.filePath("original.zip");
        const QString source = fixture.filePath(QStringLiteral("новый файл.bin"));
        QVERIFY(writeFile(QDir(root).filePath("inside/keep.bin"), "keep"));
        QVERIFY(runEngine(root, {"a", "-tzip", "--", archive, "inside/keep.bin"}));
        QVERIFY(writeFile(source, "new bytes"));
        QFile timestamped(source);
        QVERIFY(timestamped.open(QIODevice::ReadWrite));
        const QDateTime modified = QDateTime::fromSecsSinceEpoch(1700000000, Qt::UTC);
        QVERIFY(timestamped.setFileTime(modified, QFileDevice::FileModificationTime));
        timestamped.close();
        ArchiveBackend backend(nullptr, enginePath());
        CNativeAddWorker worker(backend, archive, source, "zip");
        auto transport = worker.CreateProgressTransport();
        QVERIFY(transport);
        QCOMPARE(worker.Create(L"Copy into archive", nullptr), S_OK);
        QCOMPARE(worker.Result, S_OK);
        const QString extracted = fixture.filePath("extracted");
        QVERIFY(runEngine(fixture.path(), {"x", "-y", "-o" + extracted, "--", archive}));
        QCOMPARE(readFile(QDir(extracted).filePath("inside/keep.bin")), QByteArray("keep"));
        QCOMPARE(readFile(QDir(extracted).filePath(QStringLiteral("inside/новый файл.bin"))),
                 QByteArray("new bytes"));
        const qint64 difference =
            qAbs(QFileInfo(QDir(extracted).filePath(QStringLiteral("inside/новый файл.bin")))
                     .lastModified()
                     .toSecsSinceEpoch() -
                 modified.toSecsSinceEpoch());
        QVERIFY(difference <= 2);
        QCOMPARE(readFile(source), QByteArray("new bytes"));
    }
    void nativeCopyFromAddsDirectoryTree() {
        QTemporaryDir fixture;
        QVERIFY(fixture.isValid());
        const QString root = fixture.filePath("original");
        const QString archive = fixture.filePath("original.zip");
        const QString source = fixture.filePath(QStringLiteral("папка с пробелом"));
        QVERIFY(writeFile(QDir(root).filePath("inside/keep.bin"), "keep"));
        QVERIFY(runEngine(root, {"a", "-tzip", "--", archive, "inside/keep.bin"}));
        QVERIFY(
            writeFile(QDir(source).filePath(QStringLiteral("вложенная/файл.txt")), "tree bytes"));
        QVERIFY(QDir().mkpath(QDir(source).filePath("empty")));
        ArchiveBackend backend(nullptr, enginePath());
        CNativeAddWorker worker(backend, archive, source, "zip");
        auto transport = worker.CreateProgressTransport();
        QVERIFY(transport);
        QCOMPARE(worker.Create(L"Copy directory into archive", nullptr), S_OK);
        QCOMPARE(worker.Result, S_OK);
        const QString extracted = fixture.filePath("extracted");
        QVERIFY(runEngine(fixture.path(), {"x", "-y", "-o" + extracted, "--", archive}));
        QCOMPARE(readFile(QDir(extracted).filePath(
                     QStringLiteral("inside/папка с пробелом/вложенная/файл.txt"))),
                 QByteArray("tree bytes"));
        QVERIFY(QFileInfo(QDir(extracted).filePath(QStringLiteral("inside/папка с пробелом/empty")))
                    .isDir());
    }
    void nativeCopyFromAddsMultipleSelectedFiles() {
        QTemporaryDir fixture;
        QVERIFY(fixture.isValid());
        const QString root = fixture.filePath("original");
        const QString archive = fixture.filePath("original.zip");
        const QString first = fixture.filePath(QStringLiteral("первый.bin"));
        const QString empty = fixture.filePath("empty file.bin");
        QVERIFY(writeFile(QDir(root).filePath("inside/keep.bin"), "keep"));
        QVERIFY(runEngine(root, {"a", "-tzip", "--", archive, "inside/keep.bin"}));
        QVERIFY(writeFile(first, "first bytes"));
        QVERIFY(writeFile(empty, {}));
        ArchiveBackend backend(nullptr, enginePath());
        CNativeAddWorker worker(backend, archive, first, "zip");
        worker.Sources = {first, empty};
        auto transport = worker.CreateProgressTransport();
        QVERIFY(transport);
        QCOMPARE(worker.Create(L"Copy selected files", nullptr), S_OK);
        QCOMPARE(worker.Result, S_OK);
        const QString extracted = fixture.filePath("extracted");
        QVERIFY(runEngine(fixture.path(), {"x", "-y", "-o" + extracted, "--", archive}));
        QCOMPARE(readFile(QDir(extracted).filePath(QStringLiteral("inside/первый.bin"))),
                 QByteArray("first bytes"));
        QVERIFY(QFileInfo(QDir(extracted).filePath("inside/empty file.bin")).isFile());
        QCOMPARE(readFile(QDir(extracted).filePath("inside/empty file.bin")), QByteArray());
    }
    void nativeMovePublishesBeforeDeletingDiskSource() {
        QTemporaryDir fixture;
        QVERIFY(fixture.isValid());
        const QString root = fixture.filePath("original");
        const QString archive = fixture.filePath("original.zip");
        const QString source = fixture.filePath(QStringLiteral("переместить.txt"));
        QVERIFY(writeFile(QDir(root).filePath("inside/keep.bin"), "keep"));
        QVERIFY(runEngine(root, {"a", "-tzip", "--", archive, "inside/keep.bin"}));
        QVERIFY(writeFile(source, "move bytes"));
        ArchiveBackend backend(nullptr, enginePath());
        CNativeAddWorker worker(backend, archive, source, "zip");
        worker.MoveMode = true;
        auto transport = worker.CreateProgressTransport();
        QVERIFY(transport);
        QCOMPARE(worker.Create(L"Move into archive", nullptr), S_OK);
        QCOMPARE(worker.Result, S_OK);
        QVERIFY(!QFileInfo::exists(source));
        const QString extracted = fixture.filePath("extracted");
        QVERIFY(runEngine(fixture.path(), {"x", "-y", "-o" + extracted, "--", archive}));
        QCOMPARE(readFile(QDir(extracted).filePath(QStringLiteral("inside/переместить.txt"))),
                 QByteArray("move bytes"));
    }
    void cancelledMoveLeavesDiskSourceAndArchiveIntact() {
        QTemporaryDir fixture;
        QVERIFY(fixture.isValid());
        const QString root = fixture.filePath("original");
        const QString archive = fixture.filePath("original.zip");
        const QString source = fixture.filePath("large move.bin");
        QVERIFY(writeFile(QDir(root).filePath("inside/keep.bin"), "keep"));
        QVERIFY(runEngine(root, {"a", "-tzip", "--", archive, "inside/keep.bin"}));
        const QByteArray before = readFile(archive);
        QVERIFY(writeFile(source, QByteArray(8 * 1024 * 1024, 'x')));
        ArchiveBackend backend(nullptr, enginePath());
        CNativeAddWorker worker(backend, archive, source, "zip");
        worker.MoveMode = true;
        auto transport = worker.CreateProgressTransport();
        QVERIFY(transport);
        QSignalSpy started(&backend, &ArchiveBackend::engineStarted);
        connect(&backend, &ArchiveBackend::engineStarted, &backend,
                [&worker] { worker.Sync.Set_Stopped(true); });
        QCOMPARE(worker.Create(L"Cancel archive move", nullptr), S_OK);
        QCOMPARE(worker.Result, E_ABORT);
        QCOMPARE(started.count(), 1);
        QCOMPARE(readFile(archive), before);
        QCOMPARE(QFileInfo(source).size(), qint64(8 * 1024 * 1024));
    }
    void protectedTopArchiveAcceptsCopyFrom() {
        QTemporaryDir fixture;
        QVERIFY(fixture.isValid());
        const QString root = fixture.filePath("original");
        const QString archive = fixture.filePath("original.7z");
        const QString source = fixture.filePath(QStringLiteral("секретное имя.txt"));
        QVERIFY(writeFile(QDir(root).filePath("inside/keep.bin"), "keep"));
        QVERIFY(runEngine(root,
                          {"a", "-t7z", "-psecret", "-mhe=on", "--", archive, "inside/keep.bin"}));
        QVERIFY(writeFile(source, "protected bytes"));
        ArchiveBackend backend(nullptr, enginePath());
        CNativeAddWorker worker(backend, archive, source, "7z");
        worker.Password = "secret";
        auto transport = worker.CreateProgressTransport();
        QVERIFY(transport);
        QCOMPARE(worker.Create(L"Copy into protected archive", nullptr), S_OK);
        QCOMPARE(worker.Result, S_OK);
        const QString extracted = fixture.filePath("extracted");
        QVERIFY(
            runEngine(fixture.path(), {"x", "-y", "-psecret", "-o" + extracted, "--", archive}));
        QCOMPARE(readFile(QDir(extracted).filePath(QStringLiteral("inside/секретное имя.txt"))),
                 QByteArray("protected bytes"));
        QCOMPARE(readFile(QDir(extracted).filePath("inside/keep.bin")), QByteArray("keep"));
    }
    void sourceAddRejectsUnsafePathWithoutChangingArchive() {
        QTemporaryDir fixture;
        QVERIFY(fixture.isValid());
        const QString root = fixture.filePath("original");
        const QString staged = fixture.filePath("staged");
        const QString archive = fixture.filePath("original.zip");
        const QString output = fixture.filePath("updated.zip");
        QVERIFY(writeFile(QDir(root).filePath("inside/keep.bin"), "keep"));
        QVERIFY(runEngine(root, {"a", "-tzip", "--", archive, "inside/keep.bin"}));
        QVERIFY(writeFile(QDir(staged).filePath("inside/safe.bin"), "safe"));
        const QByteArray before = readFile(archive);
        ArchiveBackend backend(nullptr, enginePath());
        QSignalSpy finished(&backend, &ArchiveBackend::finished);
        backend.updateSourceAdd(archive, {"../outside.bin"}, staged, output, "zip", {});
        QCOMPARE(finished.count(), 1);
        QVERIFY(!finished[0][0].toBool());
        QVERIFY(!QFileInfo::exists(output));
        QCOMPARE(readFile(archive), before);
        const QString outside = fixture.filePath("outside.zip");
        if (QFile::link(outside, output)) {
            backend.updateSourceAdd(archive, {"inside/safe.bin"}, staged, output, "zip", {});
            QCOMPARE(finished.count(), 2);
            QVERIFY(!finished[1][0].toBool());
            QVERIFY(!QFileInfo::exists(outside));
            QCOMPARE(readFile(archive), before);
        }
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
    void sourceMutationProducesSeparateArchive_data() {
        QTest::addColumn<int>("operation");
        QTest::newRow("delete") << int(ArchiveBackend::SourceMutation::Delete);
        QTest::newRow("rename") << int(ArchiveBackend::SourceMutation::Rename);
        QTest::newRow("create-folder") << int(ArchiveBackend::SourceMutation::CreateFolder);
    }
    void sourceMutationProducesSeparateArchive() {
        QFETCH(int, operation);
        QTemporaryDir fixture;
        QVERIFY(fixture.isValid());
        const QString root = fixture.filePath("source");
        const QString archive = fixture.filePath("original.zip");
        const QString output = fixture.filePath("updated.zip");
        const QString entry = QStringLiteral("inside/имя с пробелом.bin");
        QVERIFY(writeFile(QDir(root).filePath(entry), "payload"));
        QVERIFY(writeFile(QDir(root).filePath("keep.bin"), "keep"));
        QVERIFY(runEngine(root, {"a", "-tzip", "--", archive, "inside", "keep.bin"}));
        const QByteArray original = readFile(archive);
        QStringList paths;
        QString workingDirectory;
        const auto kind = ArchiveBackend::SourceMutation(operation);
        if (kind == ArchiveBackend::SourceMutation::Delete)
            paths = {entry};
        else if (kind == ArchiveBackend::SourceMutation::Rename)
            paths = {entry, QStringLiteral("inside/новое имя.bin")};
        else {
            paths = {QStringLiteral("inside/новая папка")};
            workingDirectory = fixture.filePath("new-folder");
            QVERIFY(QDir().mkpath(QDir(workingDirectory).filePath(paths.first())));
        }
        ArchiveBackend backend(nullptr, enginePath());
        QSignalSpy finished(&backend, &ArchiveBackend::finished);
        backend.updateSourceMutation(archive, output, "zip", kind, paths, workingDirectory);
        QTRY_COMPARE_WITH_TIMEOUT(finished.size(), 1, 10000);
        QVERIFY2(finished[0][0].toBool(), qPrintable(finished[0][1].toString()));
        QCOMPARE(readFile(archive), original);
        const QString extracted = fixture.filePath("extracted");
        QVERIFY(runEngine(fixture.path(), {"x", "-y", "-o" + extracted, "--", output}));
        QCOMPARE(readFile(QDir(extracted).filePath("keep.bin")), QByteArray("keep"));
        if (kind == ArchiveBackend::SourceMutation::Delete)
            QVERIFY(!QFile::exists(QDir(extracted).filePath(entry)));
        else if (kind == ArchiveBackend::SourceMutation::Rename) {
            QVERIFY(!QFile::exists(QDir(extracted).filePath(entry)));
            QCOMPARE(readFile(QDir(extracted).filePath(paths[1])), QByteArray("payload"));
        } else {
            QCOMPARE(readFile(QDir(extracted).filePath(entry)), QByteArray("payload"));
            QVERIFY(QFileInfo(QDir(extracted).filePath(paths[0])).isDir());
        }
    }
    void sourceFolderMutationPublishesAndReopens_data() {
        QTest::addColumn<QString>("format");
        QTest::addColumn<int>("operation");
        for (const QString &format : {QStringLiteral("zip"), QStringLiteral("7z")})
            for (int kind = 0; kind < 3; ++kind)
                QTest::newRow(qPrintable(format + QString::number(kind))) << format << kind;
    }
    void sourceFolderMutationPublishesAndReopens() {
        QFETCH(QString, format);
        QFETCH(int, operation);
        QTemporaryDir fixture;
        QVERIFY(fixture.isValid());
        const QString root = fixture.filePath("source");
        const QString archive = fixture.filePath("original." + format);
        const QString entry = QStringLiteral("inside/имя с пробелом.bin");
        QVERIFY(writeFile(QDir(root).filePath(entry), "payload"));
        QVERIFY(writeFile(QDir(root).filePath("keep.bin"), "keep"));
        const QDateTime originalTime = QDateTime::fromSecsSinceEpoch(1700000000, Qt::UTC);
        QFile keep(QDir(root).filePath("keep.bin"));
        QVERIFY(keep.open(QIODevice::ReadWrite));
        QVERIFY(keep.setFileTime(originalTime, QFileDevice::FileModificationTime));
        keep.close();
        QVERIFY(runEngine(root, {"a", "-t" + format, "--", archive, "inside", "keep.bin"}));
        ArchiveBackend backend(nullptr, enginePath());
        CNativeMutationWorker worker(backend, archive, format);
        worker.Operation = ArchiveBackend::SourceMutation(operation);
        auto transport = worker.CreateProgressTransport();
        QVERIFY(transport);
        QCOMPARE(worker.Create(L"Source folder mutation", nullptr), S_OK);
        QCOMPARE(worker.Result, S_OK);
        QVERIFY(!backend.busy());
        const QString extracted = fixture.filePath("extracted");
        QVERIFY(runEngine(fixture.path(), {"x", "-y", "-o" + extracted, "--", archive}));
        QCOMPARE(readFile(QDir(extracted).filePath("keep.bin")), QByteArray("keep"));
        QCOMPARE(QFileInfo(QDir(extracted).filePath("keep.bin")).lastModified().toSecsSinceEpoch(),
                 originalTime.toSecsSinceEpoch());
        if (worker.Operation == ArchiveBackend::SourceMutation::Delete)
            QVERIFY(!QFile::exists(QDir(extracted).filePath(entry)));
        else if (worker.Operation == ArchiveBackend::SourceMutation::Rename) {
            QVERIFY(!QFile::exists(QDir(extracted).filePath(entry)));
            QCOMPARE(readFile(QDir(extracted).filePath("inside/новое имя.bin")),
                     QByteArray("payload"));
        } else {
            QCOMPARE(readFile(QDir(extracted).filePath(entry)), QByteArray("payload"));
            QVERIFY(QFileInfo(QDir(extracted).filePath("inside/новая папка")).isDir());
        }
    }
    void rejectedOrCancelledSourceMutationKeepsOriginal() {
        QTemporaryDir fixture;
        QVERIFY(fixture.isValid());
        const QString root = fixture.filePath("source");
        const QString archive = fixture.filePath("original.zip");
        const QString output = fixture.filePath("updated.zip");
        const QString entry = QStringLiteral("inside/имя с пробелом.bin");
        QVERIFY(writeFile(QDir(root).filePath(entry), QByteArray(4 * 1024 * 1024, 'x')));
        QVERIFY(runEngine(root, {"a", "-tzip", "--", archive, "inside"}));
        const QByteArray original = readFile(archive);
        ArchiveBackend backend(nullptr, enginePath());
        QSignalSpy finished(&backend, &ArchiveBackend::finished);
        backend.updateSourceMutation(archive, output, "zip", ArchiveBackend::SourceMutation::Rename,
                                     {entry, QStringLiteral("../escape")}, {});
        QCOMPARE(finished.size(), 1);
        QVERIFY(!finished[0][0].toBool());
        QVERIFY(!QFile::exists(output));
        QCOMPARE(readFile(archive), original);
        connect(&backend, &ArchiveBackend::engineStarted, &backend,
                [&backend] { backend.cancel(); });
        backend.updateSourceMutation(archive, output, "zip", ArchiveBackend::SourceMutation::Delete,
                                     {entry}, {});
        QTRY_COMPARE_WITH_TIMEOUT(finished.size(), 2, 10000);
        QCOMPARE(backend.lastResult(), E_ABORT);
        QVERIFY(!QFile::exists(output));
        QCOMPARE(readFile(archive), original);
    }
    void implicitFolderRenameAndDelete_data() {
        QTest::addColumn<QString>("format");
        QTest::addColumn<int>("operation");
        for (const QString &format : {QStringLiteral("zip"), QStringLiteral("7z")})
            for (int kind = 0; kind < 2; ++kind)
                QTest::newRow(qPrintable(format + QString::number(kind))) << format << kind;
    }
    void implicitFolderRenameAndDelete() {
        QFETCH(QString, format);
        QFETCH(int, operation);
        QTemporaryDir fixture;
        QVERIFY(fixture.isValid());
        const QString root = fixture.filePath("source");
        const QString archive = fixture.filePath("original." + format);
        const QString entry = QStringLiteral("inside/имя с пробелом.bin");
        QVERIFY(writeFile(QDir(root).filePath(entry), "payload"));
        QVERIFY(writeFile(QDir(root).filePath("keep.bin"), "keep"));
        QVERIFY(runEngine(root, {"a", "-t" + format, "--", archive, entry, "keep.bin"}));
        ArchiveBackend backend(nullptr, enginePath());
        CNativeMutationWorker worker(backend, archive, format);
        worker.RootFolder = true;
        worker.Operation = operation == 0 ? ArchiveBackend::SourceMutation::Delete
                                          : ArchiveBackend::SourceMutation::Rename;
        auto transport = worker.CreateProgressTransport();
        QVERIFY(transport);
        QCOMPARE(worker.Create(L"Source folder rename/delete", nullptr), S_OK);
        QCOMPARE(worker.Result, S_OK);
        const QString extracted = fixture.filePath("extracted");
        QVERIFY(runEngine(fixture.path(), {"x", "-y", "-o" + extracted, "--", archive}));
        QCOMPARE(readFile(QDir(extracted).filePath("keep.bin")), QByteArray("keep"));
        QVERIFY(!QFile::exists(QDir(extracted).filePath(entry)));
        if (operation == 1)
            QCOMPARE(readFile(QDir(extracted).filePath(
                         QStringLiteral("новое имя.bin/имя с пробелом.bin"))),
                     QByteArray("payload"));
    }
    void cancelledSourceDeleteKeepsArchive() {
        QTemporaryDir fixture;
        QVERIFY(fixture.isValid());
        const QString root = fixture.filePath("source");
        const QString archive = fixture.filePath("original.zip");
        const QString entry = QStringLiteral("inside/имя с пробелом.bin");
        QVERIFY(writeFile(QDir(root).filePath(entry), QByteArray(4 * 1024 * 1024, 'x')));
        QVERIFY(writeFile(QDir(root).filePath("keep.bin"), "keep"));
        QVERIFY(runEngine(root, {"a", "-tzip", "--", archive, entry, "keep.bin"}));
        const QByteArray original = readFile(archive);
        ArchiveBackend backend(nullptr, enginePath());
        CNativeMutationWorker worker(backend, archive, "zip");
        worker.Operation = ArchiveBackend::SourceMutation::Delete;
        auto transport = worker.CreateProgressTransport();
        QVERIFY(transport);
        QSignalSpy started(&backend, &ArchiveBackend::engineStarted);
        connect(&backend, &ArchiveBackend::engineStarted, &backend,
                [&worker] { worker.Sync.Set_Stopped(true); });
        QCOMPARE(worker.Create(L"Cancel source delete", nullptr), S_OK);
        QCOMPARE(worker.Result, E_ABORT);
        QCOMPARE(started.size(), 1);
        QCOMPARE(readFile(archive), original);
        QVERIFY(!backend.busy());
    }
    void encryptedSourceMutationPreservesProtection_data() {
        QTest::addColumn<QString>("format");
        QTest::addColumn<int>("operation");
        for (const QString &format : {QStringLiteral("zip"), QStringLiteral("7z")})
            for (int kind = 0; kind < 3; ++kind)
                QTest::newRow(qPrintable(format + QString::number(kind))) << format << kind;
    }
    void protectedGuiAddPreservesEncryption_data() {
        QTest::addColumn<QString>("format");
        QTest::newRow("zip-aes") << QStringLiteral("zip");
        QTest::newRow("7z-header") << QStringLiteral("7z");
    }
    void protectedGuiAddPreservesEncryption() {
        QFETCH(QString, format);
        QTemporaryDir fixture;
        QVERIFY(fixture.isValid());
        const QString root = fixture.filePath("source");
        const QString archive = fixture.filePath("protected." + format);
        const QString source = QDir(root).filePath(QStringLiteral("новый файл.txt"));
        QVERIFY(writeFile(QDir(root).filePath("keep.txt"), "protected old"));
        QVERIFY(writeFile(source, "protected new"));
        QStringList args = {"a", "-t" + format, "-pfixture123"};
        args << (format == "7z" ? "-mhe=on" : "-mem=AES256") << "--" << archive << "keep.txt";
        QVERIFY(runEngine(root, args));
        ArchiveBackend backend(nullptr, enginePath());
        CNativeAddWorker worker(backend, archive, source, format);
        worker.RootFolder = true;
        worker.ArchivePaths = {"keep.txt"};
        worker.ActionSet = &NUpdateArchive::k_ActionSet_Add;
        worker.PathMode = NWildcard::k_RelatPath;
        worker.Password = "fixture123";
        worker.DisableUserQuestions = true;
        auto transport = worker.CreateProgressTransport();
        QCOMPARE(worker.Create(L"Protected path-mode Add", nullptr), S_OK);
        QCOMPARE(worker.Result, S_OK);
        QProcess listProcess;
        listProcess.start(enginePath(), {"l", "-slt", "-sccUTF-8", "-pfixture123", "--", archive});
        QVERIFY(listProcess.waitForFinished(10000));
        QCOMPARE(listProcess.exitCode(), 0);
        const QByteArray listing = listProcess.readAllStandardOutput();
        QVERIFY(listing.contains("Path = keep.txt"));
        const QString listingText =
            QString::fromUtf8(listing).normalized(QString::NormalizationForm_C);
        QVERIFY(listingText.contains(QStringLiteral("Path = новый файл.txt")));
        QCOMPARE(listing.count("Encrypted = +"), 2);
        if (format == "zip")
            QCOMPARE(listing.count("Method = AES-256"), 2);
        else
            QVERIFY(!runEngine(fixture.path(), {"l", "-slt", "-pwrong", "--", archive}));
        const QString extracted = fixture.filePath("extracted");
        QVERIFY(runEngine(fixture.path(),
                          {"x", "-y", "-pfixture123", "-o" + extracted, "--", archive}));
        QCOMPARE(readFile(QDir(extracted).filePath("keep.txt")), QByteArray("protected old"));
        const QString addedPath = QDir(extracted).filePath(QStringLiteral("новый файл.txt"));
        QCOMPARE(readFile(addedPath), QByteArray("protected new"));
    }
    void protectedPlanWrongPasswordAndCancelKeepOriginal() {
        QTemporaryDir fixture;
        QVERIFY(fixture.isValid());
        const QString root = fixture.filePath("source");
        const QString archive = fixture.filePath("protected.7z");
        const QString output = fixture.filePath("updated.7z");
        QVERIFY(writeFile(QDir(root).filePath("keep.txt"), "old"));
        QVERIFY(writeFile(QDir(root).filePath("added.txt"), QByteArray(4 * 1024 * 1024, 'a')));
        QVERIFY(
            runEngine(root, {"a", "-t7z", "-pfixture123", "-mhe=on", "--", archive, "keep.txt"}));
        const QByteArray original = readFile(archive);
        ArchiveBackend::SourceUpdatePlan plan;
        plan.allowProtected = true;
        plan.fileRoot = root;
        plan.files = {"added.txt"};
        CompressionOptions options;
        ArchiveBackend wrong(nullptr, enginePath());
        QSignalSpy wrongFinished(&wrong, &ArchiveBackend::finished);
        wrong.updateSourcePlan(archive, plan, output, "7z", "wrong", options);
        QTRY_COMPARE_WITH_TIMEOUT(wrongFinished.count(), 1, 10000);
        QVERIFY(!wrongFinished.takeFirst().first().toBool());
        QVERIFY(!QFileInfo::exists(output));
        QCOMPARE(readFile(archive), original);

        ArchiveBackend cancelled(nullptr, enginePath());
        QSignalSpy cancelledFinished(&cancelled, &ArchiveBackend::finished);
        connect(&cancelled, &ArchiveBackend::engineStarted, &cancelled, &ArchiveBackend::cancel);
        cancelled.updateSourcePlan(archive, plan, output, "7z", "fixture123", options);
        QTRY_COMPARE_WITH_TIMEOUT(cancelledFinished.count(), 1, 10000);
        QVERIFY(!cancelledFinished.takeFirst().first().toBool());
        QCOMPARE(cancelled.lastResult(), E_ABORT);
        QVERIFY(!QFileInfo::exists(output));
        QCOMPARE(readFile(archive), original);
    }
    void encryptedSourceMutationPreservesProtection() {
        QFETCH(QString, format);
        QFETCH(int, operation);
        QTemporaryDir fixture;
        QVERIFY(fixture.isValid());
        const QString root = fixture.filePath("source");
        const QString archive = fixture.filePath("protected." + format);
        const QString entry = QStringLiteral("inside/имя с пробелом.bin");
        QVERIFY(writeFile(QDir(root).filePath(entry), "protected payload"));
        QVERIFY(writeFile(QDir(root).filePath("keep.bin"), "keep"));
        const QDateTime originalTime = QDateTime::fromSecsSinceEpoch(1700000000, Qt::UTC);
        QFile keep(QDir(root).filePath("keep.bin"));
        QVERIFY(keep.open(QIODevice::ReadWrite));
        QVERIFY(keep.setFileTime(originalTime, QFileDevice::FileModificationTime));
        keep.close();
        QStringList args = {"a", "-t" + format, "-pfixture123"};
        args << (format == "7z" ? "-mhe=on" : "-mem=AES256") << "--" << archive << entry
             << "keep.bin";
        QVERIFY(runEngine(root, args));
        ArchiveBackend backend(nullptr, enginePath());
        CNativeMutationWorker worker(backend, archive, format);
        worker.Password = "fixture123";
        worker.Operation = ArchiveBackend::SourceMutation(operation);
        auto transport = worker.CreateProgressTransport();
        QVERIFY(transport);
        QCOMPARE(worker.Create(L"Protected source mutation", nullptr), S_OK);
        QCOMPARE(worker.Result, S_OK);
        QVERIFY(!backend.busy());
        QVERIFY(!runEngine(fixture.path(), {"t", "-pwrong", "--", archive}));
        QVERIFY(runEngine(fixture.path(), {"t", "-pfixture123", "--", archive}));
        const QString extracted = fixture.filePath("extracted");
        QVERIFY(runEngine(fixture.path(),
                          {"x", "-y", "-pfixture123", "-o" + extracted, "--", archive}));
        QCOMPARE(readFile(QDir(extracted).filePath("keep.bin")), QByteArray("keep"));
        QCOMPARE(QFileInfo(QDir(extracted).filePath("keep.bin")).lastModified().toSecsSinceEpoch(),
                 originalTime.toSecsSinceEpoch());
        if (operation == 0)
            QVERIFY(!QFile::exists(QDir(extracted).filePath(entry)));
        else if (operation == 1) {
            QVERIFY(!QFile::exists(QDir(extracted).filePath(entry)));
            QCOMPARE(readFile(QDir(extracted).filePath("inside/новое имя.bin")),
                     QByteArray("protected payload"));
        } else {
            QCOMPARE(readFile(QDir(extracted).filePath(entry)), QByteArray("protected payload"));
            QVERIFY(QFileInfo(QDir(extracted).filePath("inside/новая папка")).isDir());
        }
    }
    void wrongPasswordAndCancellationKeepProtectedArchive() {
        QTemporaryDir fixture;
        QVERIFY(fixture.isValid());
        const QString root = fixture.filePath("source");
        const QString archive = fixture.filePath("protected.7z");
        const QString entry = QStringLiteral("inside/имя с пробелом.bin");
        QVERIFY(writeFile(QDir(root).filePath(entry), QByteArray(4 * 1024 * 1024, 'x')));
        QVERIFY(writeFile(QDir(root).filePath("keep.bin"), "keep"));
        QVERIFY(runEngine(
            root, {"a", "-t7z", "-pfixture123", "-mhe=on", "--", archive, entry, "keep.bin"}));
        const QByteArray original = readFile(archive);
        ArchiveBackend backend(nullptr, enginePath());
        CNativeMutationWorker wrong(backend, archive, "7z");
        wrong.DisableUserQuestions = true;
        wrong.Password = "wrong";
        wrong.Operation = ArchiveBackend::SourceMutation::Delete;
        auto wrongTransport = wrong.CreateProgressTransport();
        QVERIFY(wrongTransport);
        QCOMPARE(wrong.Create(L"Wrong protected password", nullptr), S_OK);
        QVERIFY(wrong.Result != S_OK);
        QCOMPARE(readFile(archive), original);
        CNativeMutationWorker cancelled(backend, archive, "7z");
        cancelled.Password = "fixture123";
        cancelled.Operation = ArchiveBackend::SourceMutation::Delete;
        auto cancelTransport = cancelled.CreateProgressTransport();
        QVERIFY(cancelTransport);
        connect(&backend, &ArchiveBackend::engineStarted, &backend,
                [&cancelled] { cancelled.Sync.Set_Stopped(true); });
        QCOMPARE(cancelled.Create(L"Cancel protected mutation", nullptr), S_OK);
        QCOMPARE(cancelled.Result, E_ABORT);
        QCOMPARE(readFile(archive), original);
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
