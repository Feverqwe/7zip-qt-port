#include "CPP/7zip/UI/FileManager/FSFolder.h"
#include "Common/StringConvert.h"
#include <QDir>
#include <QFile>
#include <QTemporaryDir>
#include <QtTest>

class CopyCallback final : public IFolderOperationsExtractCallback, public CMyUnknownImp {
    Z7_COM_UNKNOWN_IMP_1(IFolderOperationsExtractCallback)

  public:
    bool skipExisting = false;
    UInt64 cancelAt = 0;
    UInt64 total = 0;
    UInt64 numFiles = 0;
    int asks = 0;
    QStringList messages;

    Z7_COM7F_IMF(SetTotal(UInt64 value)) override {
        total = value;
        return S_OK;
    }
    Z7_COM7F_IMF(SetCompleted(const UInt64 *value)) override {
        return value && cancelAt && *value >= cancelAt ? E_ABORT : S_OK;
    }
    Z7_COM7F_IMF(SetNumFiles(UInt64 value)) override {
        numFiles = value;
        return S_OK;
    }
    Z7_COM7F_IMF(SetCurrentFilePath(const wchar_t *)) override { return S_OK; }
    Z7_COM7F_IMF(ShowMessage(const wchar_t *message)) override {
        messages << QString::fromWCharArray(message);
        return S_OK;
    }
    Z7_COM7F_IMF(AskWrite(const wchar_t *, Int32, const FILETIME *, const UInt64 *,
                          const wchar_t *destPath, BSTR *destPathResult, Int32 *writeAnswer))
    override {
        ++asks;
        const QString target = QString::fromWCharArray(destPath);
        *writeAnswer = 1;
        if (QFileInfo::exists(target)) {
            if (skipExisting) {
                *writeAnswer = 0;
                return S_OK;
            }
            if (!QFile::remove(target))
                return E_FAIL;
        }
        return StringToBstr(destPath, destPathResult);
    }
};

class FsFolderCopyTests : public QObject {
    Q_OBJECT
    static bool write(const QString &path, const QByteArray &bytes) {
        QFile file(path);
        return file.open(QIODevice::WriteOnly) && file.write(bytes) == bytes.size();
    }
    static QByteArray read(const QString &path) {
        QFile file(path);
        if (!file.open(QIODevice::ReadOnly))
            return {};
        return file.readAll();
    }
    static HRESULT copy(NFsFolder::CFSFolder &folder, bool move, const QString &destination,
                        CopyCallback &callback) {
        const std::wstring wide = QDir::toNativeSeparators(destination).toStdWString();
        const UInt32 index = 0;
        return folder.CopyTo(BoolToInt(move), &index, 1, 0, 0, wide.c_str(), &callback);
    }

  private slots:
    void nativeCopyPreservesBytesAndRefusesOccupiedTarget() {
        QTemporaryDir temp;
        QVERIFY(temp.isValid());
        const QString source = temp.filePath(QStringLiteral("данные с пробелом.bin"));
        const QString target = temp.filePath(QStringLiteral("copy δ.bin"));
        QByteArray bytes(150000, '\0');
        for (qsizetype i = 0; i < bytes.size(); ++i)
            bytes[i] = char(i % 251);
        QVERIFY(write(source, bytes));
        const FString input = us2fs(QDir::toNativeSeparators(source).toStdWString().c_str());
        const FString output = us2fs(QDir::toNativeSeparators(target).toStdWString().c_str());
        NFsFolder::CCopyStateIO state;
        QCOMPARE(state.MyCopyFile(input, output), S_OK);
        QCOMPARE(state.ErrorFileIndex, -1);
        QCOMPARE(state.CurrentSize, quint64(bytes.size()));
        QCOMPARE(read(target), bytes);
        QVERIFY(write(target, "occupied"));
        state.DeleteSrcFile = true;
        QCOMPARE(state.MyCopyFile(input, output), S_OK);
        QCOMPARE(state.ErrorFileIndex, 1);
        QCOMPARE(state.CurrentSize, quint64(0));
        QCOMPARE(read(target), QByteArray("occupied"));
        QCOMPARE(read(source), bytes);
    }

