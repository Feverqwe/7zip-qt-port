#include "Common/StringConvert.h"
#include "fileops/native/FileDir.h"
#include <QDir>
#include <QFile>
#include <QTemporaryDir>
#include <QtTest>

namespace NDir = NWindows::NFile::NDir;

class FilesystemCleanupTests : public QObject {
    Q_OBJECT

    static FString nativePath(const QString &path) {
        const auto native = QDir::toNativeSeparators(path).toStdWString();
        return us2fs(native.c_str());
    }
    static void createFile(const QString &path) {
        QFile file(path);
        QVERIFY(file.open(QIODevice::WriteOnly));
        QCOMPARE(file.write("keep original bytes"), qint64(19));
    }

  private slots:
    void nativeAndQtBoundaryDeleteReadonlyUnicodeFiles_data() {
        QTest::addColumn<bool>("qtBoundary");
        QTest::newRow("source-native") << false;
        QTest::newRow("qt-boundary") << true;
    }
    void nativeAndQtBoundaryDeleteReadonlyUnicodeFiles() {
        QFETCH(bool, qtBoundary);
        QTemporaryDir temporary;
        QVERIFY(temporary.isValid());
        const QString path = temporary.filePath(QStringLiteral("readonly й с пробелом.txt"));
        createFile(path);
        QVERIFY(QFile::setPermissions(path, QFileDevice::ReadUser));
        QVERIFY(QFileInfo::exists(path));
        const bool removed =
            qtBoundary ? NDir::DeleteFileAlways(path) : NDir::DeleteFileAlways(nativePath(path));
        QVERIFY(removed);
        QVERIFY(!QFileInfo::exists(path));
        QVERIFY(!(qtBoundary ? NDir::DeleteFileAlways(path)
                             : NDir::DeleteFileAlways(nativePath(path))));
    }

    void removeDirectoryPreservesNonemptyAndFileTargets() {
        QTemporaryDir temporary;
        QVERIFY(temporary.isValid());
        const QString directory = temporary.filePath(QStringLiteral("папка й с пробелом"));
        QVERIFY(QDir().mkdir(directory));
        const QString path = QDir(directory).filePath(QStringLiteral("внутри й.txt"));
        createFile(path);
        QVERIFY(!NDir::RemoveDir(nativePath(directory)));
        QVERIFY(!NDir::RemoveDir(nativePath(path)));
        QFile file(path);
        QVERIFY(file.open(QIODevice::ReadOnly));
        QCOMPARE(file.readAll(), QByteArray("keep original bytes"));
        file.close();
        QVERIFY(NDir::DeleteFileAlways(nativePath(path)));
        QVERIFY(NDir::RemoveDir(nativePath(directory)));
        QVERIFY(!QFileInfo::exists(directory));
        QVERIFY(!NDir::RemoveDir(nativePath(directory)));
    }

    void emptyDirectoryDeletionRetainsSourcePlatformContract() {
        QTemporaryDir temporary;
        QVERIFY(temporary.isValid());
        const QString directory = temporary.filePath(QStringLiteral("пустая й"));
        QVERIFY(QDir().mkdir(directory));
#ifdef _WIN32
        QVERIFY(!NDir::DeleteFileAlways(nativePath(directory)));
        QVERIFY(QFileInfo::exists(directory));
        QVERIFY(NDir::RemoveDir(nativePath(directory)));
#else
        // Original POSIX DeleteFileAlways uses remove(), which also removes an
        // empty directory. Do not substitute Windows semantics in this port.
        QVERIFY(NDir::DeleteFileAlways(nativePath(directory)));
#endif
        QVERIFY(!QFileInfo::exists(directory));
    }

    void symbolicLinkCleanupDoesNotTouchTarget() {
#ifdef _WIN32
        QSKIP("POSIX DeleteFileAlways directory-link contract.");
#else
        QTemporaryDir temporary;
        QVERIFY(temporary.isValid());
        const QString directory = temporary.filePath(QStringLiteral("target й с пробелом"));
        QVERIFY(QDir().mkdir(directory));
        const QString filePath = QDir(directory).filePath("keep.txt");
        createFile(filePath);
        QVERIFY(QFile::setPermissions(filePath, QFileDevice::ReadUser));
        const QString fileLink = temporary.filePath("file link");
        const QString directoryLink = temporary.filePath("directory link");
        QVERIFY(QFile::link(filePath, fileLink));
        QVERIFY(QFile::link(directory, directoryLink));
        QVERIFY(QFileInfo(fileLink).isSymLink() && QFileInfo(directoryLink).isSymLink());
        QVERIFY(!NDir::RemoveDir(nativePath(directoryLink)));
        QVERIFY(NDir::DeleteFileAlways(nativePath(fileLink)));
        QVERIFY(NDir::DeleteFileAlways(directoryLink));
        QVERIFY(!QFileInfo(fileLink).isSymLink() && !QFileInfo(directoryLink).isSymLink());
        QVERIFY(QFileInfo::exists(directory));
        QFile file(filePath);
        QVERIFY(file.open(QIODevice::ReadOnly));
        QCOMPARE(file.readAll(), QByteArray("keep original bytes"));
#endif
    }
};

QTEST_GUILESS_MAIN(FilesystemCleanupTests)
#include "filesystem_cleanup_tests.moc"
