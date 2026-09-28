#include "7zip/PropID.h"
#include "CPP/7zip/UI/Agent/Agent.h"
#include "Windows/PropVariant.h"
#include "qtpanelfolder.h"
#include <QtTest>

class AgentFolderTests : public QObject {
    Q_OBJECT

    static int rowByName(IFolderFolder *folder, const QString &name) {
        UInt32 count = 0;
        if (folder->GetNumberOfItems(&count) != S_OK)
            return -1;
        for (UInt32 i = 0; i < count; ++i) {
            NWindows::NCOM::CPropVariant prop;
            if (folder->GetProperty(i, kpidName, &prop) == S_OK && prop.vt == VT_BSTR &&
                ToQString(UString(prop.bstrVal)) == name)
                return (int)i;
        }
        return -1;
    }

    static CMyComPtr<IFolderFolder> root(CProxyArc &proxy, const QList<ArchiveEntry> &entries) {
        proxy.Load(entries);
        CObjectVector<CPanelItem> items;
        for (const ArchiveEntry &entry : entries) {
            CPanelItem item;
            item.RelPath = entry.path;
            item.IsFolder = entry.directory;
            item.SizeDefined = true;
            item.Size = UInt64(entry.size);
            item.PackSize = UInt64(entry.packedSize);
            items.Add(item);
        }
        CMyComPtr<IFolderFolder> props = new QtPanelFolder("archive.zip", items);
        auto *spec = new CAgentFolder;
        CMyComPtr<IFolderFolder> folder = spec;
        spec->Init(&proxy, k_Proxy_RootDirIndex, props, UString("zip"));
        return folder;
    }

  private slots:
    void nameIndexAndParentBindingUseSourcePaths() {
        CProxyArc proxy;
        CMyComPtr<IFolderFolder> folder =
            root(proxy, {{QStringLiteral("папка с пробелом/внутри/файл 😀.txt"), 13, 5},
                         {QStringLiteral("папка с пробелом/другой.txt"), 7, 3},
                         {"root.bin", 4, 2}});
        QCOMPARE(folder->LoadItems(), S_OK);
        const int index = rowByName(folder, QStringLiteral("папка с пробелом"));
        QVERIFY(index >= 0);
        CMyComPtr<IFolderFolder> byName;
        CMyComPtr<IFolderFolder> byIndex;
        QCOMPARE(folder->BindToFolder(L"папка с пробелом", &byName), S_OK);
        QCOMPARE(folder->BindToFolder(UInt32(index), &byIndex), S_OK);
        QCOMPARE(ToQString(GetFolderPath(byName)), QStringLiteral("папка с пробелом/"));
        QCOMPARE(ToQString(GetFolderPath(byIndex)), ToQString(GetFolderPath(byName)));
        CMyComPtr<IFolderFolder> inner;
        QCOMPARE(byName->BindToFolder(L"внутри", &inner), S_OK);
        QCOMPARE(ToQString(GetFolderPath(inner)), QStringLiteral("папка с пробелом/внутри/"));
        folder.Release(); // children own delegated properties; proxy is explicitly external here.
        QCOMPARE(inner->LoadItems(), S_OK);
        NWindows::NCOM::CPropVariant size;
        QCOMPARE(inner->GetProperty(0, kpidSize, &size), S_OK);
        QCOMPARE(size.uhVal.QuadPart, UInt64(13));
        CMyComPtr<IFolderFolder> parent;
        QCOMPARE(inner->BindToParentFolder(&parent), S_OK);
        QCOMPARE(ToQString(GetFolderPath(parent)), QStringLiteral("папка с пробелом/"));
        CMyComPtr<IFolderFolder> rootAgain;
        QCOMPARE(parent->BindToParentFolder(&rootAgain), S_OK);
        QCOMPARE(ToQString(GetFolderPath(rootAgain)), QString());
        CMyComPtr<IFolderFolder> noParent;
        QCOMPARE(rootAgain->BindToParentFolder(&noParent), S_OK);
        QVERIFY(!noParent);
    }