    void nativeProgressCancellationRetainsMoveSourceAndResetsErrors() {
        QTemporaryDir temp;
        QVERIFY(temp.isValid());
        const QString source = temp.filePath("source.bin");
        const QString target = temp.filePath("partial.bin");
        const QByteArray bytes(150000, 'x');
        QVERIFY(write(source, bytes));
        const FString input = us2fs(source.toStdWString().c_str());
        const FString output = us2fs(target.toStdWString().c_str());
        CopyCallback callback;
        callback.cancelAt = 65536;
        NFsFolder::CCopyStateIO state;
        state.Progress = &callback;
        state.DeleteSrcFile = true;
        QCOMPARE(state.MyCopyFile(input, output), E_ABORT);
        QCOMPARE(state.CurrentSize, quint64(65536));
        QCOMPARE(read(source), bytes);
        QCOMPARE(read(target), bytes.left(65536));
        state.ErrorMessage = "stale";
        QVERIFY(state.MyCopyFile(temp.filePath("missing"), temp.filePath("new"),
                                 QFileDevice::Permissions{}));
        QCOMPARE(state.ErrorFileIndex, 0);
        QCOMPARE(state.CurrentSize, quint64(0));
        QVERIFY(state.ErrorMessage.isEmpty());
        QVERIFY(!QFileInfo::exists(temp.filePath("new")));
    }

    void directFilenameUsesSourceCopyToBranch() {
        QTemporaryDir temp;
        QVERIFY(temp.isValid());
        QVERIFY(QDir().mkpath(temp.filePath("source")));
        QVERIFY(QDir().mkpath(temp.filePath("destination")));
        const QString source = temp.filePath(QStringLiteral("source/δ name.txt"));
        const QString target = temp.filePath(QStringLiteral("destination/renamed.txt"));
        QVERIFY(write(source, "file bytes"));
        NFsFolder::CFSFolder folder(temp.filePath("source"));
        QCOMPARE(folder.LoadItems(), S_OK);
        CopyCallback callback;
        QCOMPARE(copy(folder, false, target, callback), S_OK);
        QCOMPARE(read(target), QByteArray("file bytes"));
        QCOMPARE(read(source), QByteArray("file bytes"));
        QCOMPARE(callback.asks, 1);
        QCOMPARE(callback.total, UInt64(10));
        QCOMPARE(callback.numFiles, UInt64(1));
    }

    void recursiveMergeAsksForNestedFile() {
        QTemporaryDir temp;
        QVERIFY(temp.isValid());
        QVERIFY(QDir().mkpath(temp.filePath("source/tree/sub")));
        QVERIFY(QDir().mkpath(temp.filePath("destination/tree/sub")));
        QVERIFY(write(temp.filePath("source/tree/sub/same.txt"), "new bytes"));
        QVERIFY(write(temp.filePath("destination/tree/sub/same.txt"), "old bytes"));
        QVERIFY(write(temp.filePath("destination/tree/untouched.txt"), "keep"));
        NFsFolder::CFSFolder folder(temp.filePath("source"));
        QCOMPARE(folder.LoadItems(), S_OK);
        CopyCallback callback;
        QCOMPARE(copy(folder, false, temp.filePath("destination/"), callback), S_OK);
        QCOMPARE(read(temp.filePath("destination/tree/sub/same.txt")), QByteArray("new bytes"));
        QCOMPARE(read(temp.filePath("destination/tree/untouched.txt")), QByteArray("keep"));
        QCOMPARE(callback.asks, 1);
        QVERIFY(callback.messages.isEmpty());
    }

    void directDirectoryMoveRemovesSource() {
        QTemporaryDir temp;
        QVERIFY(temp.isValid());
        QVERIFY(QDir().mkpath(temp.filePath("source/tree/sub")));
        QVERIFY(QDir().mkpath(temp.filePath("destination")));
        QVERIFY(write(temp.filePath("source/tree/sub/payload"), "move bytes"));
        NFsFolder::CFSFolder folder(temp.filePath("source"));
        QCOMPARE(folder.LoadItems(), S_OK);
        CopyCallback callback;
        QCOMPARE(copy(folder, true, temp.filePath("destination/new tree"), callback), S_OK);
        QCOMPARE(read(temp.filePath("destination/new tree/sub/payload")), QByteArray("move bytes"));
        QVERIFY(!QFileInfo::exists(temp.filePath("source/tree")));
    }

