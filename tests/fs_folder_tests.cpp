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
#ifndef Q_OS_WIN
#include <sys/stat.h>
#include <unistd.h>
#endif

using NWindows::NCOM::CPropVariant;

class FolderStatProgress final : public IProgress, public CMyUnknownImp {
    Z7_COM_UNKNOWN_IMP_1(IProgress)

  public:
    bool cancel = false;
    Z7_COM7F_IMF(SetTotal(UInt64)) override { return S_OK; }
    Z7_COM7F_IMF(SetCompleted(const UInt64 *)) override { return cancel ? E_ABORT : S_OK; }
};

class FsFolderTests : public QObject {
    Q_OBJECT
    static QString text(IFolderFolder *folder, UInt32 index, PROPID id) {
        CPropVariant value;
        if (folder->GetProperty(index, id, &value) != S_OK || value.vt != VT_BSTR)
            return {};
        return QDir::fromNativeSeparators(QString::fromWCharArray(value.bstrVal));
    }
  private slots:
    void directoryStatisticsRetainSourceCountsAndCancel() {
        QTemporaryDir temp;
        QVERIFY(temp.isValid());
        QVERIFY(QDir().mkpath(temp.filePath(QStringLiteral("папка с пробелом/sub"))));
        QFile file(temp.filePath(QStringLiteral("папка с пробелом/sub/payload")));
        QVERIFY(file.open(QIODevice::WriteOnly));
        QCOMPARE(file.write("payload"), qint64(7));
        file.close();
        CMyComPtr<IFolderFolder> folder = new NFsFolder::CFSFolder(temp.path());
        QCOMPARE(folder->LoadItems(), S_OK);
        UInt32 count = 0;
        QCOMPARE(folder->GetNumberOfItems(&count), S_OK);
        QCOMPARE(count, UInt32(1));
        CMyComPtr<IFolderCalcItemFullSize> statistics;
        QCOMPARE(folder.QueryInterface(IID_IFolderCalcItemFullSize, &statistics), S_OK);
        CPropVariant value;
        QCOMPARE(folder->GetProperty(0, kpidSize, &value), S_OK);
        QCOMPARE(value.vt, VARTYPE(VT_EMPTY));
        CMyComPtr<IProgress> progress = new FolderStatProgress;
        auto *control = static_cast<FolderStatProgress *>((IProgress *)progress);
        control->cancel = true;
        QCOMPARE(statistics->CalcItemFullSize(0, progress), E_ABORT);
        QCOMPARE(folder->GetProperty(0, kpidNumSubFiles, &value), S_OK);
        QCOMPARE(value.vt, VARTYPE(VT_EMPTY));
        control->cancel = false;
        QCOMPARE(statistics->CalcItemFullSize(0, progress), S_OK);
        QCOMPARE(folder->GetProperty(0, kpidNumSubDirs, &value), S_OK);
        QCOMPARE(value.uhVal.QuadPart, UInt64(1));
        QCOMPARE(folder->GetProperty(0, kpidNumSubFiles, &value), S_OK);
        QCOMPARE(value.uhVal.QuadPart, UInt64(1));
        QCOMPARE(folder->GetProperty(0, kpidSize, &value), S_OK);
        QCOMPARE(value.uhVal.QuadPart, UInt64(7));
    }
    void descriptionCommentsUseSourceParserAndReload() {
        QTemporaryDir temp;
        QVERIFY(temp.isValid());
        QFile item(temp.filePath(QStringLiteral("file with spaces.txt")));
        QVERIFY(item.open(QIODevice::WriteOnly));
        item.close();
        QFile description(temp.filePath(QStringLiteral("descript.ion")));
        QVERIFY(description.open(QIODevice::WriteOnly));
        QCOMPARE(
            description.write("\xEF\xBB\xBF\r\n\"file with spaces.txt\"  first\x04metadata\r\n"),
            qint64(45));
        description.close();
        CMyComPtr<IFolderFolder> folder = new NFsFolder::CFSFolder(temp.path());
        QCOMPARE(folder->LoadItems(), S_OK);
        UInt32 count = 0;
        QCOMPARE(folder->GetNumberOfItems(&count), S_OK);
        UInt32 itemIndex = count;
        for (UInt32 index = 0; index < count; ++index)
            if (text(folder, index, kpidName) == QStringLiteral("file with spaces.txt"))
                itemIndex = index;
        QVERIFY(itemIndex < count);
        QCOMPARE(text(folder, itemIndex, kpidComment), QStringLiteral("first"));
        QVERIFY(description.open(QIODevice::WriteOnly | QIODevice::Truncate));
        QCOMPARE(description.write("\"file with spaces.txt\" second\r\n"), qint64(31));
        description.close();
        QCOMPARE(text(folder, itemIndex, kpidComment), QStringLiteral("first"));
        QCOMPARE(folder->LoadItems(), S_OK);
        QCOMPARE(text(folder, itemIndex, kpidComment), QStringLiteral("second"));
    }
    void packedSizeUsesSourceCacheAndMissingFileFallback_data() {
        QTest::addColumn<bool>("flat");
        QTest::addColumn<bool>("cached");
        for (bool flat : {false, true})
            for (bool cached : {false, true})
                QTest::newRow(qPrintable(QString("flat%1-cached%2").arg(flat).arg(cached)))
                    << flat << cached;
    }
    void packedSizeUsesSourceCacheAndMissingFileFallback() {
        QFETCH(bool, flat);
        QFETCH(bool, cached);
        QTemporaryDir temp;
        QVERIFY(temp.isValid());
        QVERIFY(QDir(temp.path()).mkdir("directory"));
        const QString name = QStringLiteral("cache й with spaces");
        QFile file(temp.filePath(name));
        QVERIFY(file.open(QIODevice::WriteOnly));
        QCOMPARE(file.write("fixture"), qint64(7));
        file.close();
        CMyComPtr<IFolderFolder> folder = new NFsFolder::CFSFolder(temp.path());
        CMyComPtr<IFolderSetFlatMode> mode;
        QCOMPARE(folder.QueryInterface(IID_IFolderSetFlatMode, &mode), S_OK);
        QCOMPARE(mode->SetFlatMode(flat), S_OK);
        QCOMPARE(folder->LoadItems(), S_OK);
        UInt32 count = 0;
        QCOMPARE(folder->GetNumberOfItems(&count), S_OK);
        QCOMPARE(count, UInt32(2));
        UInt32 fileIndex = count;
        CPropVariant value;
        for (UInt32 i = 0; i < count; ++i) {
            if (text(folder, i, kpidName).normalized(QString::NormalizationForm_C) == name)
                fileIndex = i;
            else {
                QCOMPARE(folder->GetProperty(i, kpidSize, &value), S_OK);
                QCOMPARE(value.vt, VARTYPE(VT_EMPTY));
                QCOMPARE(folder->GetProperty(i, kpidPackSize, &value), S_OK);
                QCOMPARE(value.vt, VARTYPE(VT_UI8));
                QCOMPARE(value.uhVal.QuadPart, UInt64(0));
            }
        }
        QVERIFY(fileIndex < count);
        UInt64 firstPacked = 7;
        if (cached) {
            QCOMPARE(folder->GetProperty(fileIndex, kpidPackSize, &value), S_OK);
            QCOMPARE(value.vt, VARTYPE(VT_UI8));
            firstPacked = value.uhVal.QuadPart;
        }
        QVERIFY(QFile::remove(file.fileName()));
        QCOMPARE(folder->GetProperty(fileIndex, kpidPackSize, &value), S_OK);
        QCOMPARE(value.vt, VARTYPE(VT_UI8));
        QCOMPARE(value.uhVal.QuadPart, firstPacked);
        QCOMPARE(folder->GetProperty(fileIndex, kpidSize, &value), S_OK);
        QCOMPARE(value.uhVal.QuadPart, UInt64(7));
        QCOMPARE(folder->LoadItems(), S_OK);
        QCOMPARE(folder->GetNumberOfItems(&count), S_OK);
        QCOMPARE(count, UInt32(1));
    }
#ifndef Q_OS_WIN
    void flatListingKeepsUnreadableChildAndVisibleSiblings() {
        QTemporaryDir temp;
        QVERIFY(temp.isValid());
        const QString denied = temp.filePath(QStringLiteral("denied"));
        QVERIFY(QDir().mkdir(denied));
        QVERIFY(QDir().mkdir(temp.filePath(QStringLiteral("empty"))));
        QFile hidden(temp.filePath(QStringLiteral(".hidden")));
        QVERIFY(hidden.open(QIODevice::WriteOnly));
        hidden.close();
        QFile visible(temp.filePath(QStringLiteral("видимый файл")));
        QVERIFY(visible.open(QIODevice::WriteOnly));
        visible.close();
        const QByteArray deniedPath = QFile::encodeName(denied);
        QCOMPARE(::chmod(deniedPath.constData(), 0), 0);
        if (::access(deniedPath.constData(), R_OK) == 0) {
            ::chmod(deniedPath.constData(), 0700);
            QSKIP("This process can still read mode-000 directories");
        }
        CMyComPtr<IFolderFolder> folder = new NFsFolder::CFSFolder(temp.path());
        CMyComPtr<IFolderSetFlatMode> flat;
        QCOMPARE(folder.QueryInterface(IID_IFolderSetFlatMode, &flat), S_OK);
        QCOMPARE(flat->SetFlatMode(1), S_OK);
        const HRESULT result = folder->LoadItems();
        ::chmod(deniedPath.constData(), 0700);
        QCOMPARE(result, S_OK);
        UInt32 count = 0;
        QCOMPARE(folder->GetNumberOfItems(&count), S_OK);
        QCOMPARE(count, UInt32(4));
        QStringList names;
        for (UInt32 index = 0; index < count; ++index)
            names.append(text(folder, index, kpidName).normalized(QString::NormalizationForm_C));
        QVERIFY(names.contains(QStringLiteral("denied")));
        QVERIFY(names.contains(QStringLiteral("empty")));
        QVERIFY(names.contains(QStringLiteral(".hidden")));
        QVERIFY(names.contains(QStringLiteral("видимый файл")));
    }
    void hardLinksExposeSourceFileIdentityAndChangeTime() {
        QTemporaryDir temp;
        QVERIFY(temp.isValid());
        QFile original(temp.filePath(QStringLiteral("original")));
        QVERIFY(original.open(QIODevice::WriteOnly));
        QCOMPARE(original.write("payload"), qint64(7));
        original.close();
        const QByteArray source = QFile::encodeName(original.fileName());
        const QByteArray linked = QFile::encodeName(temp.filePath(QStringLiteral("linked")));
        QCOMPARE(::link(source.constData(), linked.constData()), 0);
        CMyComPtr<IFolderFolder> folder = new NFsFolder::CFSFolder(temp.path());
        QCOMPARE(folder->LoadItems(), S_OK);
        UInt32 count = 0;
        QCOMPARE(folder->GetNumberOfItems(&count), S_OK);
        QCOMPARE(count, UInt32(2));
        UInt64 inode = 0;
        for (UInt32 index = 0; index < count; ++index) {
            CPropVariant links, identity, changed;
            QCOMPARE(folder->GetProperty(index, kpidLinks, &links), S_OK);
            QCOMPARE(links.vt, VARTYPE(VT_UI4));
            QCOMPARE(links.ulVal, UInt32(2));
            QCOMPARE(folder->GetProperty(index, kpidINode, &identity), S_OK);
            QCOMPARE(identity.vt, VARTYPE(VT_UI8));
            QVERIFY(identity.uhVal.QuadPart != 0);
            if (index == 0)
                inode = identity.uhVal.QuadPart;
            else
                QCOMPARE(identity.uhVal.QuadPart, inode);
            QCOMPARE(folder->GetProperty(index, kpidChangeTime, &changed), S_OK);
            QCOMPARE(changed.vt, VARTYPE(VT_FILETIME));
        }
    }
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
        spec->CheckBreak = [failure](const QString &) { return failure != 0; };
        spec->SetCompleted = [&](quint64 value) {
            orderedProgress &= value >= completed;
            completed = value;
            if (value < 3)
                return true;
            if (failure == 2)
                throw std::bad_alloc();
            return false;
        };
        const HRESULT expected = failure == 2 ? E_OUTOFMEMORY : E_ABORT;
        QCOMPARE(folder->LoadItems(), expected);
        QVERIFY(orderedProgress);
        if (failure == 0)
            QCOMPARE(completed, quint64(0));
        else
            QVERIFY(completed >= 3);
        QCOMPARE(folder->GetNumberOfItems(&count), S_OK);
        QCOMPARE(count, UInt32(0));
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
        CMyComPtr<IFolderClone> cloneOwner;
        QCOMPARE(folder.QueryInterface(IID_IFolderClone, &cloneOwner), S_OK);
        CMyComPtr<IFolderFolder> clone;
        QCOMPARE(cloneOwner->Clone(&clone), S_OK);
        QVERIFY(clone);
        QCOMPARE(clone->LoadItems(), S_OK);
        UInt32 cloneCount = 0;
        QCOMPARE(clone->GetNumberOfItems(&cloneCount), S_OK);
        QCOMPARE(cloneCount, count);
        QCOMPARE(text(folder, 0, kpidName), QString::fromUtf8("папка"));
        CPropVariant value;
        QCOMPARE(folder->GetProperty(0, kpidSize, &value), S_OK);
        QCOMPARE(value.vt, VARTYPE(VT_EMPTY));
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
        BSTR label = nullptr;
        PROPID id = 0;
        VARTYPE type = VT_EMPTY;
        bool hasPrefix = false;
        for (UInt32 i = 0; i < flatProps; ++i) {
            QCOMPARE(folder->GetPropertyInfo(i, &label, &id, &type), S_OK);
            SysFreeString(label);
            label = nullptr;
            if (id == kpidPrefix) {
                hasPrefix = true;
                QCOMPARE(type, VARTYPE(VT_BSTR));
            }
        }
        QVERIFY(hasPrefix);
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

    void missingReloadUsesSourceEmptyEnumerationAndUnavailableInterfaces() {
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
        QCOMPARE(folder->LoadItems(), S_OK);
        UInt32 count = 99;
        QCOMPARE(folder->GetNumberOfItems(&count), S_OK);
        QCOMPARE(count, UInt32(0));
        void *unsupported = nullptr;
        QCOMPARE(folder->QueryInterface(IID_IFolderAltStreams, &unsupported), E_NOINTERFACE);
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
