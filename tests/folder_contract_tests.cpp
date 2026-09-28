#include "7zip/PropID.h"
#include "7zip/UI/FileManager/IFolder.h"
#include "Common/MyCom.h"
#include "Windows/PropVariant.h"
#include <QtTest>

// Test-only folder: no filesystem, archive engine or production state.
class ContractFolder final : public IFolderFolder, public CMyUnknownImp {
    Z7_COM_UNKNOWN_IMP_1(IFolderFolder)
  public:
    explicit ContractFolder(int &destroyed) : destroyed_(destroyed) {}
    ~ContractFolder() { ++destroyed_; }
    Z7_COM7F_IMF(LoadItems()) override { return S_OK; }
    Z7_COM7F_IMF(GetNumberOfItems(UInt32 *count)) override {
        *count = 1;
        return S_OK;
    }
    Z7_COM7F_IMF(GetProperty(UInt32 index, PROPID prop, PROPVARIANT *value)) override {
        if (index != 0)
            return E_INVALIDARG;
        if (prop != kpidName)
            return E_NOTIMPL;
        NWindows::NCOM::CPropVariant name(L"папка с пробелом/имя.txt");
        return name.Detach(value);
    }
    Z7_COM7F_IMF(BindToFolder(UInt32, IFolderFolder **folder)) override {
        *folder = nullptr;
        return E_NOTIMPL;
    }
    Z7_COM7F_IMF(BindToFolder(const wchar_t *, IFolderFolder **folder)) override {
        *folder = nullptr;
        return E_NOTIMPL;
    }
    Z7_COM7F_IMF(BindToParentFolder(IFolderFolder **folder)) override {
        *folder = nullptr;
        return S_OK;
    }
    Z7_COM7F_IMF(GetNumberOfProperties(UInt32 *count)) override {
        *count = 1;
        return S_OK;
    }
    Z7_COM7F_IMF(GetPropertyInfo(UInt32 index, BSTR *name, PROPID *prop, VARTYPE *type)) override {
        if (index != 0)
            return E_INVALIDARG;
        *name = ::SysAllocString(L"Name");
        *prop = kpidName;
        *type = VT_BSTR;
        return *name ? S_OK : E_OUTOFMEMORY;
    }
    Z7_COM7F_IMF(GetFolderProperty(PROPID prop, PROPVARIANT *value)) override {
        if (prop != kpidPath)
            return E_NOTIMPL;
        NWindows::NCOM::CPropVariant path(L"папка с пробелом/");
        return path.Detach(value);
    }

  private:
    int &destroyed_;
};

class FolderContractTests : public QObject {
    Q_OBJECT
  private slots:
    void interfaceIdentityAndLifetime() {
        int destroyed = 0;
        CMyComPtr<IFolderFolder> folder = new ContractFolder(destroyed);
        QCOMPARE(folder->LoadItems(), S_OK);
        UInt32 count = 0;
        QCOMPARE(folder->GetNumberOfItems(&count), S_OK);
        QCOMPARE(count, 1u);
        void *object = nullptr;
        QCOMPARE(folder->QueryInterface(IID_IFolderFolder, &object), S_OK);
        QCOMPARE(object, static_cast<void *>(folder.Interface()));
        CMyComPtr<IFolderFolder> queried;
        queried.Attach(static_cast<IFolderFolder *>(object));
        object = nullptr;
        QCOMPARE(folder->QueryInterface(IID_IUnknown, &object), S_OK);
        CMyComPtr<IUnknown> identity;
        identity.Attach(static_cast<IUnknown *>(object));
        object = reinterpret_cast<void *>(1);
        QCOMPARE(folder->QueryInterface(IID_IFolderOperations, &object), E_NOINTERFACE);
        QVERIFY(object == nullptr);
        folder.Release();
        queried.Release();
        QCOMPARE(destroyed, 0);
        identity.Release();
        QCOMPARE(destroyed, 1);
    }

    void unicodePropertiesHaveIndependentOwnership() {
        int destroyed = 0;
        CMyComPtr<IFolderFolder> folder = new ContractFolder(destroyed);
        NWindows::NCOM::CPropVariant property;
        QCOMPARE(folder->GetProperty(0, kpidName, &property), S_OK);
        QCOMPARE(property.vt, VARTYPE(VT_BSTR));
        NWindows::NCOM::CPropVariant copy(property);
        QCOMPARE(property.Clear(), S_OK);
        folder.Release();
        QCOMPARE(destroyed, 1);
        QCOMPARE(QString::fromWCharArray(copy.bstrVal, ::SysStringLen(copy.bstrVal)),
                 QStringLiteral("папка с пробелом/имя.txt"));
        PROPVARIANT detached{};
        QCOMPARE(copy.Detach(&detached), S_OK);
        QCOMPARE(copy.vt, VARTYPE(VT_EMPTY));
        QCOMPARE(detached.vt, VARTYPE(VT_BSTR));
        NWindows::NCOM::CPropVariant attached;
        QCOMPARE(attached.Attach(&detached), S_OK);
        QCOMPARE(detached.vt, VARTYPE(VT_EMPTY));
        QCOMPARE(attached.Clear(), S_OK);
        QCOMPARE(attached.vt, VARTYPE(VT_EMPTY));
    }

    void folderPathAndMissingParent() {
        int destroyed = 0;
        CMyComPtr<IFolderFolder> folder = new ContractFolder(destroyed);
        CMyComPtr<IFolderFolder> parent;
        QCOMPARE(folder->BindToParentFolder(&parent), S_OK);
        QVERIFY(!parent);
        NWindows::NCOM::CPropVariant path;
        QCOMPARE(folder->GetFolderProperty(kpidPath, &path), S_OK);
        QCOMPARE(QString::fromWCharArray(path.bstrVal), QStringLiteral("папка с пробелом/"));
        BSTR name = nullptr;
        PROPID prop = 0;
        VARTYPE type = VT_EMPTY;
        QCOMPARE(folder->GetPropertyInfo(0, &name, &prop, &type), S_OK);
        QCOMPARE(QString::fromWCharArray(name), QString("Name"));
        ::SysFreeString(name);
        QCOMPARE(prop, PROPID(kpidName));
        QCOMPARE(type, VARTYPE(VT_BSTR));
        QCOMPARE(folder->GetProperty(1, kpidName, &path), E_INVALIDARG);
    }
};

QTEST_GUILESS_MAIN(FolderContractTests)
#include "folder_contract_tests.moc"
