#include "Common/StringConvert.h"
#include "Windows/FileFind.h"
#include "Windows/FileName.h"
#include "Windows/StdAfx.h"
#include "fileops/native/FileDir.h"
#include <QDir>
#include <QFile>
#include <QTemporaryDir>
#include <QtTest>

namespace {
FString pathString(const QString &path) {
    const std::wstring wide = QDir::toNativeSeparators(path).toStdWString();
    return us2fs(wide.c_str());
}
QString qtString(const FString &path) { return QString::fromWCharArray(fs2us(path)); }
} // namespace

class FileFindTests : public QObject {
    Q_OBJECT
  private slots:
    void unicodeEnumerationAndMetadata() {
        QTemporaryDir temp;
        QVERIFY(temp.isValid());
        const QString name = QString::fromUtf8("данные 📦.txt");
        QFile file(temp.filePath(name));
        QVERIFY(file.open(QIODevice::WriteOnly));
        QCOMPARE(file.write("payload"), qint64(7));
        file.close();
        QVERIFY(QDir(temp.path()).mkdir(QString::fromUtf8("папка")));
        FString prefix = pathString(temp.path());
        NWindows::NFile::NName::NormalizeDirPathPrefix(prefix);
        NWindows::NFile::NFind::CEnumerator enumerator;
        enumerator.SetDirPrefix(prefix);
        QStringList names;
        for (;;) {
            NWindows::NFile::NFind::CDirEntry entry;
            bool found = false;
            QVERIFY(enumerator.Next(entry, found));
            if (!found)
                break;
            QVERIFY(!entry.IsDots());
            names.append(qtString(entry.Name));
#ifndef _WIN32
            NWindows::NFile::NFind::CFileInfo info;
            QVERIFY(enumerator.Fill_FileInfo(entry, info, false));
#else
            const auto &info = entry;
#endif
            if (qtString(entry.Name) == name) {
                QVERIFY(!info.IsDir());
                QCOMPARE(info.Size, UInt64(7));
            } else {
                QVERIFY(info.IsDir());
            }
        }
        names.sort();
        QStringList expected{name, QString::fromUtf8("папка")};
        expected.sort();
        QCOMPARE(names, expected);
    }

    void missingDirectoryAndPathResolution() {
        QTemporaryDir temp;
        QVERIFY(temp.isValid());
        FString missing = pathString(temp.filePath("missing"));
        NWindows::NFile::NName::NormalizeDirPathPrefix(missing);
        NWindows::NFile::NFind::CEnumerator enumerator;
        enumerator.SetDirPrefix(missing);
        NWindows::NFile::NFind::CDirEntry entry;
        bool found = true;
        QVERIFY(!enumerator.Next(entry, found));
        QVERIFY(!found);
        FString current;
        QVERIFY(NWindows::NFile::NDir::GetCurrentDir(current));
        QCOMPARE(QDir(qtString(current)).canonicalPath(), QDir::current().canonicalPath());
        FString base = pathString(temp.path());
        NWindows::NFile::NName::NormalizeDirPathPrefix(base);
        FString full;
        QVERIFY(NWindows::NFile::NName::GetFullPath(base, FTEXT("child/../file"), full));
        QCOMPARE(QDir::cleanPath(qtString(full)), temp.filePath("file"));
    }

    void posixSymlinkMetadata() {
#ifdef _WIN32
        QSKIP("POSIX lstat/follow-link fixture; Windows reparse acceptance remains required.");
#else
        QTemporaryDir temp;
        QVERIFY(temp.isValid());
        const QString target = temp.filePath("target");
        QVERIFY(QDir().mkdir(target));
        const QString link = temp.filePath("link");
        QVERIFY(QFile::link(target, link));
        NWindows::NFile::NFind::CFileInfo info;
        const FString path = pathString(link);
        QVERIFY(info.Find(path, false));
        QVERIFY(info.IsPosixLink());
        QVERIFY(!info.IsDir());
        QVERIFY(info.Find(path, true));
        QVERIFY(info.IsDir());
#endif
    }
};
QTEST_GUILESS_MAIN(FileFindTests)
#include "file_find_tests.moc"
