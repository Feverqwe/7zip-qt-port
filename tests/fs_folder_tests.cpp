#include "7zip/PropID.h"
#include "CPP/7zip/UI/FileManager/FSDrives.h"
#include "CPP/7zip/UI/FileManager/FSFolder.h"
#include "Common/StringConvert.h"
#include "Windows/PropVariant.h"
#include <QDir>
#include <QFile>
#include <QTemporaryDir>
#include <QtTest>
#include <new>

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
#ifndef _WIN32
    void flatListingDoesNotFollowDirectoryLinks() {
        QTemporaryDir temp;
        QVERIFY(temp.isValid());
        QVERIFY(QDir(temp.path()).mkdir("nested"));
        QFile file(temp.filePath("nested/payload"));
        QVERIFY(file.open(QIODevice::WriteOnly));
        QCOMPARE(file.write("fixture"), qint64(7));
        file.close();
        QVERIFY(QFile::link(temp.path(), temp.filePath("nested/cycle")));
        const QString fileTarget = temp.filePath("nested/payload");
        QVERIFY(QFile::link(fileTarget, temp.filePath("file-link")));
        CMyComPtr<IFolderFolder> folder = new NFsFolder::CFSFolder(temp.path());
        CMyComPtr<IFolderSetFlatMode> flat;
        QCOMPARE(folder.QueryInterface(IID_IFolderSetFlatMode, &flat), S_OK);
        QCOMPARE(flat->SetFlatMode(1), S_OK);
        QCOMPARE(folder->LoadItems(), S_OK);
        UInt32 count = 0;
        QCOMPARE(folder->GetNumberOfItems(&count), S_OK);
        QCOMPARE(count, UInt32(4));
        bool foundCycle = false, foundFileLink = false;
        for (UInt32 i = 0; i < count; ++i) {
            const QString name = text(folder, i, kpidName);
            if (name != "cycle" && name != "file-link")
                continue;
            CPropVariant value;
            QCOMPARE(folder->GetProperty(i, kpidIsDir, &value), S_OK);
            QCOMPARE(value.vt, VARTYPE(VT_BOOL));
            QCOMPARE(value.boolVal, VARIANT_FALSE);
            QCOMPARE(folder->GetProperty(i, kpidSize, &value), S_OK);
            QCOMPARE(value.vt, VARTYPE(VT_UI8));
            const QString target = name == "cycle" ? temp.path() : fileTarget;
            QCOMPARE(value.uhVal.QuadPart, UInt64(QFile::encodeName(target).size()));
            foundCycle |= name == "cycle";
            foundFileLink |= name == "file-link";
        }
        QVERIFY(foundCycle && foundFileLink);
    }
