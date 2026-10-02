#include "7zip/Common/StreamObjects.h"
#include "7zip/PropID.h"
#include "CPP/7zip/UI/Agent/Agent.h"
#include "CPP/7zip/UI/Common/OpenArchive.h"
#include "CPP/7zip/UI/FileManager/FileFolderPluginOpen.h"
#include "CPP/7zip/UI/FileManager/PasswordDialog.h"
#include "CPP/7zip/UI/FileManager/ProgressDialog2.h"
#include "Common/StringConvert.h"
#include "Windows/PropVariant.h"
#include "archive_backend_test_support.h"
#include "archivecodeccatalog.h"
#include "qtarchivefileresource.h"
#include "qtarchivefoldermanager.h"
#include "qtarchiveproperties.h"
#include "qtpanelfolder.h"
#include <QApplication>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QStandardPaths>

namespace {
class CRecordingBackend final : public ArchiveBackend {
  public:
    explicit CRecordingBackend(const QString &engine) : ArchiveBackend(nullptr, engine) {}
    QString OpenedPath;
    void openFolder(const QString &path, const QString &format, const QString &password) override {
        OpenedPath = path;
        ArchiveBackend::openFolder(path, format, password);
    }
};
} // namespace

class ArchiveOpeningTests : public QObject, protected ArchiveBackendTestSupport {
    Q_OBJECT
  private slots:
    void detectedMetadataOwnsNamesAndWriteReadiness_data() {
        QTest::addColumn<QString>("format");
        QTest::addColumn<QString>("name");
        QTest::addColumn<QString>("openType");
        QTest::addColumn<QString>("defaultName");
        QTest::addColumn<bool>("writable");
        QTest::newRow("ZIP unknown suffix") << "zip" << QStringLiteral("архив 空.unknown") << "*"
                                            << QStringLiteral("архив 空") << true;
        QTest::newRow("gzip tgz AddExt") << "gzip" << QStringLiteral("архив 空.tgz") << "*"
                                         << QStringLiteral("архив 空.tar") << true;
        QTest::newRow("gzip misleading ZIP suffix") << "gzip" << QStringLiteral("архив 空.zip")
                                                    << "*" << QStringLiteral("архив 空") << true;
        QTest::newRow("parser container")
            << "zip" << QStringLiteral("архив 空.unknown") << "#" << QString() << false;
    }
    void detectedMetadataOwnsNamesAndWriteReadiness() {
        QFETCH(QString, format);
        QFETCH(QString, name);
        QFETCH(QString, openType);
        QFETCH(QString, defaultName);
        QFETCH(bool, writable);
        const QString engine = QStandardPaths::findExecutable("7zz");
        QVERIFY2(!engine.isEmpty(), "official 7zz fixture engine is required");
        ArchiveBackend backend(nullptr, engine);
        QSignalSpy ready(&backend, &ArchiveBackend::codecCatalogReady);
        QVERIFY(backend.discoverCodecCatalog());
        QTRY_COMPARE(ready.size(), 1);
        if (!ready[0][0].toBool())
            QSKIP("Matching official module unavailable; native metadata consumer unverified");
        QTemporaryDir temporary;
        QVERIFY(temporary.isValid());
        const QString input = temporary.filePath(QStringLiteral("payload 空 with spaces.bin"));
        const QByteArray payload("binary\0payload", 14);
        writeFile(input, payload);
        const QString path = temporary.filePath(name);
        QProcess creator;
        creator.start(engine, {"a", "-t" + format, "--", path, input});
        QVERIFY(creator.waitForFinished(10000));
        QCOMPARE(creator.exitCode(), 0);
        QFile original(path);
        QVERIFY(original.open(QIODevice::ReadOnly));
        const QByteArray bytes = original.readAll();
        original.close();
        if (openType == "#")
            writeFile(path, bytes + bytes);
        CMyComPtr<IFolderManager> manager = new QtArchiveFolderManager(backend);
        CFfpOpen opening(manager);
        QCOMPARE(opening.OpenFileFolderPlugin(nullptr, us2fs(path.toStdWString().c_str()),
                                              UString(openType.toStdWString().c_str()), nullptr),
                 S_OK);
        auto *folder = dynamic_cast<CAgentFolder *>(opening.Folder.Interface());
        QVERIFY(folder);
        auto *resource = dynamic_cast<QtArchiveFileResource *>(folder->GetEngineResource());
        QVERIFY(resource);
        CArc arc;
        arc.Archive = folder->GetArchiveProperties();
        QCOMPARE(resource->ConfigureArc(arc), S_OK);
        QCOMPARE(QString::fromWCharArray(arc.DefaultName.Ptr()), defaultName);
        NWindows::NCOM::CPropVariant readOnly;
        QCOMPARE(folder->GetFolderProperty(kpidReadOnly, &readOnly), S_OK);
        QCOMPARE(readOnly.vt, VARTYPE(VT_BOOL));
        QCOMPARE(readOnly.boolVal != VARIANT_FALSE, !writable);
        QCOMPARE(resource->HasUpdateService(), writable);
        QVERIFY(original.open(QIODevice::ReadOnly));
        QCOMPARE(original.readAll(), openType == "#" ? bytes + bytes : bytes);
    }
    void nativeMetadataUsesFileTimeAndEmptyUnsupportedProperties() {
        CObjectVector<CPanelItem> items;
        for (const QString &time :
             {QStringLiteral("2026-09-29 12:34:56.1234567"),
              QStringLiteral("2026-09-29 12:34:56.1234598"), QStringLiteral("invalid timestamp")}) {
            CPanelItem item;
            item.RelPath = "input.bin";
            item.MTimeText = time;
            items.Add(item);
        }
        CMyComPtr<IFolderFolder> metadata =
            new QtPanelFolder({}, items, QtPanelFolder::PropertyCatalog::ArchiveTransport);
        CArc arc;
        QCOMPARE(metadata.QueryInterface(IID_IInArchive, &arc.Archive), S_OK);
        CArcTime first, second;
        QCOMPARE(arc.GetItem_MTime(0, first), S_OK);
        QCOMPARE(arc.GetItem_MTime(1, second), S_OK);
        QVERIFY(first.Def);
        QVERIFY(second.Def);
        QCOMPARE(first.Prec, UInt16(k_PropVar_TimePrec_Base + 7));
        const UInt64 ticks1 = (UInt64(first.FT.dwHighDateTime) << 32) | first.FT.dwLowDateTime;
        const UInt64 ticks2 = (UInt64(second.FT.dwHighDateTime) << 32) | second.FT.dwLowDateTime;
        QCOMPARE(ticks2 - ticks1, UInt64(31));
        NWindows::NCOM::CPropVariant value;
        QCOMPARE(arc.Archive->GetProperty(2, kpidMTime, &value), S_OK);
        QCOMPARE(value.vt, VARTYPE(VT_EMPTY));
        QCOMPARE(arc.Archive->GetProperty(0, kpidSymLink, &value), S_OK);
        QCOMPARE(value.vt, VARTYPE(VT_EMPTY));
    }
    void sourceBasicPropertiesDetectTailOffsetAndUnexpectedEnd() {
        QTemporaryDir temp;
        QVERIFY(temp.isValid());
        const QString path = temp.filePath("metadata.bin");
        writeFile(path, QByteArray(64, 'x'));
        CObjectVector<CPanelItem> items;
        CMyComPtr<IFolderFolder> metadata =
            new QtPanelFolder({}, items, QtPanelFolder::PropertyCatalog::ArchiveTransport);
        auto *resourceSpec = new QtArchiveFileResource(path);
        CMyComPtr<IUnknown> resource = resourceSpec;
        resourceSpec->SetArchiveMetadata(32, 0);
        CMyComPtr2_Create<IUnknown, CAgent> agent;
        QCOMPARE(agent->Init({}, metadata, UString("zip"), resource), S_OK);
        const CArc &arc = agent->GetArc();
        QVERIFY(arc.PhySize_Defined);
        QCOMPARE(arc.PhySize, UInt64(32));
        QVERIFY(arc.ErrorInfo.ThereIsTail);
        QCOMPARE(arc.ErrorInfo.TailSize, UInt64(32));
        QVERIFY(!agent->CanUpdate());
        resourceSpec->SetArchiveMetadata(48, 16);
        CArc offsetArc;
        QCOMPARE(CreateArchivePropertyTransport(metadata, &offsetArc.Archive, resource), S_OK);
        QCOMPARE(resourceSpec->ConfigureArc(offsetArc), S_OK);
        QCOMPARE(offsetArc.GetGlobalOffset(), Int64(16));
        QCOMPARE(offsetArc.AvailPhySize, UInt64(48));
        QVERIFY(!offsetArc.ErrorInfo.ThereIsTail);
        QVERIFY(!offsetArc.ErrorInfo.UnexpecedEnd);
        resourceSpec->SetArchiveMetadata(128, 0);
        QCOMPARE(resourceSpec->ConfigureArc(offsetArc), S_OK);
        QVERIFY(offsetArc.ErrorInfo.UnexpecedEnd);
        QVERIFY(!offsetArc.ErrorInfo.ThereIsTail);
    }
    void sharedNativeArchiveMetadata() {
        CObjectVector<CPanelItem> items;
        CPanelItem unnamed;
        unnamed.SizeDefined = true;
        unnamed.Size = 7;
        items.Add(unnamed);
        CPanelItem folder;
        folder.IsFolder = true;
        items.Add(folder);
        CMyComPtr<IFolderFolder> properties =
            new QtPanelFolder(QString(), items, QtPanelFolder::PropertyCatalog::ArchiveTransport);
        CArc arc;
        QCOMPARE(properties.QueryInterface(IID_IInArchive, &arc.Archive), HRESULT(S_OK));
        arc.DefaultName = L"content й space";
        UString name;
        QCOMPARE(arc.GetItem_DefaultPath(0, name), HRESULT(S_OK));
        QCOMPARE(QString::fromWCharArray(name.Ptr()), QStringLiteral("content й space"));
        QCOMPARE(arc.GetItem_DefaultPath(1, name), HRESULT(S_OK));
        QVERIFY(name.IsEmpty());
        bool isDir = false;
        QCOMPARE(Archive_IsItem_Dir(arc.Archive, 1, isDir), HRESULT(S_OK));
        QVERIFY(isDir);
        NWindows::NCOM::CPropVariant size;
        QCOMPARE(arc.Archive->GetProperty(0, kpidSize, &size), HRESULT(S_OK));
        QCOMPARE(size.vt, VARTYPE(VT_UI8));
        QCOMPARE(size.uhVal.QuadPart, UInt64(7));
        CMyComPtr<IFolderFolder> custom = new QtPanelFolder(QString(), items);
        CMyComPtr<IInArchive> unsupported;
        QCOMPARE(custom.QueryInterface(IID_IInArchive, &unsupported), HRESULT(E_NOINTERFACE));
        QVERIFY(!unsupported);
    }
    void realArchiveAndNotArchive() {
        const QString engine = QStandardPaths::findExecutable("7zz");
        QVERIFY2(!engine.isEmpty(), "official 7zz fixture engine is required");
        QTemporaryDir temp;
        QVERIFY(temp.isValid());
        const QString source = temp.filePath(QStringLiteral("имя й с пробелом.txt"));
        writeFile(source, "source data");
        const QString archive = temp.filePath(QStringLiteral("архив й space.zip"));
        QProcess creator;
        creator.start(engine, {"a", "-tzip", "--", archive, source});
        QVERIFY(creator.waitForFinished(10000));
        QCOMPARE(creator.exitCode(), 0);
        ArchiveBackend backend(nullptr, engine);
        CMyComPtr<IFolderManager> manager = new QtArchiveFolderManager(backend);
        CFfpOpen opening(manager);
        QCOMPARE(opening.OpenFileFolderPlugin(nullptr, us2fs(archive.toStdWString().c_str()),
                                              UString("zip"), nullptr),
                 HRESULT(S_OK));
        QVERIFY(opening.Folder);
        QCOMPARE(backend.lastResult(), HRESULT(S_OK));
        QCOMPARE(opening.Folder->LoadItems(), HRESULT(S_OK));
        UInt32 count = 0;
        QCOMPARE(opening.Folder->GetNumberOfItems(&count), HRESULT(S_OK));
        QCOMPARE(count, UInt32(1));
        NWindows::NCOM::CPropVariant name;
        QCOMPARE(opening.Folder->GetProperty(0, kpidName, &name), HRESULT(S_OK));
        QCOMPARE(QString::fromWCharArray(name.bstrVal).normalized(QString::NormalizationForm_C),
                 QFileInfo(source).fileName().normalized(QString::NormalizationForm_C));
        CFfpOpen plain(manager);
        QCOMPARE(plain.OpenFileFolderPlugin(nullptr, us2fs(source.toStdWString().c_str()),
                                            UString(), nullptr),
                 HRESULT(S_FALSE));
        QVERIFY(!plain.Folder);
        QVERIFY(!plain.Encrypted);
        QVERIFY(!backend.busy());
    }
    void passwordCallback_data() {
        QTest::addColumn<bool>("accept");
        QTest::addColumn<bool>("empty");
        QTest::newRow("accept") << true << false;
        QTest::newRow("empty") << true << true;
        QTest::newRow("cancel") << false << false;
    }
    void memoryStreamSpoolLifetime() {
        const QString engine = QStandardPaths::findExecutable("7zz");
        QVERIFY2(!engine.isEmpty(), "official 7zz fixture engine is required");
        QTemporaryDir temp;
        QVERIFY(temp.isValid());
        QVERIFY(QDir().mkdir(temp.filePath("inside")));
        writeFile(temp.filePath("inside/file.txt"), "payload");
        const QString archive = temp.filePath("inner.zip");
        QProcess creator;
        creator.setWorkingDirectory(temp.path());
        creator.start(engine, {"a", "-tzip", "--", archive, "inside"});
        QVERIFY(creator.waitForFinished(10000));
        QCOMPARE(creator.exitCode(), 0);
        QFile file(archive);
        QVERIFY(file.open(QIODevice::ReadOnly));
        const QByteArray bytes = file.readAll();
        auto *buffer = new CBufferInStream;
        CMyComPtr<IInStream> stream = buffer;
        buffer->Buf.CopyFrom(reinterpret_cast<const Byte *>(bytes.constData()),
                             size_t(bytes.size()));
        buffer->Init();
        CRecordingBackend backend(engine);
        CMyComPtr<IFolderManager> manager = new QtArchiveFolderManager(backend);
        CFfpOpen opening(manager);
        QCOMPARE(opening.OpenFileFolderPlugin(stream, FString("virtual-name.zip"), UString("zip"),
                                              nullptr),
                 HRESULT(S_OK));
        QVERIFY(!backend.OpenedPath.isEmpty());
        QVERIFY(backend.OpenedPath != archive);
        QVERIFY(QFileInfo::exists(backend.OpenedPath));
        CMyComPtr<IFolderFolder> child;
        QCOMPARE(opening.Folder->BindToFolder(L"inside", &child), HRESULT(S_OK));
        QVERIFY(child);
        opening.Folder.Release();
        stream.Release();
        // Every bound source folder retains CAgent and its private spool.
        QVERIFY(QFileInfo::exists(backend.OpenedPath));
        child.Release();
        QVERIFY(!QFileInfo::exists(backend.OpenedPath));
        const QByteArray invalid("not an archive");
        buffer = new CBufferInStream;
        stream = buffer;
        buffer->Buf.CopyFrom(reinterpret_cast<const Byte *>(invalid.constData()),
                             size_t(invalid.size()));
        buffer->Init();
        CFfpOpen bad(manager);
        QCOMPARE(bad.OpenFileFolderPlugin(stream, FString("bad.zip"), UString(), nullptr),
                 HRESULT(S_FALSE));
        QVERIFY(!bad.Folder);
        QVERIFY(!QFileInfo::exists(backend.OpenedPath));
        QVERIFY(!backend.busy());
    }
    void passwordCallback() {
        QFETCH(bool, accept);
        QFETCH(bool, empty);
        QTemporaryDir temp;
        QVERIFY(temp.isValid());
        const QString archive = temp.filePath("fixture.7z");
        writeFile(archive, "fixture transport input");
        qputenv("SEVENZIP_TEST_HELPER", empty ? "open-password-empty" : "open-password");
        ArchiveBackend backend(nullptr, QCoreApplication::applicationFilePath());
        CMyComPtr<IFolderManager> manager = new QtArchiveFolderManager(backend);
        CFfpOpen opening(manager);
        bool observed = false;
        QTimer responder;
        responder.setInterval(5);
        connect(&responder, &QTimer::timeout, this, [&] {
            auto *dialog = dynamic_cast<CPasswordDialog *>(QApplication::activeModalWidget());
            if (!dialog || observed)
                return;
            observed = true;
            dialog->findChild<QLineEdit *>("passwordEdit")
                ->setText(empty ? QString() : QStringLiteral("fixture пароль"));
            if (accept)
                dialog->accept();
            else
                dialog->reject();
        });
        responder.start();
        QCOMPARE(opening.OpenFileFolderPlugin(nullptr, us2fs(archive.toStdWString().c_str()),
                                              UString(), nullptr),
                 HRESULT(accept ? S_OK : E_ABORT));
        qunsetenv("SEVENZIP_TEST_HELPER");
        QVERIFY(observed);
        QCOMPARE(bool(opening.Folder), accept);
        if (accept) {
            QVERIFY(opening.Encrypted);
            QCOMPARE(QString::fromWCharArray(opening.Password.Ptr()),
                     empty ? QString() : QStringLiteral("fixture пароль"));
        }
        QVERIFY(!backend.busy());
    }
    void cancelWaitsForOwnedEngine() {
        QTemporaryDir temp;
        QVERIFY(temp.isValid());
        const QString archive = temp.filePath("fixture.7z");
        writeFile(archive, "fixture transport input");
        qputenv("SEVENZIP_TEST_HELPER", "wait");
        ArchiveBackend backend(nullptr, QCoreApplication::applicationFilePath());
        CMyComPtr<IFolderManager> manager = new QtArchiveFolderManager(backend);
        CFfpOpen opening(manager);
        bool requested = false, confirmed = false;
        QTimer responder;
        responder.setInterval(5);
        connect(&responder, &QTimer::timeout, this, [&] {
            auto *dialog = dynamic_cast<CProgressDialog *>(QApplication::activeModalWidget());
            if (dialog && backend.busy() && !requested) {
                requested = true;
                QTimer::singleShot(5, this, [&] {
                    auto *question = qobject_cast<QMessageBox *>(QApplication::activeModalWidget());
                    if (question) {
                        confirmed = true;
                        question->button(QMessageBox::Yes)->click();
                    }
                });
                dialog->findChild<QPushButton *>("IDCANCEL")->click();
            }
        });
        responder.start();
        QCOMPARE(opening.OpenFileFolderPlugin(nullptr, us2fs(archive.toStdWString().c_str()),
                                              UString(), nullptr),
                 HRESULT(E_ABORT));
        qunsetenv("SEVENZIP_TEST_HELPER");
        QVERIFY(requested);
        QVERIFY(confirmed);
        QVERIFY(!opening.Folder);
        QVERIFY(!backend.busy());
        QCOMPARE(backend.lastResult(), HRESULT(E_ABORT));
    }
    void failedStartIsNotNotArchive() {
        QTemporaryDir temp;
        QVERIFY(temp.isValid());
        const QString archive = temp.filePath("fixture.7z");
        writeFile(archive, "fixture transport input");
        ArchiveBackend backend(nullptr, "/nonexistent/sevenzip-opening-fixture");
        CMyComPtr<IFolderManager> manager = new QtArchiveFolderManager(backend);
        CFfpOpen opening(manager);
        QCOMPARE(opening.OpenFileFolderPlugin(nullptr, us2fs(archive.toStdWString().c_str()),
                                              UString(), nullptr),
                 HRESULT(E_FAIL));
        QVERIFY(!opening.Folder);
        QVERIFY(!backend.busy());
    }
};

int main(int argc, char **argv) {
    const QByteArray mode = qgetenv("SEVENZIP_TEST_HELPER");
    if (mode.startsWith("open-password")) {
        QCoreApplication app(argc, argv);
        writeOutput("Enter password:");
        char input[256];
        if (!std::fgets(input, sizeof(input), stdin))
            return 2;
        const QByteArray expected = mode == "open-password-empty"
                                        ? QByteArray()
                                        : QStringLiteral("fixture пароль").toUtf8();
        if (QByteArray(input).trimmed() != expected)
            return 2;
        writeOutput("Type = 7z\n----------\nPath = папка/имя с пробелом.txt\nSize = 3\n\n");
        return 0;
    }
    if (!mode.isEmpty()) {
        QObject unused;
        return backendTestMain(argc, argv, &unused);
    }
    QApplication app(argc, argv);
    ArchiveOpeningTests tests;
    return QTest::qExec(&tests, argc, argv);
}
#include "archive_opening_tests.moc"
