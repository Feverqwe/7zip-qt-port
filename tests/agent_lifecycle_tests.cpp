#include "7zip/PropID.h"
#include "CPP/7zip/UI/Agent/Agent.h"
#include "Windows/PropVariant.h"
#include "archive_backend_test_support.h"
#include "qtarchivefoldermanager.h"
#include <QScopeGuard>
#include <QStandardPaths>
#include <atomic>
#include <functional>

namespace {
class CBindingCancellation final : public IProgress,
                                   public IArchiveOpenCallback,
                                   public CMyUnknownImp {
    Z7_COM_UNKNOWN_IMP_2(IProgress, IArchiveOpenCallback)
  public:
    std::atomic<bool> Cancel{false};
    std::atomic<unsigned> Totals{0};
    Z7_COM7F_IMF(SetTotal(UInt64)) override {
        ++Totals;
        return Cancel.load() ? E_ABORT : S_OK;
    }
    Z7_COM7F_IMF(SetCompleted(const UInt64 *)) override { return Cancel.load() ? E_ABORT : S_OK; }
    Z7_COM7F_IMF(SetTotal(const UInt64 *, const UInt64 *)) override {
        ++Totals;
        return Cancel.load() ? E_ABORT : S_OK;
    }
    Z7_COM7F_IMF(SetCompleted(const UInt64 *, const UInt64 *)) override {
        return Cancel.load() ? E_ABORT : S_OK;
    }
};
class CBindingBackend final : public ArchiveBackend {
  public:
    CBindingCancellation &Progress;
    bool ArmCancellation = true;
    unsigned Listings = 0;
    CBindingBackend(const QString &engine, CBindingCancellation &progress)
        : ArchiveBackend(nullptr, engine), Progress(progress) {}
    void openFolder(const QString &path, const QString &format, const QString &password) override {
        ++Listings;
        // Native opening has completed. Cancel specifically during the
        // source proxy's subsequent root binding, after process listing.
        Progress.Cancel.store(ArmCancellation);
        ArchiveBackend::openFolder(path, format, password);
    }
};
} // namespace

class AgentLifecycleTests final : public QObject, protected ArchiveBackendTestSupport {
    Q_OBJECT
    QString engine_;

    HRESULT run(ArchiveBackend &backend, const std::function<HRESULT()> &operation) {
        std::atomic<bool> finished{false};
        HRESULT result = E_FAIL;
        auto *worker = QThread::create([&] {
            result = operation();
            finished.store(true);
        });
        const auto join = qScopeGuard([&] {
            backend.cancelAndWait();
            worker->wait();
            delete worker;
        });
        worker->start();
        QElapsedTimer timer;
        timer.start();
        while (!finished.load() && timer.elapsed() < 30000) {
            QCoreApplication::processEvents();
            QTest::qWait(1);
        }
        if (!finished.load()) {
            QTest::qFail("Source lifecycle worker did not finish", __FILE__, __LINE__);
            return E_ABORT;
        }
        return result;
    }

