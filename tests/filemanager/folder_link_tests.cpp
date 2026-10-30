#include "Common/StringConvert.h"
#include "filemanager/panels/Panel.h"
#include "filemanager/providers/FSFolder.h"
#include <QDir>
#include <QFile>
#include <QTemporaryDir>
#include <QtTest>

class FolderLinkTests : public QObject {
    Q_OBJECT

    static FString nativePath(const QString &path) {
        const auto native = QDir::toNativeSeparators(path).toStdWString();
        return us2fs(native.c_str());
    }
    static void createFile(const QString &path) {
        QFile file(path);
        QVERIFY(file.open(QIODevice::WriteOnly));
        QCOMPARE(file.write("original bytes"), qint64(14));
    }

  private slots:
    void cleanupRequiresExplicitOwnership_data() {
        QTest::addColumn<bool>("needDelete");
        QTest::addColumn<bool>("extraFile");
        QTest::newRow("borrowed") << false << false;
        QTest::newRow("owned") << true << false;
        QTest::newRow("owned-nonempty-parent") << true << true;
    }
    void cleanupRequiresExplicitOwnership() {
        QFETCH(bool, needDelete);
        QFETCH(bool, extraFile);
        QTemporaryDir temporary;
        QVERIFY(temporary.isValid());
        const QString directory = temporary.filePath(QStringLiteral("временная й папка"));
        QVERIFY(QDir().mkdir(directory));
        const QString path = QDir(directory).filePath(QStringLiteral("вложенный й архив.7z"));
        createFile(path);
        const QString other = QDir(directory).filePath("unrelated.txt");
        if (extraFile)
            createFile(other);
        CTempFileInfo info;
        info.NeedDelete = needDelete;
        info.FilePath = nativePath(path);
        info.FolderPath = nativePath(directory);
        info.DeleteDirAndFile();
        QCOMPARE(QFileInfo::exists(path), !needDelete);
        QCOMPARE(QFileInfo::exists(directory), !needDelete || extraFile);
        info.DeleteDirAndFile();
        QCOMPARE(QFileInfo::exists(path), !needDelete);
        if (extraFile) {
            QFile file(other);
            QVERIFY(file.open(QIODevice::ReadOnly));
            QCOMPARE(file.readAll(), QByteArray("original bytes"));
        }
    }

    void nativeSnapshotDetectsSizeAndTimestampChanges() {
        QTemporaryDir temporary;
        QVERIFY(temporary.isValid());
        const QString path = temporary.filePath(QStringLiteral("снимок й с пробелом.7z"));
        createFile(path);
        CFolderLink link;
        QVERIFY(link.FileInfo.Find(nativePath(path)));
        auto current = link.FileInfo;
        QVERIFY(!link.WasChanged_from_TempFileInfo(current));
        QVERIFY(!link.WasChanged_from_FolderLink(current));
        ++current.Size;
        QVERIFY(link.WasChanged_from_TempFileInfo(current));
        current = link.FileInfo;
#ifdef _WIN32
        current.MTime.dwLowDateTime ^= 1;
#else
        current.MTime.tv_nsec ^= 1;
#endif
        QVERIFY(link.WasChanged_from_TempFileInfo(current));
        QVERIFY(link.WasChanged_from_FolderLink(current));
        current = link.FileInfo;
#ifdef _WIN32
        current.MTime.dwHighDateTime ^= 1;
#else
        ++current.MTime.tv_sec;
#endif
        QVERIFY(link.WasChanged_from_TempFileInfo(current));
        link.IsVirtual = true;
        QVERIFY(link.WasChanged_from_FolderLink(link.FileInfo));
        QVERIFY(!link.WasChanged_from_TempFileInfo(link.FileInfo));
    }

    void parentAndZoneBufferSurviveOriginalVectorCopy() {
        QTemporaryDir temporary;
        QVERIFY(temporary.isValid());
        CMyComPtr<IFolderFolder> parent = new NFsFolder::CFSFolder(temporary.path());
        CFolderLink original;
        original.ParentFolder = parent;
        original.ParentFolderPath = L"родитель с пробелом/";
        original.VirtualPath = L"родитель с пробелом/вложенный.7z";
        const Byte zone[] = {0, 1, 127, 255};
        original.ZoneBuf.CopyFrom(zone, sizeof(zone));
        CObjectVector<CFolderLink> links;
        links.Add(original);
        parent.Release();
        original.ParentFolder.Release();
        original.ZoneBuf.NonConstData()[0] = 42;
        QVERIFY(links.Back().ParentFolder);
        QCOMPARE(links.Back().ParentFolder->LoadItems(), S_OK);
        QCOMPARE(links.Back().ZoneBuf.Size(), sizeof(zone));
        QCOMPARE(links.Back().ZoneBuf.ConstData()[0], Byte(0));
        QCOMPARE(links.Back().ZoneBuf.ConstData()[3], Byte(255));
        links.DeleteBack();
    }

    void originalAddThenLibraryAttachTransfersOwnership() {
        const QString fixture = QString::fromUtf8(NATIVE_LIBRARY_FIXTURE);
        NWindows::NDLL::CLibrary library;
        QVERIFY(library.Load(nativePath(fixture)));
        CObjectVector<CFolderLink> links;
        CFolderLink link;
        links.Add(link);
        links.Back().Library.Attach(library.Detach());
        QVERIFY(!library.IsLoaded());
        const auto value = reinterpret_cast<int (*)(void)>(
            ::GetProcAddress(links.Back().Library.Get_HMODULE(), "sevenzip_fixture_value"));
        QVERIFY(value);
        QCOMPARE(value(), 42);
        bool threw = false;
        try {
            CFolderLink forbidden(links.Back());
        } catch (int) {
            threw = true;
        }
        QVERIFY(threw);
        library.Attach(links.Back().Library.Detach());
        links.DeleteBack();
        QVERIFY(library.IsLoaded());
        QVERIFY(library.Free());
    }
};

QTEST_GUILESS_MAIN(FolderLinkTests)
#include "folder_link_tests.moc"
