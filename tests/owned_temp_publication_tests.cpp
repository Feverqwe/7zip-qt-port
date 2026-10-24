#include "7zip/Common/FileStreams.h"
#include "CPP/7zip/UI/Agent/Agent.h"
#include "CPP/7zip/UI/Common/WorkDir.h"
#include "CPP/7zip/UI/FileManager/ProgressDialog2.h"
#include "CPP/7zip/UI/FileManager/UpdateCallback100.h"
#include "Common/StringConvert.h"
#include "archivebackend.h"
#include "native_archive_fixture.h"
#include "qtarchivefileresource.h"
#include "qtownedtempregistry.h"
#include "qtpanelfolder.h"
#include <QFile>
#include <QProcess>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QtTest>
#include <algorithm>

// Exercise CommonUpdateOperation, including its publication/reopen adapter.
class CSpoolRenameWorker final : public CProgressThreadVirt {
    QtArchiveFileResource &_resource;
    HRESULT ProcessVirt() override {
        CObjectVector<CPanelItem> items;
        CPanelItem item;
        item.RelPath = "original.bin";
        items.Add(item);
        CMyComPtr<IFolderFolder> props =
            new QtPanelFolder({}, items, QtPanelFolder::PropertyCatalog::ArchiveTransport);
        CMyComPtr2_Create<IFolderArcProps, CAgent> agent;
        RINOK(agent->Init({}, props, UString("zip"), &_resource))
        CMyComPtr<IFolderFolder> root;
        RINOK(agent->BindToRootFolder(&root))
        RINOK(root->LoadItems())
        CMyComPtr<IFolderOperations> operations;
        RINOK(root.QueryInterface(IID_IFolderOperations, &operations))
        CMyComPtr2_Create<IFolderArchiveUpdateCallback, CUpdateCallback100Imp> callback;
        callback->ProgressDialog = this;
        callback->Init();
        return operations->Rename(0, L"новое имя.bin", callback);
    }

  public:
    explicit CSpoolRenameWorker(QtArchiveFileResource &resource) : _resource(resource) {}
};

class OwnedTempPublicationTests : public QObject {
    Q_OBJECT
    static QByteArray read(const QString &path) {
        QFile file(path);
        return file.open(QIODevice::ReadOnly) ? file.readAll() : QByteArray();
    }