#endif
    void cancelledOrFailedLoadClearsPartialState_data() {
        QTest::addColumn<int>("failure");
        QTest::newRow("before-enumeration") << 0;
        QTest::newRow("during-flat-recursion") << 1;
        QTest::newRow("progress-exception") << 2;
    }
    void cancelledOrFailedLoadClearsPartialState() {
        QFETCH(int, failure);
        QTemporaryDir temp;
        QVERIFY(temp.isValid());
        const QString nested = QStringLiteral("папка с пробелом/sub");
        QVERIFY(QDir(temp.path()).mkpath(nested));
        for (const QString &name :
             {QStringLiteral("root.txt"), nested + QStringLiteral("/файл 📦.txt")}) {
            QFile file(temp.filePath(name));
            QVERIFY(file.open(QIODevice::WriteOnly));
            QCOMPARE(file.write("fixture"), qint64(7));
        }
        auto *spec = new NFsFolder::CFSFolder(temp.path());
        CMyComPtr<IFolderFolder> folder = spec;
        QCOMPARE(spec->SetFlatMode(1), S_OK);
        QCOMPARE(folder->LoadItems(), S_OK);
        UInt32 count = 0;
        QCOMPARE(folder->GetNumberOfItems(&count), S_OK);
        QCOMPARE(count, UInt32(4));
        quint64 completed = 0;
        bool orderedProgress = true;
        bool reachedNested = false;
        spec->CheckBreak = [failure](const QString &) { return failure != 0; };
        spec->SetCompleted = [&](quint64 value) {
            orderedProgress &= value == completed + 1;
            completed = value;
            if (value < 3)
                return true;
            reachedNested = spec->Files.Back().Parent >= 0;
            if (failure == 2)
                throw std::bad_alloc();
            return false;
        };
        const HRESULT expected = failure == 2 ? E_OUTOFMEMORY : E_ABORT;
        QCOMPARE(folder->LoadItems(), expected);
        QVERIFY(orderedProgress);
        QCOMPARE(completed, failure == 0 ? quint64(0) : quint64(3));
        QCOMPARE(reachedNested, failure != 0);
        QCOMPARE(folder->GetNumberOfItems(&count), S_OK);
        QCOMPARE(count, UInt32(0));
        QVERIFY(spec->Folders.IsEmpty());
        spec->CheckBreak = {};
        spec->SetCompleted = {};
        QCOMPARE(folder->LoadItems(), S_OK);
        QCOMPARE(folder->GetNumberOfItems(&count), S_OK);
        QCOMPARE(count, UInt32(4));
    }
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
        QVERIFY(folder->BindToFolder(L"missing", &child) != S_OK);
        QVERIFY(!child);
    }

    void bindingParentRootAndFailure() {
        QTemporaryDir temp;
        QVERIFY(temp.isValid());
        const QString name = QString::fromUtf8("папка 📦");
        QVERIFY(QDir(temp.path()).mkdir(name));
        QFile file(temp.filePath("file"));
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.close();
        auto *spec = new NFsFolder::CFSFolder(temp.path());
        CMyComPtr<IFolderFolder> folder = spec;
        QCOMPARE(folder->LoadItems(), S_OK);
        const auto wide = name.toStdWString();
        CMyComPtr<IFolderFolder> child;
        QCOMPARE(folder->BindToFolder(wide.c_str(), &child), S_OK);
        QVERIFY(child);
        QCOMPARE(child->LoadItems(), S_OK);
        CPropVariant value;
        QCOMPARE(child->GetFolderProperty(kpidPath, &value), S_OK);
        QCOMPARE(
            QDir::cleanPath(QDir::fromNativeSeparators(QString::fromWCharArray(value.bstrVal))),
            temp.filePath(name));
        CMyComPtr<IFolderFolder> parent;
        QCOMPARE(child->BindToParentFolder(&parent), S_OK);
        QVERIFY(parent);
        QCOMPARE(parent->GetFolderProperty(kpidPath, &value), S_OK);
        QCOMPARE(
            QDir::cleanPath(QDir::fromNativeSeparators(QString::fromWCharArray(value.bstrVal))),
            temp.path());
        UInt32 count = 0;
        QCOMPARE(folder->GetNumberOfItems(&count), S_OK);
        for (UInt32 i = 0; i < count; ++i) {
            CMyComPtr<IFolderFolder> indexed;
            if (text(folder, i, kpidName) == name) {
                QCOMPARE(folder->BindToFolder(i, &indexed), S_OK);
                QVERIFY(indexed);
            } else {
                QCOMPARE(folder->BindToFolder(i, &indexed), E_INVALIDARG);
                QVERIFY(!indexed);
            }
        }
        QCOMPARE(folder->BindToFolder(count, &child), E_INVALIDARG);
        QVERIFY(!child);
        QVERIFY(folder->BindToFolder(L"file", &child) != S_OK);
        QVERIFY(!child);
        QVERIFY(folder->BindToFolder(L"missing", &child) != S_OK);
        QVERIFY(!child);
        CMyComPtr<IFolderWasChanged> changed;
        QCOMPARE(folder.QueryInterface(IID_IFolderWasChanged, &changed), S_OK);
        Int32 wasChanged = 99;
        QCOMPARE(changed->WasChanged(&wasChanged), S_OK);
        QCOMPARE(wasChanged, Int32(0));
#ifdef _WIN32
        const QString rootPath = QDir::toNativeSeparators(temp.path()).left(3);
#else
        const QString rootPath = QStringLiteral("/");
#endif
        CMyComPtr<IFolderFolder> root = new NFsFolder::CFSFolder(rootPath);
        CMyComPtr<IFolderFolder> above;
        QCOMPARE(root->BindToParentFolder(&above), S_OK);
#ifdef _WIN32
        QVERIFY(above);
        QCOMPARE(above->GetFolderProperty(kpidType, &value), S_OK);
        QCOMPARE(QString::fromWCharArray(value.bstrVal), QStringLiteral("FSDrives"));
        CMyComPtr<IFolderFolder> rebound;
        const auto rootWide = rootPath.toStdWString();
        QCOMPARE(above->BindToFolder(rootWide.c_str(), &rebound), S_OK);
        QVERIFY(rebound);
#else
        QVERIFY(!above);
#endif
        CMyComPtr<IFolderFolder> unbound = new NFsFolder::CFSFolder;
        QCOMPARE(unbound->BindToParentFolder(&above), E_INVALIDARG);
        QVERIFY(!above);
    }

    void flatIndexBindingAndChildMode() {
        QTemporaryDir temp;
        QVERIFY(temp.isValid());
        QVERIFY(QDir(temp.path()).mkpath("a/b/c"));
        CMyComPtr<IFolderFolder> folder = new NFsFolder::CFSFolder(temp.path());
        CMyComPtr<IFolderSetFlatMode> flat;
        QCOMPARE(folder.QueryInterface(IID_IFolderSetFlatMode, &flat), S_OK);
        QCOMPARE(flat->SetFlatMode(1), S_OK);
        QCOMPARE(folder->LoadItems(), S_OK);
        UInt32 count = 0;
        QCOMPARE(folder->GetNumberOfItems(&count), S_OK);
        QCOMPARE(count, UInt32(3));
        bool found = false;
        for (UInt32 i = 0; i < count; ++i) {
            if (text(folder, i, kpidName) != "b")
                continue;
            found = true;
            CMyComPtr<IFolderFolder> child;
            QCOMPARE(folder->BindToFolder(i, &child), S_OK);
            QCOMPARE(child->LoadItems(), S_OK);
            UInt32 childCount = 0;
            QCOMPARE(child->GetNumberOfItems(&childCount), S_OK);
            QCOMPARE(childCount, UInt32(1));
            QCOMPARE(text(child, 0, kpidName), QStringLiteral("c"));
        }
        QVERIFY(found);
    }
};
QTEST_GUILESS_MAIN(FsFolderTests)
#include "fs_folder_tests.moc"
