#include "7zip/PropID.h"
#include "CPP/7zip/UI/FileManager/Panel.h"
#include "Common/StringConvert.h"
#include "Windows/PropVariant.h"
#include "qtlocationfolder.h"
#include <QDir>
#include <QFile>
#include <QTemporaryDir>
#include <QtTest>

using NWindows::NCOM::CPropVariant;

// Public folder contract fixture for source classification/drive-prefix branches.
class PathFolder final : public IFolderFolder, public CMyUnknownImp {
    Z7_COM_UNKNOWN_IMP_1(IFolderFolder)
  public:
    Z7_IFACE_COM7_IMP(IFolderFolder)
    PathFolder() { Type = "FSDrives"; }
    CPropVariant Type;
    CPropVariant Name = L"C:";
    CPropVariant Hash;
    HRESULT TypeResult = S_OK;
    HRESULT NameResult = S_OK;
};
Z7_COM7F_IMF(PathFolder::LoadItems()) { return S_OK; }
Z7_COM7F_IMF(PathFolder::GetNumberOfItems(UInt32 *count)) {
    *count = 1;
    return S_OK;
}
Z7_COM7F_IMF(PathFolder::GetProperty(UInt32 index, PROPID id, PROPVARIANT *value)) {
    if (index != 0)
        return E_INVALIDARG;
    CPropVariant prop;
    if (id == kpidName) {
        if (NameResult != S_OK)
            return NameResult;
        prop = Name;
    }
    return prop.Detach(value);
}
Z7_COM7F_IMF(PathFolder::GetFolderProperty(PROPID id, PROPVARIANT *value)) {
    CPropVariant prop;
    if (id == kpidType) {
        if (TypeResult != S_OK)
            return TypeResult;
        prop = Type;
    } else if (id == kpidIsHash)
        prop = Hash;
    return prop.Detach(value);
}
Z7_COM7F_IMF(PathFolder::GetNumberOfProperties(UInt32 *count)) {
    *count = 0;
    return S_OK;
}
Z7_COM7F_IMF(PathFolder::GetPropertyInfo(UInt32, BSTR *, PROPID *, VARTYPE *)) { return E_NOTIMPL; }
Z7_COM7F_IMF(PathFolder::BindToFolder(UInt32, IFolderFolder **folder)) {
    *folder = nullptr;
    return E_NOTIMPL;
}
Z7_COM7F_IMF(PathFolder::BindToFolder(const wchar_t *, IFolderFolder **folder)) {
    *folder = nullptr;
    return E_NOTIMPL;
}
Z7_COM7F_IMF(PathFolder::BindToParentFolder(IFolderFolder **folder)) {
    *folder = nullptr;
    return E_NOTIMPL;
}

class PanelPathTests : public QObject {
    Q_OBJECT

  private slots:
    void nativePathsRetainTheLoadedSourcePrefix_data() {
        QTest::addColumn<bool>("flatMode");
        QTest::newRow("normal") << false;
        QTest::newRow("flat") << true;
    }
    void nativePathsRetainTheLoadedSourcePrefix() {
        QFETCH(bool, flatMode);
        QTemporaryDir temporary;
        QVERIFY(temporary.isValid());
        const QDir root(QFileInfo(temporary.path()).canonicalFilePath());
        const QString directory = QStringLiteral("папка й с пробелом");
        const QString name = QStringLiteral("файл й.txt");
        QVERIFY(root.mkdir(directory));
        for (const QString &relative : {name, directory + '/' + name}) {
            QFile file(root.filePath(relative));
            QVERIFY(file.open(QIODevice::WriteOnly));
            QCOMPARE(file.write("fixture"), qint64(7));
        }
        CMyComPtr<IFolderFolder> owner;
        QCOMPARE(CreateFilesystemFolder(root.path(), &owner), S_OK);
        CMyComPtr<IFolderSetFlatMode> flat;
        QCOMPARE(owner.QueryInterface(IID_IFolderSetFlatMode, &flat), S_OK);
        QCOMPARE(flat->SetFlatMode(flatMode ? 1 : 0), S_OK);
        QCOMPARE(owner->LoadItems(), S_OK);
        CPanel panel(owner);
        owner.Release();
        QVERIFY(panel.IsFSFolder());
        QVERIFY(!panel.IsArcFolder() && !panel.IsFSDrivesFolder());
        QCOMPARE(QtFilesystemPath(us2fs(panel.GetFsPath())), root.path() + '/');
        UInt32 count = 0;
        QCOMPARE(panel.GetFolder()->GetNumberOfItems(&count), S_OK);
        QStringList paths;
        for (UInt32 i = 0; i < count; ++i) {
            const UString relative = panel.GetItemRelPath(i);
            QCOMPARE(panel.GetItemRelPath2(i), relative);
            paths << QtFilesystemPath(us2fs(panel.GetItemFullPath(i)));
        }
        QStringList expected = {root.filePath(directory), root.filePath(name)};
        if (flatMode)
            expected << root.filePath(directory + '/' + name);
        paths.sort();
        expected.sort();
        QCOMPARE(paths, expected);
    }

