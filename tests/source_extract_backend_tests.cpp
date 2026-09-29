#include "7zip/Common/StreamObjects.h"
#include "CPP/7zip/UI/Agent/Agent.h"
#include "CPP/7zip/UI/FileManager/ExtractCallback.h"
#include "CPP/7zip/UI/FileManager/Panel.h"
#include "CPP/7zip/UI/FileManager/ProgressDialog2.h"
#include "archivebackend.h"
#include "qtarchivebackendrequest.h"
#include "qtarchivefileresource.h"
#include "qtarchivefoldermanager.h"
#include "qtpanelfolder.h"
#include "source_extract_fixture.h"
#include <QDir>
#include <QFile>
#include <QProcess>
#include <QSignalSpy>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QtTest>

namespace {
using namespace SourceExtractFixture;
class CNativeExtractWorker final : public CProgressThreadVirt {
    ArchiveBackend &_backend;
    QString _archive, _destination;
    UInt64 _size;
    CMyComPtr<IUnknown> _resource;
    QtArchiveFileResource *_resourceSpec;
    std::unique_ptr<IQtProgressTransport> _transport;
    HRESULT ProcessVirt() override {
        CObjectVector<CPanelItem> items;
        CPanelItem item;
        item.RelPath = kName;
        item.SizeDefined = true;
        item.Size = _size;
        items.Add(item);
        CMyComPtr<IFolderFolder> metadata =
            new QtPanelFolder({}, items, QtPanelFolder::PropertyCatalog::ArchiveTransport);
        CMyComPtr2_Create<IUnknown, CAgent> agent;
        RINOK(agent->Init({}, metadata, UString("zip"), _resource))
        CMyComPtr<IFolderFolder> root, inside;
        RINOK(agent->BindToRootFolder(&root))
        RINOK(root->BindToFolder(L"inside", &inside))
        RINOK(inside->LoadItems())
        CMyComPtr2_Create<IFolderArchiveExtractCallback, CExtractCallbackImp> ui;
        ui->ProgressDialog = this;
        ui->Init();
        ui->StreamMode = Memory;
        CMyComPtr2<ISequentialOutStream, CVirtFileSystem> virtualFiles;
        if (Memory) {
            virtualFiles.Create_if_Empty();
            virtualFiles->FileName = L"密碼 с пробелом.bin";
            virtualFiles->DirPrefix = qUtf8Printable(_destination + "/");
            ui->VirtFileSystem = virtualFiles.Interface();
            ui->VirtFileSystemSpec = virtualFiles.ClsPtr();
        }
        const UInt32 selected = 0;
        CMyComPtr<IFolderOperations> operations;
        RINOK(inside.QueryInterface(IID_IFolderOperations, &operations))
        CMyComPtr<IFolderOperationsExtractCallback> copyCallback;
        RINOK(ui.Interface()->QueryInterface(IID_IFolderOperationsExtractCallback,
                                             reinterpret_cast<void **>(&copyCallback)))
        RINOK(operations->CopyTo(false, &selected, 1, false, false,
                                 _destination.toStdWString().c_str(), copyCallback))
        if (Memory) {
            if (virtualFiles->Files.Size() != 1)
                return E_FAIL;
            const auto &file = virtualFiles->Files.Front();
            MemoryBytes =
                QByteArray(reinterpret_cast<const char *>(static_cast<const Byte *>(file.Data)),
                           file.WrittenSize);
        }
        return ui->IsOK() ? S_OK : E_FAIL;
    }

  public:
    bool Memory = false;
    QByteArray MemoryBytes;
    CNativeExtractWorker(ArchiveBackend &backend, const QString &archive,
                         const QString &destination, UInt64 size)
        : _backend(backend), _archive(archive), _destination(destination), _size(size),
          _resourceSpec(new QtArchiveFileResource(archive)) {
        _resource = _resourceSpec;
        _resourceSpec->AttachExtractService(backend, "zip");
        _transport = _resourceSpec->CreateProgressTransport(*this);
    }
};

} // namespace

