#include "7zip/PropID.h"
#include "Common/MyCom.h"
#include "Windows/PropVariant.h"
#include "filemanager/providers/qtlocationfolder.h"
#include <QDir>
#include <QFile>
#include <QTemporaryDir>
#include <QtTest>

using NWindows::NCOM::CPropVariant;

class LocationFolderTests : public QObject {
    Q_OBJECT
    static int findRow(IFolderFolder *folder, const QString &name) {
        UInt32 count = 0;
        if (folder->GetNumberOfItems(&count) != S_OK)
            return -1;
        for (UInt32 i = 0; i < count; ++i) {
            CPropVariant value;
            if (folder->GetProperty(i, kpidName, &value) == S_OK && value.vt == VT_BSTR &&
                QString::fromWCharArray(value.bstrVal) == name)
                return int(i);
        }
        return -1;
    }
  private slots:
    void filesystemOwnerAndRejectedPaths() {
        QTemporaryDir temp;
        QVERIFY(temp.isValid());
        const QString name = QString::fromUtf8("папка 📦");
        QVERIFY(QDir(temp.path()).mkdir(name));
        CMyComPtr<IFolderFolder> folder;
        QCOMPARE(CreateFilesystemFolder(temp.path() + "/./", &folder), S_OK);
        QCOMPARE(folder->LoadItems(), S_OK);
        CMyComPtr<IFolderFolder> child;
        const auto wide = name.toStdWString();
        QCOMPARE(folder->BindToFolder(wide.c_str(), &child), S_OK);
        folder.Release();
        CPropVariant value;
        QCOMPARE(child->GetFolderProperty(kpidPath, &value), S_OK);
        QCOMPARE(
            QDir::cleanPath(QDir::fromNativeSeparators(QString::fromWCharArray(value.bstrVal))),
            temp.filePath(name));
        QCOMPARE(CreateFilesystemFolder(QString(), &folder), E_INVALIDARG);
        QVERIFY(!folder);
        QVERIFY(CreateFilesystemFolder(temp.filePath("missing"), &folder) != S_OK);
        QVERIFY(!folder);
        QFile file(temp.filePath("file"));
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.close();
        QVERIFY(CreateFilesystemFolder(file.fileName(), &folder) != S_OK);
        QVERIFY(!folder);
    }

    void archiveOwnerDelegatesTypedMetadataAfterInputRelease() {
        QList<ArchiveEntry> entries{{QString::fromUtf8("папка 📦/file.txt"), 17, 8},
                                    {"unknown.bin", -1, -1},
                                    {"zero.bin", 0, 0}};
        CMyComPtr<IFolderFolder> folder;
        QCOMPARE(CreateArchiveFolder(entries, UString("zip"), &folder), S_OK);
        entries.clear();
        QCOMPARE(folder->LoadItems(), S_OK);
        CPropVariant value;
        QCOMPARE(folder->GetFolderProperty(kpidSize, &value), S_OK);
        QCOMPARE(value.uhVal.QuadPart, UInt64(17));
        QCOMPARE(folder->GetFolderProperty(kpidPackSize, &value), S_OK);
        QCOMPARE(value.uhVal.QuadPart, UInt64(8));
        const int unknown = findRow(folder, "unknown.bin");
        QVERIFY(unknown >= 0);
        QCOMPARE(folder->GetProperty(UInt32(unknown), kpidSize, &value), S_OK);
        QCOMPARE(value.vt, VARTYPE(VT_EMPTY));
        QCOMPARE(folder->GetProperty(UInt32(unknown), kpidPackSize, &value), S_OK);
        QCOMPARE(value.vt, VARTYPE(VT_EMPTY));
        const int zero = findRow(folder, "zero.bin");
        QVERIFY(zero >= 0);
        QCOMPARE(folder->GetProperty(UInt32(zero), kpidSize, &value), S_OK);
        QCOMPARE(value.vt, VARTYPE(VT_UI8));
        QCOMPARE(value.uhVal.QuadPart, UInt64(0));
        CMyComPtr<IFolderFolder> child;
        QCOMPARE(folder->BindToFolder(L"папка 📦", &child), S_OK);
        folder.Release();
        QCOMPARE(child->LoadItems(), S_OK);
        QCOMPARE(child->GetProperty(0, kpidSize, &value), S_OK);
        QCOMPARE(value.uhVal.QuadPart, UInt64(17));
        QCOMPARE(child->GetProperty(0, kpidPackSize, &value), S_OK);
        QCOMPARE(value.uhVal.QuadPart, UInt64(8));
        QCOMPARE(child->GetFolderProperty(kpidType, &value), S_OK);
        QCOMPARE(QString::fromWCharArray(value.bstrVal), QStringLiteral("7-Zip.zip"));
    }

    void emptyArchiveIsAValidOwnedRoot() {
        CMyComPtr<IFolderFolder> folder;
        QCOMPARE(CreateArchiveFolder({}, UString("7z"), &folder), S_OK);
        QCOMPARE(folder->LoadItems(), S_OK);
        UInt32 count = 99;
        QCOMPARE(folder->GetNumberOfItems(&count), S_OK);
        QCOMPARE(count, UInt32(0));
        CMyComPtr<IFolderFolder> parent;
        QCOMPARE(folder->BindToParentFolder(&parent), S_OK);
        QVERIFY(!parent);
    }
};
QTEST_GUILESS_MAIN(LocationFolderTests)
#include "location_folder_tests.moc"
