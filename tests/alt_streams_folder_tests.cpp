#include "7zip/PropID.h"
#include "CPP/7zip/UI/FileManager/AltStreamsFolder.h"
#include "CPP/7zip/UI/FileManager/RootFolder.h"
#include "Common/StringConvert.h"
#include "Windows/PropVariant.h"
#include <QDir>
#include <QFile>
#include <QTemporaryDir>
#include <QtTest>

class StreamCallback final : public IFolderOperationsExtractCallback, public CMyUnknownImp {
    Z7_COM_UNKNOWN_IMP_1(IFolderOperationsExtractCallback)

  public:
    UInt64 cancelAt = 0;
    UInt64 total = 0;
    bool skip = false;
    QStringList errors;
    Z7_COM7F_IMF(SetTotal(UInt64 size)) override {
        total = size;
        return S_OK;
    }
    Z7_COM7F_IMF(SetCompleted(const UInt64 *value)) override {
        return value && cancelAt && *value >= cancelAt ? E_ABORT : S_OK;
    }
    Z7_COM7F_IMF(SetNumFiles(UInt64)) override { return S_OK; }
    Z7_COM7F_IMF(SetCurrentFilePath(const wchar_t *)) override { return S_OK; }
    Z7_COM7F_IMF(ShowMessage(const wchar_t *message)) override {
        errors << QString::fromWCharArray(message);
        return S_OK;
    }
    Z7_COM7F_IMF(AskWrite(const wchar_t *, Int32, const FILETIME *, const UInt64 *,
                          const wchar_t *path, BSTR *result, Int32 *answer))
    override {
        *answer = skip ? 0 : 1;
        return StringToBstr(path, result);
    }
};

class AltStreamsFolderTests : public QObject {
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
    static FString native(const QString &path) {
        return us2fs(QDir::toNativeSeparators(path).toStdWString().c_str());
    }

  private slots:
    void rootBindingListsStreamsAndPreservesBaseOnRenameDelete() {
        QTemporaryDir temp;
        QVERIFY(temp.isValid());
        const QString base = temp.filePath(QStringLiteral("файл δ с пробелом.bin"));
        QVERIFY(write(base, "main bytes"));
        QVERIFY(write(base + QStringLiteral(":поток 2"), "stream bytes"));
        CRootFolder root;
        root.Init();
        CMyComPtr<IFolderFolder> folder;
        QCOMPARE(static_cast<IFolderFolder &>(root).BindToFolder(
                     (base + ':').toStdWString().c_str(), &folder),
                 S_OK);
        QVERIFY(folder);
        QCOMPARE(folder->LoadItems(), S_OK);
        UInt32 count = 0;
        QCOMPARE(folder->GetNumberOfItems(&count), S_OK);
        QCOMPARE(count, UInt32(1));
        NWindows::NCOM::CPropVariant name, size, alt, directory;
        QCOMPARE(folder->GetProperty(0, kpidName, &name), S_OK);
        QCOMPARE(name.vt, VARTYPE(VT_BSTR));
        QCOMPARE(QString::fromWCharArray(name.bstrVal), QStringLiteral("поток 2"));
        QCOMPARE(folder->GetProperty(0, kpidSize, &size), S_OK);
        QCOMPARE(size.vt, VARTYPE(VT_UI8));
        QCOMPARE(size.uhVal.QuadPart, UInt64(12));
        QCOMPARE(folder->GetProperty(0, kpidIsAltStream, &alt), S_OK);
        QCOMPARE(alt.boolVal, VARIANT_TRUE);
        QCOMPARE(folder->GetProperty(0, kpidIsDir, &directory), S_OK);
        QCOMPARE(directory.boolVal, VARIANT_FALSE);
        CMyComPtr<IFolderOperations> operations;
        QCOMPARE(folder.QueryInterface(IID_IFolderOperations, &operations), S_OK);
        StreamCallback callback;
        QCOMPARE(operations->Rename(0, L"renamed поток", &callback), S_OK);
        QCOMPARE(read(base + QStringLiteral(":renamed поток")), QByteArray("stream bytes"));
        QVERIFY(!QFileInfo::exists(base + QStringLiteral(":поток 2")));
        QCOMPARE(folder->LoadItems(), S_OK);
        const UInt32 index = 0;
        QCOMPARE(operations->Delete(&index, 1, &callback), S_OK);
        QCOMPARE(folder->LoadItems(), S_OK);
        QCOMPARE(folder->GetNumberOfItems(&count), S_OK);
        QCOMPARE(count, UInt32(0));
        QCOMPARE(read(base), QByteArray("main bytes"));
    }

