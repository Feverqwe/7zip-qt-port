#include "7zip/PropID.h"
#include "CPP/7zip/UI/Agent/Agent.h"
#include "Windows/PropVariant.h"
#include "qtlocationfolder.h"
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

    static CMyComPtr<IFolderFolder> root(const QList<ArchiveEntry> &entries) {
        CMyComPtr<IFolderFolder> folder;
        if (CreateArchiveFolder(entries, UString("zip"), &folder) != S_OK)
            return {};
        return folder;
    }

  private slots:
    void sourceNameAndFullPrefixRemainRootRelative_data() {
        QTest::addColumn<bool>("flatMode");
        QTest::newRow("normal") << false;
        QTest::newRow("flat") << true;
    }
    void sourceNameAndFullPrefixRemainRootRelative() {
        QFETCH(bool, flatMode);
        const QString directory = QStringLiteral("папка й с пробелом");
        auto rootFolder = root({{directory + "/base/same.txt", 7, 3},
                                {directory + "/base/sub/same.txt", 11, 5},
                                {directory + "/base/empty/", 0, 0, {}, true}});
        QVERIFY(rootFolder);
        QCOMPARE(rootFolder->LoadItems(), S_OK);
        auto *rootSpec = static_cast<CAgentFolder *>((IFolderFolder *)rootFolder);
        QCOMPARE(ToQString(rootSpec->GetFullPrefix(0)), QString());
        QCOMPARE(ToQString(rootSpec->GetName(0)), directory);
        CMyComPtr<IFolderFolder> top;
        const auto native = directory.toStdWString();
        QCOMPARE(rootFolder->BindToFolder(native.c_str(), &top), S_OK);
        CMyComPtr<IFolderFolder> base;
        QCOMPARE(top->BindToFolder(L"base", &base), S_OK);
        rootFolder.Release();
        top.Release();
        CMyComPtr<IFolderSetFlatMode> flat;
        QCOMPARE(base.QueryInterface(IID_IFolderSetFlatMode, &flat), S_OK);
        QCOMPARE(flat->SetFlatMode(flatMode ? 1 : 0), S_OK);
        QCOMPARE(base->LoadItems(), S_OK);
        auto *spec = static_cast<CAgentFolder *>((IFolderFolder *)base);
        UInt32 count = 0;
        QCOMPARE(base->GetNumberOfItems(&count), S_OK);
        QStringList paths;
        for (UInt32 i = 0; i < count; ++i)
            paths << ToQString(spec->GetFullPrefix(i) + spec->GetName(i));
        QStringList expected = {directory + "/base/sub", directory + "/base/empty",
                                directory + "/base/same.txt"};
        if (flatMode)
            expected << directory + "/base/sub/same.txt";
        paths.sort();
        expected.sort();
        QCOMPARE(paths, expected);
    }

    void nameIndexAndParentBindingUseSourcePaths() {
        CMyComPtr<IFolderFolder> folder =
            root({{QStringLiteral("папка с пробелом/внутри/файл 😀.txt"), 13, 5},
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
        folder.Release(); // Children retain the source agent and its authoritative proxy.
        QCOMPARE(inner->LoadItems(), S_OK);
        UInt32 propertyCount = 0;
        QCOMPARE(inner->GetNumberOfProperties(&propertyCount), S_OK);
        QCOMPARE(propertyCount, UInt32(5));
        CMyComBSTR propertyName;
        PROPID propertyId;
        VARTYPE propertyType;
        QCOMPARE(inner->GetPropertyInfo(0, &propertyName, &propertyId, &propertyType), S_OK);
        QCOMPARE(propertyId, PROPID(kpidName));
        QCOMPARE(propertyType, VARTYPE(VT_BSTR));
        QVERIFY(propertyName == nullptr);
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
        CMyComPtr<IFolderFolder> folder = root({{"base/a/same.txt", 1, 1},
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
        CMyComPtr<IFolderFolder> folder = root({{"folder/file.txt", 9, 4}, {"root.txt", 2, 1}});
        CMyComPtr<IArchiveGetRawProps> raw;
        QCOMPARE(folder.QueryInterface(IID_IArchiveGetRawProps, &raw), S_OK);
        UInt32 rawCount = 99;
        QCOMPARE(raw->GetNumRawProps(&rawCount), S_OK);
        QCOMPARE(rawCount, UInt32(0));
        const void *rawData = reinterpret_cast<const void *>(1);
        UInt32 rawSize = 99;
        UInt32 rawType = 99;
        QCOMPARE(raw->GetRawProp(0, kpidNtReparse, &rawData, &rawSize, &rawType), S_OK);
        QVERIFY(!rawData);
        QCOMPARE(rawSize, UInt32(0));
        QCOMPARE(rawType, UInt32(0));
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
        count = 9;
        QCOMPARE(unbound.GetNumberOfProperties(&count), E_FAIL);
        QCOMPARE(count, UInt32(0));
        CMyComBSTR name;
        PROPID id;
        VARTYPE propertyType;
        QCOMPARE(unbound.GetPropertyInfo(0, &name, &id, &propertyType), E_FAIL);
    }

    void ownedAgentSurvivesInputRootAndAgentRelease() {
        QList<ArchiveEntry> entries{{QString::fromUtf8("папка 📦/sub/file.txt"), 17, 8}};
        CObjectVector<CPanelItem> items;
        CPanelItem item;
        item.RelPath = entries[0].path;
        item.SizeDefined = true;
        item.Size = 17;
        item.PackSize = 8;
        items.Add(item);
        CMyComPtr<IFolderFolder> props = new QtPanelFolder("archive.zip", items);
        auto *agentSpec = new CAgent;
        CMyComPtr<IUnknown> agent = agentSpec;
        QCOMPARE(agentSpec->Init(entries, props, UString("zip")), S_OK);
        QCOMPARE(agentSpec->Init({}, props, UString("other")), E_FAIL);
        CMyComPtr<IFolderFolder> rootFolder;
        QCOMPARE(agentSpec->BindToRootFolder(&rootFolder), S_OK);
        entries.clear();
        items.Clear();
        props.Release();
        CMyComPtr<IFolderFolder> child;
        QCOMPARE(rootFolder->BindToFolder(L"папка 📦", &child), S_OK);
        rootFolder.Release();
        agent.Release();
        CMyComPtr<IFolderFolder> grandchild;
        QCOMPARE(child->BindToFolder(L"sub", &grandchild), S_OK);
        child.Release();
        QCOMPARE(grandchild->LoadItems(), S_OK);
        UInt32 count = 0;
        QCOMPARE(grandchild->GetNumberOfItems(&count), S_OK);
        QCOMPARE(count, UInt32(1));
        QCOMPARE(rowByName(grandchild, "file.txt"), 0);
        NWindows::NCOM::CPropVariant value;
        QCOMPARE(grandchild->GetProperty(0, kpidSize, &value), S_OK);
        QCOMPARE(value.uhVal.QuadPart, UInt64(17));
        CMyComPtr<IFolderFolder> parent;
        QCOMPARE(grandchild->BindToParentFolder(&parent), S_OK);
        grandchild.Release();
        QCOMPARE(parent->GetFolderProperty(kpidType, &value), S_OK);
        QCOMPARE(ToQString(UString(value.bstrVal)), QStringLiteral("7-Zip.zip"));
        QCOMPARE(parent->BindToParentFolder(&rootFolder), S_OK);
        parent.Release();
        QCOMPARE(rootFolder->LoadItems(), S_OK);
        QCOMPARE(rowByName(rootFolder, QString::fromUtf8("папка 📦")), 0);
        QCOMPARE(rootFolder->BindToParentFolder(&parent), S_OK);
        QVERIFY(!parent);
    }

    void agentRootRejectsMissingMetadataAndEmptyArchiveIsValid() {
        auto *agentSpec = new CAgent;
        CMyComPtr<IUnknown> owner = agentSpec;
        CMyComPtr<IFolderFolder> rootFolder;
        QCOMPARE(agentSpec->BindToRootFolder(&rootFolder), E_FAIL);
        QVERIFY(!rootFolder);
        QCOMPARE(agentSpec->Init({}, nullptr, UString("zip")), E_INVALIDARG);
        CObjectVector<CPanelItem> items;
        CMyComPtr<IFolderFolder> props = new QtPanelFolder("empty.zip", items);
        QCOMPARE(agentSpec->Init({}, props, UString("zip")), S_OK);
        QCOMPARE(agentSpec->BindToRootFolder(&rootFolder), S_OK);
        owner.Release();
        props.Release();
        QCOMPARE(rootFolder->LoadItems(), S_OK);
        UInt32 count = 99;
        QCOMPARE(rootFolder->GetNumberOfItems(&count), S_OK);
        QCOMPARE(count, UInt32(0));
    }
};

QTEST_GUILESS_MAIN(AgentFolderTests)
#include "agent_folder_tests.moc"