    void drivePrefixesUseSourceClassification() {
        auto *spec = new PathFolder;
        CMyComPtr<IFolderFolder> owner = spec;
        CPanel panel(owner);
        QVERIFY(panel.IsFSDrivesFolder());
        panel.SetCurrentFolderPrefix("Computer/");
        QVERIFY(panel.GetFsPath().IsEmpty());
#ifdef _WIN32
        QCOMPARE(ToQString(panel.GetItemFullPath(0)), QString("C:\\"));
#else
        QCOMPARE(ToQString(panel.GetItemFullPath(0)), QString("C:"));
#endif
        panel.SetCurrentFolderPrefix("\\\\.\\");
        QVERIFY(panel.IsDeviceDrivesPrefix());
        QCOMPARE(ToQString(panel.GetFsPath()), QString("\\\\.\\"));
        QCOMPARE(ToQString(panel.GetItemFullPath(0)), QString("\\\\.\\C:"));
        panel.SetCurrentFolderPrefix("\\\\?\\");
        QVERIFY(panel.IsSuperDrivesPrefix());
        QCOMPARE(ToQString(panel.GetFsPath()), QString("\\\\?\\"));
#ifdef _WIN32
        QCOMPARE(ToQString(panel.GetItemFullPath(0)), QString("\\\\?\\C:\\"));
#else
        QCOMPARE(ToQString(panel.GetItemFullPath(0)), QString("\\\\?\\C:"));
#endif
        spec->Type = "FSFolder";
        QVERIFY(panel.IsFSFolder() && !panel.IsFSDrivesFolder());
        panel.SetCurrentFolderPrefix("/normal/");
        QCOMPARE(ToQString(panel.GetItemFullPath(0)), QString("/normal/C:"));
        spec->Type = "7-zip.zip";
        QVERIFY(panel.IsArcFolder());
        spec->Type = "RootFolder";
        QVERIFY(panel.IsRootFolder());
        spec->Type = "AltStreamsFolder";
        QVERIFY(panel.IsAltStreamsFolder());
    }

    void failedAndUnsupportedPropertiesKeepSourceContracts() {
        auto *spec = new PathFolder;
        CMyComPtr<IFolderFolder> owner = spec;
        CPanel panel(owner);
        spec->TypeResult = E_FAIL;
        QVERIFY(panel.GetFolderTypeID().IsEmpty());
        spec->TypeResult = S_OK;
        spec->Type = UInt32(1);
        QVERIFY(panel.GetFolderTypeID().IsEmpty());
        QVERIFY(!panel.IsHashFolder());
        spec->Hash = true;
        QVERIFY(panel.IsHashFolder());
        spec->Hash = UInt32(1);
        QVERIFY(!panel.IsHashFolder());
        spec->NameResult = E_FAIL;
        QVERIFY_THROWS_EXCEPTION(int, panel.GetItemRelPath(0));
        spec->NameResult = S_OK;
        spec->Name = UInt32(1);
        QVERIFY_THROWS_EXCEPTION(int, panel.GetItemRelPath(0));
        CPanel unbound;
        QVERIFY(!unbound.IsHashFolder());
    }
};

QTEST_GUILESS_MAIN(PanelPathTests)
#include "panel_path_tests.moc"