class SourceExtractBackendTests final : public QObject {
    Q_OBJECT
  private slots:
    void nativeNestedOpenUsesMemoryAndRestoresParent() {
        QTemporaryDir inner, outer;
        const QByteArray contents("nested contents\0bytes", 21);
        const auto innerArchive = makeArchive(inner.path(), contents);
        QVERIFY(!innerArchive.isEmpty());
        const auto outerArchive = makeArchive(outer.path(), readFile(innerArchive));
        QVERIFY(!outerArchive.isEmpty());
        ArchiveBackend backend;
        CMyComPtr<IFolderManager> manager = new QtArchiveFolderManager(backend);
        CPanel panel;
        panel.AttachFolderManager(manager, outer.filePath("settings.ini"));
        COpenResult result;
        QCOMPARE(panel.BindToPath(ToUString(outerArchive + "/inside/"), UString(), result), S_OK);
        QCOMPARE(panel.GetParentFolders().Size(), 1u);
        auto *parent = panel.GetFolder();
        CMyComPtr<IFolderFolder> retainParent = parent;
        QCOMPARE(parent->LoadItems(), S_OK);
        panel.OpenItemInArchive(0, true, false, false, false, L"zip");
        QCOMPARE(panel.GetParentFolders().Size(), 2u);
        QVERIFY(panel.GetParentFolders().Back().IsVirtual);
        QVERIFY(panel.GetParentFolders().Back().NeedDelete);
        QVERIFY(!QFileInfo::exists(ToQString(fs2us(panel.GetParentFolders().Back().FilePath))));
        QCOMPARE(panel.GetParentFolders().Back().ParentFolder, parent);
        QVERIFY(!backend.busy());
        QCOMPARE(panel.GetFolder()->LoadItems(), S_OK);
        UInt32 items;
        QCOMPARE(panel.GetFolder()->GetNumberOfItems(&items), S_OK);
        QCOMPARE(items, 1u); // Native proxy owns the inner archive's "inside" directory.
        panel.OpenParentFolder();
        QCOMPARE(panel.GetParentFolders().Size(), 1u);
        QCOMPARE(panel.GetFolder(), parent);
        panel.CloseOpenFolders();
        QCOMPARE(panel.GetParentFolders().Size(), 0u);
        QCOMPARE(readFile(innerArchive), readFile(outer.filePath(kName)));
    }
    void panelCopyOwnsNativeCallbackAndVirtualFiles() {
        QTemporaryDir directory;
        const QByteArray bytes("panel stream\0binary\xff", 20);
        const auto archive = makeArchive(directory.path(), bytes);
        QVERIFY(!archive.isEmpty());
        ArchiveBackend backend;
        auto *resourceSpec = new QtArchiveFileResource(archive);
        CMyComPtr<IUnknown> resource = resourceSpec;
        resourceSpec->AttachExtractService(backend, "zip");
        // The source folder retains this resource while the panel's own worker
        // owns the progress transport and callback for the complete modal call.
        CObjectVector<CPanelItem> items;
        CPanelItem item;
        item.RelPath = kName;
        item.SizeDefined = true;
        item.Size = bytes.size();
        items.Add(item);
        CMyComPtr<IFolderFolder> metadata =
            new QtPanelFolder({}, items, QtPanelFolder::PropertyCatalog::ArchiveTransport);
        CMyComPtr2_Create<IUnknown, CAgent> agent;
        QCOMPARE(agent->Init({}, metadata, UString("zip"), resource), S_OK);
        CMyComPtr<IFolderFolder> root, inside;
        QCOMPARE(agent->BindToRootFolder(&root), S_OK);
        QCOMPARE(root->BindToFolder(L"inside", &inside), S_OK);
        QCOMPARE(inside->LoadItems(), S_OK);
        CPanel panel(inside);
        CRecordVector<UInt32> selected;
        selected.Add(0);
        bool usePassword = false;
        UString password;
        UStringVector messages;
        for (bool memory : {false, true}) {
            QTemporaryDir output;
            CCopyToOptions options;
            options.folder = ToUString(output.path() + "/");
            options.streamMode = memory;
            options.NeedRegistryZone = false;
            CMyComPtr2<ISequentialOutStream, CVirtFileSystem> virtualFiles;
            if (memory) {
                virtualFiles.Create_if_Empty();
                virtualFiles->FileName = L"密碼 с пробелом.bin";
                virtualFiles->DirPrefix = qUtf8Printable(output.path() + "/");
                options.VirtFileSystemSpec = virtualFiles.ClsPtr();
            }
            QCOMPARE(panel.CopyTo(options, selected, &messages, usePassword, password), S_OK);
            QVERIFY(messages.IsEmpty());
            QVERIFY(!backend.busy());
            if (memory) {
                QCOMPARE(virtualFiles->Files.Size(), 1u);
                const auto &file = virtualFiles->Files.Front();
                QCOMPARE(
                    QByteArray(reinterpret_cast<const char *>(static_cast<const Byte *>(file.Data)),
                               file.WrittenSize),
                    bytes);
            } else {
                QCOMPARE(readFile(output.filePath(QStringLiteral("密碼 с пробелом.bin"))), bytes);
            }
        }
    }
    void nativeFolderExtractionAndMemory() {
        QTemporaryDir directory;
        const QByteArray bytes("native stream\0with binary\xff", 26);
        const auto archive = makeArchive(directory.path(), bytes);
        QVERIFY(!archive.isEmpty());
        for (bool memory : {false, true}) {
            QTemporaryDir output;
            ArchiveBackend backend;
            CNativeExtractWorker worker(backend, archive, output.path(), bytes.size());
            worker.Memory = memory;
            QCOMPARE(worker.Create(L"Source extraction"), S_OK);
            QCOMPARE(worker.Result, S_OK);
            QVERIFY(!backend.busy());
            if (memory)
                QCOMPARE(worker.MemoryBytes, bytes);
            else
                QCOMPARE(readFile(output.filePath(QStringLiteral("密碼 с пробелом.bin"))), bytes);
        }
    }
    void rejectedOutputAndFailedStart() {
        QTemporaryDir directory;
        const auto archive = makeArchive(directory.path(), "contents");
        QVERIFY(!archive.isEmpty());
        const auto existing = directory.filePath("existing.bin");
        QVERIFY(writeFile(existing, "preserved"));
        ArchiveBackend backend;
        QSignalSpy finished(&backend, &ArchiveBackend::finished);
        backend.extractSourceEntry(archive, kName, existing, "zip", {});
        QCOMPARE(finished.size(), 1);
        QVERIFY(!finished.front().front().toBool());
        QCOMPARE(readFile(existing), QByteArray("preserved"));
        ArchiveBackend missing(nullptr, directory.filePath("missing-engine"));
        QSignalSpy failed(&missing, &ArchiveBackend::finished);
        const auto stream = directory.filePath("failed.bin");
        missing.extractSourceEntry(archive, kName, stream, "zip", {});
        QTRY_COMPARE_WITH_TIMEOUT(failed.size(), 1, 10000);
        QVERIFY(!failed.front().front().toBool());
        QVERIFY(!QFileInfo::exists(stream));
        QVERIFY(!missing.busy());
    }
    void cancelAndMalformedArchiveCleanOwnedStream() {
        QTemporaryDir directory;
        const auto archive = makeArchive(directory.path(), QByteArray(4 << 20, 'x'));
        QVERIFY(!archive.isEmpty());
        const auto original = readFile(archive);
        ArchiveBackend backend;
        QSignalSpy finished(&backend, &ArchiveBackend::finished);
        const auto cancellation = connect(&backend, &ArchiveBackend::engineStarted, &backend,
                                          [&backend] { backend.cancel(); });
        const auto output = directory.filePath("cancelled.bin");
        backend.extractSourceEntry(archive, kName, output, "zip", {});
        QTRY_COMPARE_WITH_TIMEOUT(finished.size(), 1, 10000);
        QVERIFY(!finished.front().front().toBool());
        QCOMPARE(backend.lastResult(), E_ABORT);
        QVERIFY(!QFileInfo::exists(output));
        QCOMPARE(readFile(archive), original);
        disconnect(cancellation);
        finished.clear();
        const auto malformed = directory.filePath("broken.zip");
        QVERIFY(writeFile(malformed, "not an archive"));
        backend.extractSourceEntry(malformed, kName, output, "zip", {});
        QTRY_COMPARE_WITH_TIMEOUT(finished.size(), 1, 10000);
        QVERIFY(!finished.front().front().toBool());
        QVERIFY(!QFileInfo::exists(output));
        QVERIFY(!backend.busy());
    }
    void binaryStreamAndChannelReset() {
        QTemporaryDir directory;
        const QByteArray bytes =
            QByteArray::fromHex("0001ff0a0d00") + "Enter password:\n50% secret\n";
        const auto archive = makeArchive(directory.path(), bytes);
        QVERIFY(!archive.isEmpty());
        ArchiveBackend backend;
        QSignalSpy finished(&backend, &ArchiveBackend::finished);
        QSignalSpy passwords(&backend, &ArchiveBackend::passwordRequired);
        QSignalSpy messages(&backend, &ArchiveBackend::outputReceived);
        const auto output = directory.filePath("stream.bin");
        backend.extractSourceEntry(archive, kName, output, "zip", {});
        QTRY_COMPARE_WITH_TIMEOUT(finished.size(), 1, 10000);
        QVERIFY(finished.front().front().toBool());
        QCOMPARE(readFile(output), bytes);
        QCOMPARE(passwords.size(), 0);
        for (const auto &message : messages)
            QVERIFY(!message.front().toString().contains("secret"));
        finished.clear();
        backend.list(archive);
        QTRY_COMPARE_WITH_TIMEOUT(finished.size(), 1, 10000);
        QVERIFY(finished.front().front().toBool());
        QCOMPARE(readFile(output), bytes);
    }
};
QTEST_MAIN(SourceExtractBackendTests)
#include "source_extract_backend_tests.moc"