    void copyToFileAndStreamPreservesBaseAttributesAndRefusesOverwrite() {
        QTemporaryDir temp;
        QVERIFY(temp.isValid());
        const QString base = temp.filePath("source.bin");
        const QString destination = temp.filePath(QStringLiteral("назначение δ.bin"));
        QVERIFY(write(base, "source main"));
        QVERIFY(write(base + ":data", "stream bytes"));
        QVERIFY(write(destination, "destination main"));
        const FString destNative = native(destination);
        QVERIFY(::SetFileAttributesW(destNative, FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_ARCHIVE));
        NAltStreamsFolder::CAltStreamsFolder folder;
        QCOMPARE(folder.Init(native(base + ':')), S_OK);
        IFolderFolder &folderInterface = folder;
        IFolderOperations &operations = folder;
        QCOMPARE(folderInterface.LoadItems(), S_OK);
        const UInt32 index = 0;
        StreamCallback callback;
        const QString target = destination + ":copied";
        QCOMPARE(operations.CopyTo(0, &index, 1, 0, 0, target.toStdWString().c_str(), &callback),
                 S_OK);
        QCOMPARE(read(target), QByteArray("stream bytes"));
        QCOMPARE(read(destination), QByteArray("destination main"));
        QCOMPARE(::GetFileAttributesW(destNative),
                 DWORD(FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_ARCHIVE));
        QVERIFY(write(target, "occupied"));
        QCOMPARE(operations.CopyTo(0, &index, 1, 0, 0, target.toStdWString().c_str(), &callback),
                 E_ABORT);
        QCOMPARE(read(target), QByteArray("occupied"));
        QVERIFY(!callback.errors.isEmpty());
        const QString file = temp.filePath("ordinary copy.bin");
        QCOMPARE(operations.CopyTo(0, &index, 1, 0, 0, file.toStdWString().c_str(), &callback),
                 S_OK);
        QCOMPARE(read(file), QByteArray("stream bytes"));
        QCOMPARE(read(base), QByteArray("source main"));
    }

    void cancelledMoveRetainsSourceStreamAndSkipCreatesNothing() {
        QTemporaryDir temp;
        QVERIFY(temp.isValid());
        const QString base = temp.filePath("source.bin");
        const QByteArray bytes(150000, 'x');
        QVERIFY(write(base, "main"));
        QVERIFY(write(base + ":large", bytes));
        NAltStreamsFolder::CAltStreamsFolder folder;
        QCOMPARE(folder.Init(native(base + ':')), S_OK);
        IFolderFolder &folderInterface = folder;
        IFolderOperations &operations = folder;
        QCOMPARE(folderInterface.LoadItems(), S_OK);
        StreamCallback callback;
        callback.cancelAt = 65536;
        const UInt32 index = 0;
        const QString target = temp.filePath("partial.bin");
        QCOMPARE(operations.CopyTo(1, &index, 1, 0, 0, target.toStdWString().c_str(), &callback),
                 E_ABORT);
        QCOMPARE(read(base + ":large"), bytes);
        QCOMPARE(read(base), QByteArray("main"));
        QCOMPARE(read(target), bytes.left(65536));
        callback.cancelAt = 0;
        callback.skip = true;
        const QString skipped = temp.filePath("skipped.bin");
        QCOMPARE(operations.CopyTo(1, &index, 1, 0, 0, skipped.toStdWString().c_str(), &callback),
                 S_OK);
        QVERIFY(!QFileInfo::exists(skipped));
        QCOMPARE(read(base + ":large"), bytes);
    }

    void createCloneAndMoveKeepMainFileAndReportMissingDestination() {
        QTemporaryDir temp;
        QVERIFY(temp.isValid());
        const QString base = temp.filePath(QStringLiteral("main имя.bin"));
        QVERIFY(write(base, "main bytes"));
        NAltStreamsFolder::CAltStreamsFolder folder;
        QCOMPARE(folder.Init(native(base + ':')), S_OK);
        IFolderFolder &folderInterface = folder;
        IFolderOperations &operations = folder;
        IFolderClone &cloneInterface = folder;
        StreamCallback callback;
        QCOMPARE(operations.CreateFile(L"new stream", &callback), S_OK);
        QVERIFY(operations.CreateFile(L"new stream", &callback) != S_OK);
        QVERIFY(write(base + ":new stream", "move stream"));
        CMyComPtr<IFolderFolder> clone;
        QCOMPARE(cloneInterface.Clone(&clone), S_OK);
        QVERIFY(clone);
        QCOMPARE(clone->LoadItems(), S_OK);
        UInt32 count = 0;
        QCOMPARE(clone->GetNumberOfItems(&count), S_OK);
        QCOMPARE(count, UInt32(1));
        QCOMPARE(folderInterface.LoadItems(), S_OK);
        const UInt32 index = 0;
        const QString missing = temp.filePath("absent/copy.bin");
        QCOMPARE(operations.CopyTo(1, &index, 1, 0, 0, missing.toStdWString().c_str(), &callback),
                 E_ABORT);
        QVERIFY(!callback.errors.isEmpty());
        QCOMPARE(read(base + ":new stream"), QByteArray("move stream"));
        const QString target = temp.filePath(QStringLiteral("moved δ.bin"));
        QCOMPARE(operations.CopyTo(1, &index, 1, 0, 0, target.toStdWString().c_str(), &callback),
                 S_OK);
        QCOMPARE(read(target), QByteArray("move stream"));
        QCOMPARE(clone->LoadItems(), S_OK);
        QCOMPARE(clone->GetNumberOfItems(&count), S_OK);
        QCOMPARE(count, UInt32(0));
        QCOMPARE(read(base), QByteArray("main bytes"));
        NAltStreamsFolder::CAltStreamsFolder invalid;
        QVERIFY(invalid.Init(native(temp.filePath("missing.bin:"))) != S_OK);
    }
};

QTEST_GUILESS_MAIN(AltStreamsFolderTests)
#include "alt_streams_folder_tests.moc"