    void flatPrefixAndFolderBindingRemainRelativeToBase() {
        CProxyArc proxy;
        CMyComPtr<IFolderFolder> folder = root(proxy, {{"base/a/same.txt", 1, 1},
                                                       {"base/b/same.txt", 2, 2},
                                                       {"base/b/deep/child.txt", 3, 3}});
        CMyComPtr<IFolderFolder> base;
        QCOMPARE(folder->BindToFolder(L"base", &base), S_OK);
        CMyComPtr<IFolderSetFlatMode> flat;
        QCOMPARE(base.QueryInterface(IID_IFolderSetFlatMode, &flat), S_OK);
        QCOMPARE(flat->SetFlatMode(1), S_OK);
        QCOMPARE(base->LoadItems(), S_OK);
        UInt32 count = 0;
        QCOMPARE(base->GetNumberOfItems(&count), S_OK);
        QCOMPARE(count, 6u);
        QStringList prefixes;
        for (UInt32 i = 0; i < count; ++i) {
            NWindows::NCOM::CPropVariant name;
            QCOMPARE(base->GetProperty(i, kpidName, &name), S_OK);
            if (ToQString(UString(name.bstrVal)) != "same.txt")
                continue;
            NWindows::NCOM::CPropVariant prefix;
            QCOMPARE(base->GetProperty(i, kpidPrefix, &prefix), S_OK);
            prefixes << ToQString(UString(prefix.bstrVal));
        }
        prefixes.sort();
        QCOMPARE(prefixes, QStringList({"a/", "b/"}));
        const int deepIndex = rowByName(base, "deep");
        QVERIFY(deepIndex >= 0);
        CMyComPtr<IFolderFolder> deep;
        QCOMPARE(base->BindToFolder(UInt32(deepIndex), &deep), S_OK);
        QCOMPARE(ToQString(GetFolderPath(deep)), QString("base/b/deep/"));
        QCOMPARE(deep->LoadItems(), S_OK);
        QCOMPARE(deep->GetNumberOfItems(&count), S_OK);
        QCOMPARE(count, 1u);
    }

    void sourceFolderPropertiesAndRejectedBindings() {
        CProxyArc proxy;
        CMyComPtr<IFolderFolder> folder =
            root(proxy, {{"folder/file.txt", 9, 4}, {"root.txt", 2, 1}});
        NWindows::NCOM::CPropVariant type;
        QCOMPARE(folder->GetFolderProperty(kpidType, &type), S_OK);
        QCOMPARE(ToQString(UString(type.bstrVal)), QString("7-Zip.zip"));
        CMyComPtr<IFolderFolder> child;
        QCOMPARE(folder->BindToFolder(L"folder", &child), S_OK);
        NWindows::NCOM::CPropVariant size;
        QCOMPARE(child->GetFolderProperty(kpidSize, &size), S_OK);
        QCOMPARE(size.uhVal.QuadPart, UInt64(9));
        QCOMPARE(child->GetFolderProperty(kpidNumSubFiles, &size), S_OK);
        QCOMPARE(size.ulVal, UInt32(1));
        CMyComPtr<IFolderFolder> rejected;
        QCOMPARE(folder->BindToFolder(L"missing", &rejected), E_INVALIDARG);
        QVERIFY(!rejected);
        const int fileIndex = rowByName(folder, "root.txt");
        QVERIFY(fileIndex >= 0);
        QCOMPARE(folder->BindToFolder(UInt32(fileIndex), &rejected), E_INVALIDARG);
        QCOMPARE(folder->BindToFolder(UInt32(99), &rejected), E_INVALIDARG);
        QVERIFY(!rejected);
        void *operations = nullptr;
        QCOMPARE(folder->QueryInterface(IID_IFolderOperations, &operations), E_NOINTERFACE);
        QVERIFY(operations == nullptr);
        CAgentFolder unbound;
        QCOMPARE(unbound.LoadItems(), E_FAIL);
        UInt32 count = 9;
        QCOMPARE(unbound.GetNumberOfItems(&count), E_FAIL);
        QCOMPARE(count, UInt32(0));
    }
};

QTEST_GUILESS_MAIN(AgentFolderTests)
#include "agent_folder_tests.moc"