  private slots:
    void initTestCase() {
        engine_ = QStandardPaths::findExecutable("7zz");
        QVERIFY2(!engine_.isEmpty(), "Official fixture engine is required");
    }
    void closedAndFailedReopenCannotLoadOldFolders_data() {
        QTest::addColumn<QString>("format");
        QTest::addColumn<bool>("flat");
        for (const QString &format : {QStringLiteral("zip"), QStringLiteral("wim")}) {
            QTest::newRow(qPrintable(format + "-normal")) << format << false;
            QTest::newRow(qPrintable(format + "-flat")) << format << true;
        }
    }
    void closedAndFailedReopenCannotLoadOldFolders() {
        QFETCH(QString, format);
        QFETCH(bool, flat);
        QTemporaryDir temporary;
        QVERIFY(temporary.isValid());
        const QString directory = QStringLiteral("папка 空 with spaces");
        const QString name = QStringLiteral("файл 空.bin");
        QVERIFY(QDir(temporary.path()).mkpath(directory));
        const QString input = temporary.filePath(directory + '/' + name);
        const QByteArray payload("unchanged\0binary", 16);
        writeFile(input, payload);
        const QString path = temporary.filePath(QStringLiteral("архив 空.") + format);
        QProcess process;
        process.setWorkingDirectory(temporary.path());
        process.start(engine_, {"a", "-t" + format, "--", path, directory});
        QVERIFY(process.waitForFinished(10000));
        QCOMPARE(process.exitCode(), 0);
        QFile file(path);
        QVERIFY(file.open(QIODevice::ReadOnly));
        const QByteArray archiveBytes = file.readAll();
        file.close();

        ArchiveBackend backend(nullptr, engine_);
        QSignalSpy ready(&backend, &ArchiveBackend::codecCatalogReady);
        QVERIFY(backend.discoverCodecCatalog());
        QTRY_COMPARE(ready.size(), 1);
        QVERIFY2(ready[0][0].toBool(), "Matching official codec module is required");
        CMyComPtr<IFolderManager> manager = new QtArchiveFolderManager(backend);
        CMyComPtr<IFolderFolder> root;
        const std::wstring nativePath = path.toStdWString();
        const std::wstring nativeFormat = format.toStdWString();
        QCOMPARE(run(backend,
                     [&] {
                         return manager->OpenFolderFile(nullptr, nativePath.c_str(),
                                                        nativeFormat.c_str(), &root, nullptr);
                     }),
                 S_OK);
        CMyComPtr<IFolderSetFlatMode> mode;
        QCOMPARE(root.QueryInterface(IID_IFolderSetFlatMode, &mode), S_OK);
        QCOMPARE(mode->SetFlatMode(flat), S_OK);
        QCOMPARE(root->LoadItems(), S_OK);
        CMyComPtr<IFolderFolder> child;
        const std::wstring nativeDirectory = directory.toStdWString();
        QCOMPARE(root->BindToFolder(nativeDirectory.c_str(), &child), S_OK);
        QVERIFY(child);
        QCOMPARE(child->LoadItems(), S_OK);
        CMyComPtr<IGetFolderArcProps> getter;
        CMyComPtr<IFolderArcProps> properties;
        CMyComPtr<IInFolderArchive> archive;
        QCOMPARE(root.QueryInterface(IID_IGetFolderArcProps, &getter), S_OK);
        QCOMPARE(getter->GetFolderArcProps(&properties), S_OK);
        QCOMPARE(properties.QueryInterface(IID_IInFolderArchive, &archive), S_OK);
        NWindows::NCOM::CPropVariant originalType;
        QCOMPARE(root->GetFolderProperty(kpidType, &originalType), S_OK);
        const QString type = QString::fromWCharArray(originalType.bstrVal);
        manager.Release();
        QCOMPARE(archive->Close(), S_OK);
        // Both published folders still retain their source owner/proxy, but
        // the original LoadItems refuses a closed link before reading either.
        QCOMPARE(root->LoadItems(), E_FAIL);
        QCOMPARE(child->LoadItems(), E_FAIL);
        QVERIFY(QFile::remove(path));
        const HRESULT failure = run(backend, [&] { return archive->ReOpen(nullptr); });
        QVERIFY(failure != S_OK);
        // ReOpen has now deleted the old proxies. The source link-open guard
        // must short-circuit before consulting those invalidated pointers.
        QCOMPARE(root->LoadItems(), E_FAIL);
        QCOMPARE(child->LoadItems(), E_FAIL);
        NWindows::NCOM::CPropVariant stalePath;
        QCOMPARE(root->GetFolderProperty(kpidPath, &stalePath), E_FAIL);
        QCOMPARE(child->GetFolderProperty(kpidPath, &stalePath), E_FAIL);
        NWindows::NCOM::CPropVariant retainedType;
        QCOMPARE(root->GetFolderProperty(kpidType, &retainedType), S_OK);
        QCOMPARE(QString::fromWCharArray(retainedType.bstrVal), type);
        writeFile(path, archiveBytes);
        QCOMPARE(run(backend, [&] { return archive->ReOpen(nullptr); }), S_OK);
        CMyComPtr<IFolderFolder> repaired;
        QCOMPARE(archive->BindToRootFolder(&repaired), S_OK);
        QVERIFY(repaired);
        QCOMPARE(root->GetFolderProperty(kpidPath, &stalePath), E_FAIL);
        QCOMPARE(child->GetFolderProperty(kpidPath, &stalePath), E_FAIL);
        root.Release();
        child.Release();
        getter.Release();
        mode.Release();
        properties.Release();
        archive.Release();
        QCOMPARE(repaired->LoadItems(), S_OK);
        QCOMPARE(repaired->BindToFolder(nativeDirectory.c_str(), &child), S_OK);
        QCOMPARE(child->LoadItems(), S_OK);
        UInt32 count = 0;
        QCOMPARE(child->GetNumberOfItems(&count), S_OK);
        QCOMPARE(count, UInt32(1));
        NWindows::NCOM::CPropVariant size;
        QCOMPARE(child->GetProperty(0, kpidSize, &size), S_OK);
        QCOMPARE(size.vt, VARTYPE(VT_UI8));
        QCOMPARE(size.uhVal.QuadPart, UInt64(payload.size()));
        QFile original(input);
        QVERIFY(original.open(QIODevice::ReadOnly));
        QCOMPARE(original.readAll(), payload);
        QVERIFY(file.open(QIODevice::ReadOnly));
        QCOMPARE(file.readAll(), archiveBytes);
    }
    void canceledBindingCanRetryWithSameManager_data() {
        QTest::addColumn<QString>("format");
        QTest::newRow("path-proxy") << QStringLiteral("zip");
        QTest::newRow("tree-proxy") << QStringLiteral("wim");
    }
    void canceledBindingCanRetryWithSameManager() {
        QFETCH(QString, format);
        QTemporaryDir temporary;
        QVERIFY(temporary.isValid());
        const QString input = temporary.filePath(QStringLiteral("файл 空 with spaces.txt"));
        const QString path = temporary.filePath(QStringLiteral("архив 空.") + format);
        const QByteArray payload("protected\0bytes", 15);
        writeFile(input, payload);
        QProcess process;
        process.start(engine_, {"a", "-t" + format, "--", path, input});
        QVERIFY(process.waitForFinished(10000));
        QCOMPARE(process.exitCode(), 0);
        QFile file(path);
        QVERIFY(file.open(QIODevice::ReadOnly));
        const QByteArray archiveBytes = file.readAll();
        file.close();
        auto *progressSpec = new CBindingCancellation;
        CMyComPtr<IProgress> progress = progressSpec;
        CBindingBackend backend(engine_, *progressSpec);
        QSignalSpy ready(&backend, &ArchiveBackend::codecCatalogReady);
        QVERIFY(backend.discoverCodecCatalog());
        QTRY_COMPARE(ready.size(), 1);
        QVERIFY(ready[0][0].toBool());
        CMyComPtr<IFolderManager> manager = new QtArchiveFolderManager(backend);
        const std::wstring nativePath = path.toStdWString();
        const std::wstring nativeFormat = format.toStdWString();
        CMyComPtr<IFolderFolder> folder;
        auto open = [&] {
            return manager->OpenFolderFile(nullptr, nativePath.c_str(), nativeFormat.c_str(),
                                           &folder, progress);
        };
        QCOMPARE(run(backend, open), E_ABORT);
        QVERIFY(!folder);
        QCOMPARE(backend.Listings, 1u);
        QVERIFY(progressSpec->Totals.load() > 0);
        backend.ArmCancellation = false;
        progressSpec->Cancel.store(false);
        QCOMPARE(run(backend, open), S_OK);
        QVERIFY(folder);
        QCOMPARE(backend.Listings, 2u);
        manager.Release();
        progress.Release();
        CMyComPtr<IFolderSetFlatMode> flat;
        QCOMPARE(folder.QueryInterface(IID_IFolderSetFlatMode, &flat), S_OK);
        QCOMPARE(flat->SetFlatMode(true), S_OK);
        QCOMPARE(folder->LoadItems(), S_OK);
        UInt32 count = 0;
        QCOMPARE(folder->GetNumberOfItems(&count), S_OK);
        QCOMPARE(count, UInt32(1));
        QFile original(input);
        QVERIFY(original.open(QIODevice::ReadOnly));
        QCOMPARE(original.readAll(), payload);
        QVERIFY(file.open(QIODevice::ReadOnly));
        QCOMPARE(file.readAll(), archiveBytes);
    }
};
QTEST_GUILESS_MAIN(AgentLifecycleTests)
#include "agent_lifecycle_tests.moc"