    void directFileMoveUsesSourceMoveBranch() {
        QTemporaryDir temp;
        QVERIFY(temp.isValid());
        QVERIFY(QDir().mkpath(temp.filePath("source")));
        QVERIFY(QDir().mkpath(temp.filePath("destination")));
        const QString source = temp.filePath(QStringLiteral("source/имя с пробелом.txt"));
        const QString target = temp.filePath("destination/renamed.txt");
        QVERIFY(write(source, "move bytes"));
        NFsFolder::CFSFolder folder(temp.filePath("source"));
        QCOMPARE(folder.LoadItems(), S_OK);
        CopyCallback callback;
        QCOMPARE(copy(folder, true, target, callback), S_OK);
        QCOMPARE(read(target), QByteArray("move bytes"));
        QVERIFY(!QFileInfo::exists(source));
    }

    void invalidSelectionDoesNotCreateDestination() {
        QTemporaryDir temp;
        QVERIFY(temp.isValid());
        QVERIFY(QDir().mkpath(temp.filePath("source")));
        NFsFolder::CFSFolder folder(temp.filePath("source"));
        QCOMPARE(folder.LoadItems(), S_OK);
        const UInt32 invalid = 5;
        const QString target = temp.filePath("created.txt");
        const std::wstring wide = target.toStdWString();
        CopyCallback callback;
        QCOMPARE(folder.CopyTo(0, &invalid, 1, 0, 0, wide.c_str(), &callback), E_INVALIDARG);
        QVERIFY(!QFileInfo::exists(target));
    }

    void cancelDuringFileCopyPreservesSourceAndClearsPartialTarget() {
        QTemporaryDir temp;
        QVERIFY(temp.isValid());
        QVERIFY(QDir().mkpath(temp.filePath("source")));
        QVERIFY(QDir().mkpath(temp.filePath("destination")));
        const QString source = temp.filePath("source/large.bin");
        const QString target = temp.filePath("destination/large.bin");
        const QByteArray bytes(1 << 18, 'x');
        QVERIFY(write(source, bytes));
        NFsFolder::CFSFolder folder(temp.filePath("source"));
        QCOMPARE(folder.LoadItems(), S_OK);
        CopyCallback callback;
        callback.cancelAt = 1 << 16;
        QCOMPARE(copy(folder, false, target, callback), E_ABORT);
        QCOMPARE(read(source), bytes);
        QVERIFY(!QFileInfo::exists(target));
    }

    void sourceCopyFileSystemItemsCopiesSelectedPaths() {
        QTemporaryDir temp;
        QVERIFY(temp.isValid());
        QVERIFY(QDir().mkpath(temp.filePath("source")));
        QVERIFY(QDir().mkpath(temp.filePath("destination")));
        const QString first = temp.filePath("source/first.txt");
        const QString second = temp.filePath(QStringLiteral("source/второй файл.txt"));
        QVERIFY(write(first, "first"));
        QVERIFY(write(second, "second"));
        UStringVector paths;
        const std::wstring firstWide = first.toStdWString();
        const std::wstring secondWide = second.toStdWString();
        paths.Add(firstWide.c_str());
        paths.Add(secondWide.c_str());
        const std::wstring destinationWide = temp.filePath("destination/").toStdWString();
        CopyCallback callback;
        QCOMPARE(
            NFsFolder::CopyFileSystemItems(paths, us2fs(destinationWide.c_str()), false, &callback),
            S_OK);
        QCOMPARE(read(temp.filePath("destination/first.txt")), QByteArray("first"));
        QCOMPARE(read(temp.filePath(QStringLiteral("destination/второй файл.txt"))),
                 QByteArray("second"));
        QCOMPARE(callback.asks, 2);
    }
};

QTEST_GUILESS_MAIN(FsFolderCopyTests)
#include "fs_folder_copy_tests.moc"