  private slots:
    void interruptedSpoolRemovesTemporaryFileAndLease_data() {
        QTest::addColumn<HRESULT>("result");
        QTest::newRow("cancel") << HRESULT(E_ABORT);
        QTest::newRow("failure") << HRESULT(E_FAIL);
    }
    void interruptedSpoolRemovesTemporaryFileAndLease() {
        QFETCH(HRESULT, result);
        class StopProgress final : public IProgress, public CMyUnknownImp {
            Z7_COM_UNKNOWN_IMP_1(IProgress)
            HRESULT _result;

          public:
            unsigned calls = 0;
            QVector<QtOwnedTempEntry> entries;
            explicit StopProgress(HRESULT result) : _result(result) {}
            Z7_COM7F_IMF(SetTotal(UInt64)) override { return S_OK; }
            Z7_COM7F_IMF(SetCompleted(const UInt64 *)) override {
                ++calls;
                entries = QtOwnedTempRegistry().Entries();
                return _result;
            }
        };
        QTemporaryDir fixture;
        QVERIFY(fixture.isValid());
        const QString path = fixture.filePath(QStringLiteral("source файл 密碼 with space.zip"));
        // The original copy coder reports progress at 4 MiB boundaries.
        const QByteArray bytes(8 * 1024 * 1024, 'x');
        {
            QFile file(path);
            QVERIFY(file.open(QIODevice::WriteOnly));
            QCOMPARE(file.write(bytes), qint64(bytes.size()));
        }
        QtOwnedTempRegistry registry;
        const auto entriesBefore = registry.Entries();
        auto *progressSpec = new StopProgress(result);
        CMyComPtr<IProgress> progress = progressSpec;
        auto *resource = new QtArchiveFileResource(path);
        CMyComPtr<IUnknown> owner = resource;
        CMyComPtr2_Create<IInStream, CInFileStream> input;
        QVERIFY(input->Open(us2fs(path.toStdWString().c_str())));
        QCOMPARE(resource->Spool(input, progress), result);
        QVERIFY(progressSpec->calls > 0);
        QCOMPARE(resource->EnginePath, path);
        QCOMPARE(registry.Entries().size(), entriesBefore.size());
        QCOMPARE(read(path), bytes);
        QVERIFY(!progressSpec->entries.isEmpty());
        for (const auto &entry : progressSpec->entries) {
            const auto previous =
                std::find_if(entriesBefore.cbegin(), entriesBefore.cend(),
                             [&](const auto &old) { return old.key == entry.key; });
            if (previous == entriesBefore.cend())
                QVERIFY2(!QFileInfo::exists(entry.path), qPrintable(entry.path));
        }
    }
    void nativeFolderTransactionRetainsSpoolOwnership_data() {
        QTest::addColumn<int>("outcome");
        QTest::newRow("published-and-reopened") << 0;
        QTest::newRow("engine-failed") << 1;
        QTest::newRow("cancelled-before-publication") << 2;
    }
    void nativeFolderTransactionRetainsSpoolOwnership() {
        QFETCH(int, outcome);
        const QString engine = QStandardPaths::findExecutable("7z");
        QVERIFY(!engine.isEmpty());
        QTemporaryDir fixture;
        QVERIFY(fixture.isValid());
        const QString original = fixture.filePath(QStringLiteral("исходный архив.zip"));
        {
            QFile file(fixture.filePath("original.bin"));
            QVERIFY(file.open(QIODevice::WriteOnly));
            QCOMPARE(file.write("original content"), qint64(16));
        }
        QProcess create;
        create.setWorkingDirectory(fixture.path());
        create.start(engine, {"a", "-tzip", "--", original, "original.bin"});
        QVERIFY(create.waitForFinished(10000));
        QCOMPARE(create.exitCode(), 0);
        const QByteArray originalBytes = read(original);
        auto *resource = new QtArchiveFileResource(original);
        CMyComPtr<IUnknown> owner = resource;
        {
            CMyComPtr2_Create<IInStream, CInFileStream> input;
            QVERIFY(input->Open(us2fs(original.toStdWString().c_str())));
            QCOMPARE(resource->Spool(input, nullptr), S_OK);
        }
        const QString spool = resource->EnginePath;
#ifndef Q_OS_WIN
        const auto publicPermissions = QFileDevice::ReadGroup | QFileDevice::WriteGroup |
                                       QFileDevice::ExeGroup | QFileDevice::ReadOther |
                                       QFileDevice::WriteOther | QFileDevice::ExeOther;
        QVERIFY(!(QFileInfo(spool).permissions() & publicPermissions));
#endif
        QtOwnedTempRegistry registry;
        QString oldIdentity;
        for (const auto &entry : registry.Entries())
            if (entry.path == spool)
                oldIdentity = entry.identity;
        QVERIFY(!oldIdentity.isEmpty());
        ArchiveBackend backend(nullptr, outcome == 1 ? fixture.filePath("missing-module") : engine);
        if (outcome != 1) {
            QCOMPARE(NativeArchiveFixture::Attach(backend), S_OK);
            std::shared_ptr<ArchiveNativeOpen> reader;
            QCOMPARE(backend.openNativeArchive(spool, "zip", nullptr, reader), S_OK);
            QVERIFY(reader);
            resource->AttachCodecCatalog(backend.codecCatalog());
            resource->AttachNativeOpen(std::move(reader));
            resource->UseNativeMetadata();
        }

        resource->AttachUpdateService(backend, "zip");
        resource->AttachReopenService(backend, "zip");
        {
            CSpoolRenameWorker worker(*resource);
            worker.DisableUserQuestions = true;
            auto transport = resource->CreateProgressTransport(worker);
            QVERIFY(transport);
            if (outcome == 2)
                worker.Sync.Set_Stopped(true);
            QCOMPARE(worker.Create(L"Spool rename", nullptr), S_OK);
            if (outcome == 0)
                QCOMPARE(worker.Result, S_OK);
            else if (outcome == 2)
                QCOMPARE(worker.Result, E_ABORT);
            else
                QVERIFY(worker.Result != S_OK);
        }
        QtOwnedTempEntry retained;
        for (const auto &entry : registry.Entries())
            if (entry.path == spool)
                retained = entry;
        QVERIFY(!retained.key.isEmpty());
        QVERIFY(retained.active);
        QCOMPARE(retained.identity != oldIdentity, outcome == 0);
        QVERIFY(!registry.ClaimForDeletion(retained));
        QCOMPARE(read(original), originalBytes);
        if (outcome != 0)
            QCOMPARE(read(spool), originalBytes);
        else {
            QProcess inspect;
            inspect.start(engine, {"l", "-slt", "-sccUTF-8", "--", spool});
            QVERIFY(inspect.waitForFinished(10000));
            QCOMPARE(inspect.exitCode(), 0);
            QVERIFY(
                inspect.readAllStandardOutput().contains(QStringLiteral("новое имя.bin").toUtf8()));
        }
        owner.Release();
        QVERIFY(!QFile::exists(spool));
        QVERIFY(QFile::exists(original));
        for (const auto &entry : registry.Entries())
            QVERIFY(entry.path != spool);
    }
    void nativePublicationTransfersOnlyAnAlreadyOwnedSpool_data() {
        QTest::addColumn<bool>("spool");
        QTest::newRow("owned-stream-spool") << true;
        QTest::newRow("user-archive-remains-unowned") << false;
    }
    void nativePublicationTransfersOnlyAnAlreadyOwnedSpool() {
        QFETCH(bool, spool);
        QTemporaryDir fixture;
        const QString original = fixture.filePath(QStringLiteral("архив с пробелом.7z"));
        {
            QFile file(original);
            QVERIFY(file.open(QIODevice::WriteOnly));
            QCOMPARE(file.write("original"), qint64(8));
        }
        auto *resource = new QtArchiveFileResource(original);
        CMyComPtr<IUnknown> owner = resource;
        if (spool) {
            CMyComPtr2_Create<IInStream, CInFileStream> input;
            QVERIFY(input->Open(us2fs(original.toStdWString().c_str())));
            QCOMPARE(resource->Spool(input, nullptr), S_OK);
        }
        const QString destination = resource->EnginePath;
        QtOwnedTempRegistry registry;
        QString oldIdentity;
        for (const auto &entry : registry.Entries())
            if (entry.path == destination)
                oldIdentity = entry.identity;
        QCOMPARE(!oldIdentity.isEmpty(), spool);
        std::shared_ptr<QtOwnedTempLease> nativeLease;
        {
            CWorkDirTempFile publication;
            QCOMPARE(publication.CreateTempFile(us2fs(destination.toStdWString().c_str()),
                                                fixture.filePath("settings.ini")),
                     S_OK);
            nativeLease = publication.OwnedLease();
            const QByteArray replacement("published replacement");
            UInt32 processed = 0;
            QCOMPARE(publication.OutStream->Write(replacement.constData(), replacement.size(),
                                                  &processed),
                     S_OK);
            QCOMPARE(processed, UInt32(replacement.size()));
            QCOMPARE(publication.MoveToOriginal(true), S_OK);
            QCOMPARE(resource->AdoptPublishedOwnedTemp(publication), S_OK);
            QCOMPARE(read(destination), replacement);
        }
        nativeLease.reset();
        QtOwnedTempEntry published;
        for (const auto &entry : registry.Entries())
            if (entry.path == destination)
                published = entry;
        QCOMPARE(!published.key.isEmpty(), spool);
        if (spool) {
            QVERIFY(published.active);
            QVERIFY(published.identity != oldIdentity);
            QVERIFY(!registry.ClaimForDeletion(published));
        }
        owner.Release();
        QCOMPARE(QFile::exists(destination), !spool);
        QVERIFY(QFile::exists(original));
        for (const auto &entry : registry.Entries())
            QVERIFY(entry.path != destination);
    }
};
QTEST_MAIN(OwnedTempPublicationTests)
#include "owned_temp_publication_tests.moc"
