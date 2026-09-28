#include "7zip/PropID.h"
#include "CPP/7zip/UI/FileManager/FSFolder.h"
#include "Common/StringConvert.h"
#include "Windows/PropVariant.h"
#include <QDir>
#include <QFile>
#include <QTemporaryDir>
#include <QtTest>

using NWindows::NCOM::CPropVariant;

class FsFolderTests : public QObject {
    Q_OBJECT
    static QString text(IFolderFolder *folder, UInt32 index, PROPID id) {
        CPropVariant value;
        if (folder->GetProperty(index, id, &value) != S_OK || value.vt != VT_BSTR)
            return {};
        return QDir::fromNativeSeparators(QString::fromWCharArray(value.bstrVal));
    }
  private slots:
    void normalFlatAndTypedMetadata() {
        QTemporaryDir temp;
        QVERIFY(temp.isValid());
        QVERIFY(QDir(temp.path()).mkpath(QString::fromUtf8("папка/sub")));
        const QString name = QString::fromUtf8("файл 📦.txt");
        QFile file(temp.filePath("папка/sub/" + name));
        QVERIFY(file.open(QIODevice::WriteOnly));
        QCOMPARE(file.write("payload"), qint64(7));
        file.close();
        auto *spec = new NFsFolder::CFSFolder(temp.path());
        CMyComPtr<IFolderFolder> folder = spec;
        CMyComPtr<IFolderSetFlatMode> flat;
        QCOMPARE(folder.QueryInterface(IID_IFolderSetFlatMode, &flat), S_OK);
        QCOMPARE(folder->LoadItems(), S_OK);
        UInt32 count = 0;
        QCOMPARE(folder->GetNumberOfItems(&count), S_OK);
        QCOMPARE(count, UInt32(1));
        QCOMPARE(text(folder, 0, kpidName), QString::fromUtf8("папка"));
        CPropVariant value;
        QCOMPARE(folder->GetProperty(0, kpidSize, &value), S_OK);
        QCOMPARE(value.vt, VARTYPE(VT_EMPTY));
        UInt32 normalProps = 0;
        QCOMPARE(folder->GetNumberOfProperties(&normalProps), S_OK);
        QCOMPARE(flat->SetFlatMode(1), S_OK);
        QCOMPARE(folder->LoadItems(), S_OK);
        QCOMPARE(folder->GetNumberOfItems(&count), S_OK);
        QCOMPARE(count, UInt32(3));
        bool found = false;
        for (UInt32 i = 0; i < count; ++i) {
            // macOS filesystems can return the source name in decomposed form.
            if (text(folder, i, kpidName).normalized(QString::NormalizationForm_C) != name)
                continue;
            found = true;
            QCOMPARE(text(folder, i, kpidPrefix), QString::fromUtf8("папка/sub/"));
            QCOMPARE(folder->GetProperty(i, kpidSize, &value), S_OK);
            QCOMPARE(value.vt, VARTYPE(VT_UI8));
            QCOMPARE(value.uhVal.QuadPart, UInt64(7));
            QCOMPARE(folder->GetProperty(i, kpidIsDir, &value), S_OK);
            QCOMPARE(value.boolVal, VARIANT_FALSE);
            QCOMPARE(folder->GetProperty(i, kpidMTime, &value), S_OK);
            QCOMPARE(value.vt, VARTYPE(VT_FILETIME));
            QCOMPARE(folder->GetProperty(i, kpidAttrib, &value), S_OK);
            QCOMPARE(value.vt, VARTYPE(VT_UI4));
        }
        QVERIFY(found);
        UInt32 flatProps = 0;
        QCOMPARE(folder->GetNumberOfProperties(&flatProps), S_OK);
        QCOMPARE(flatProps, normalProps + 1);
        BSTR label = nullptr;
        PROPID id = 0;
        VARTYPE type = VT_EMPTY;
        QCOMPARE(folder->GetPropertyInfo(flatProps - 1, &label, &id, &type), S_OK);
        QVERIFY(label == nullptr);
        QCOMPARE(id, PROPID(kpidPrefix));
        QCOMPARE(type, VARTYPE(VT_BSTR));
        QCOMPARE(folder->GetPropertyInfo(flatProps, &label, &id, &type), E_INVALIDARG);
        QCOMPARE(folder->GetProperty(count, kpidName, &value), E_INVALIDARG);
        QCOMPARE(folder->GetFolderProperty(kpidType, &value), S_OK);
        QCOMPARE(QString::fromWCharArray(value.bstrVal), QStringLiteral("FSFolder"));
        QCOMPARE(folder->GetFolderProperty(kpidPath, &value), S_OK);
        QCOMPARE(
            QDir::cleanPath(QDir::fromNativeSeparators(QString::fromWCharArray(value.bstrVal))),
            temp.path());
        QCOMPARE(flat->SetFlatMode(0), S_OK);
        QCOMPARE(folder->LoadItems(), S_OK);
        QCOMPARE(folder->GetNumberOfItems(&count), S_OK);
        QCOMPARE(count, UInt32(1));
    }

    void failedReloadClearsStaleRowsAndUnavailableInterfaces() {
        QTemporaryDir temp;
        QVERIFY(temp.isValid());
        QFile file(temp.filePath("old"));
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.close();
        auto *spec = new NFsFolder::CFSFolder(temp.path());
        CMyComPtr<IFolderFolder> folder = spec;
        QCOMPARE(folder->LoadItems(), S_OK);
        QVERIFY(QFile::remove(file.fileName()));
        QVERIFY(QDir().rmdir(temp.path()));
        QCOMPARE(folder->LoadItems(), E_FAIL);
        UInt32 count = 99;
        QCOMPARE(folder->GetNumberOfItems(&count), S_OK);
        QCOMPARE(count, UInt32(0));
        void *unsupported = nullptr;
        QCOMPARE(folder->QueryInterface(IID_IFolderOperations, &unsupported), E_NOINTERFACE);
        QVERIFY(unsupported == nullptr);
        CMyComPtr<IFolderFolder> child;
        QCOMPARE(folder->BindToFolder(L"missing", &child), E_NOTIMPL);
        QVERIFY(!child);
    }
};
QTEST_GUILESS_MAIN(FsFolderTests)
#include "fs_folder_tests.moc"
