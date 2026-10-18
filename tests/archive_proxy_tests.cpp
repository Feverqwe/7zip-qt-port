#include "7zip/PropID.h"
#include "CPP/7zip/UI/Agent/Agent.h"
#include "CPP/Common/Wildcard.h"
#include "Windows/PropVariant.h"
#include "qtlocationfolder.h"
#include "qtpanelfolder.h"
#include <QElapsedTimer>
#include <QtTest>

extern bool g_CaseSensitive; // Existing source filename policy, platform default.

namespace {
class CParentProperties final : public IArchiveGetRawProps, public CMyUnknownImp {
    Z7_COM_UNKNOWN_IMP_1(IArchiveGetRawProps)
  public:
    Z7_IFACE_COM7_IMP(IArchiveGetRawProps)
    bool InvalidParent = false;
};
Z7_COM7F_IMF(CParentProperties::GetParent(UInt32 index, UInt32 *parent, UInt32 *type)) {
    *parent = InvalidParent ? 99 : index == 0 ? UInt32(-1) : index - 1;
    *type = NParentType::kDir;
    return S_OK;
}
Z7_COM7F_IMF(CParentProperties::GetRawProp(UInt32, PROPID, const void **data, UInt32 *size,
                                           UInt32 *type)) {
    *data = nullptr; // Exercise the native BSTR name fallback.
    *size = 0;
    *type = NPropDataType::kNotDefined;
    return S_OK;
}
Z7_COM7F_IMF(CParentProperties::GetNumRawProps(UInt32 *count)) {
    *count = 0;
    return S_OK;
}
Z7_COM7F_IMF(CParentProperties::GetRawPropInfo(UInt32, BSTR *, PROPID *)) { return E_INVALIDARG; }
} // namespace

class ArchiveProxyTests : public QObject {
    Q_OBJECT
    static HRESULT load(CProxyArc &proxy, const QList<ArchiveEntry> &entries) {
        CObjectVector<CPanelItem> items;
        for (const ArchiveEntry &entry : entries) {
            CPanelItem item;
            item.RelPath = entry.path;
            item.IsFolder = entry.directory;
            item.SizeDefined = entry.size >= 0;
            item.Size = item.SizeDefined ? UInt64(entry.size) : 0;
            item.PackSizeDefined = entry.packedSize >= 0;
            item.PackSize = item.PackSizeDefined ? UInt64(entry.packedSize) : 0;
            items.Add(item);
        }
        CMyComPtr<IFolderFolder> metadata =
            new QtPanelFolder(QString(), items, QtPanelFolder::PropertyCatalog::ArchiveTransport);
        CArc arc;
        RINOK(metadata.QueryInterface(IID_IInArchive, &arc.Archive))
        arc.DefaultName = L"[Content]";
        return proxy.Load(arc, nullptr);
    }
  private slots:
    void deepNativeParentTreeKeepsFlatSelectionWithoutRecursiveStack() {
        constexpr unsigned depth = 32768;
        CObjectVector<CPanelItem> items;
        for (unsigned i = 0; i < depth; ++i) {
            CPanelItem item;
            item.RelPath = "a";
            item.IsFolder = i + 1 < depth;
            item.SizeDefined = true;
            item.Size = item.IsFolder ? 0 : 7;
            items.Add(item);
        }
        CMyComPtr<IFolderFolder> metadata =
            new QtPanelFolder(QString(), items, QtPanelFolder::PropertyCatalog::ArchiveTransport);
        CArc arc;
        QCOMPARE(metadata.QueryInterface(IID_IInArchive, &arc.Archive), S_OK);
        arc.GetRawProps = new CParentProperties;
        CProxyArc2 proxy;
        QElapsedTimer elapsed;
        elapsed.start();
        QCOMPARE(proxy.Load(arc, nullptr), S_OK);
        CUIntVector selected;
        proxy.AddRealIndices_of_ArcItem(0, false, true, selected);
        QCOMPARE(selected.Size(), depth);
        for (unsigned i = 0; i < depth; ++i)
            QCOMPARE(selected[i], i);
        auto *agentSpec = new CAgent;
        CMyComPtr<IFolderArcProps> agent = agentSpec;
        QCOMPARE(agentSpec->Init({}, metadata, UString("tree")), S_OK);
        auto *folderSpec = new CAgentFolder;
        CMyComPtr<IFolderFolder> folder = folderSpec;
        folderSpec->Init(nullptr, &proxy, k_Proxy2_RootDirIndex, agentSpec);
        CMyComPtr<IFolderSetFlatMode> flat;
        QCOMPARE(folder.QueryInterface(IID_IFolderSetFlatMode, &flat), S_OK);
        QCOMPARE(flat->SetFlatMode(1), S_OK);
        QCOMPARE(folder->LoadItems(), S_OK);
        UInt32 count = 0;
        QCOMPARE(folder->GetNumberOfItems(&count), S_OK);
        QCOMPARE(count, UInt32(depth));
        QCOMPARE(folderSpec->GetRealIndex(depth - 1), int(depth - 1));
        const UInt32 first[] = {0};
        folderSpec->GetRealIndices(first, 1, false, true, selected);
        QCOMPARE(selected.Size(), depth);
        QCOMPARE(selected.Back(), depth - 1);
        const unsigned leafParent = unsigned(proxy.Files[depth - 2].DirIndex);
        QCOMPARE(proxy.GetDirPath_as_Prefix(leafParent, false).Len(), 2 * (depth - 1));
        QVERIFY(proxy.GetDirPath_as_Prefix(leafParent, true).Len() < 128);
        qInfo("deep raw parent load/flat/selection: %u nodes in %lld ms", depth, elapsed.elapsed());
        QVERIFY(elapsed.elapsed() < 15000);
    }
    void deepPathArchivePreservesOriginalLevelReduction() {
        QString path;
        for (int i = 0; i < 4096; ++i)
            path += "a/";
        path += QStringLiteral("лист.txt");
        CProxyArc proxy;
        QCOMPARE(load(proxy, {{path, 7, 3}}), S_OK);
        QVERIFY(proxy.Are_Changed_LongPaths);
        QVERIFY(proxy.Dirs.Size() <= 1026);
        CUIntVector indices;
        QCOMPARE(proxy.Dirs[k_Proxy_RootDirIndex].SubDirs.Size(), 1u);
        proxy.AddRealIndices(proxy.Dirs[k_Proxy_RootDirIndex].SubDirs[0], indices);
        QCOMPARE(indices.Size(), 1u);
        QCOMPARE(indices[0], 0u);
        CMyComPtr<IFolderFolder> folder;
        QCOMPARE(CreateArchiveFolder({{path, 7, 3}}, UString("zip"), &folder), S_OK);
        CMyComPtr<IFolderSetFlatMode> flat;
        QCOMPARE(folder.QueryInterface(IID_IFolderSetFlatMode, &flat), S_OK);
        QCOMPARE(flat->SetFlatMode(1), S_OK);
        QCOMPARE(folder->LoadItems(), S_OK);
        UInt32 count = 0;
        QCOMPARE(folder->GetNumberOfItems(&count), S_OK);
        QVERIFY(count <= 1026);
    }
    void nativeTreeFallbackNamesAndParentValidation() {
        const QStringList names{QStringLiteral("root й space"), QStringLiteral("child 😀"),
                                QStringLiteral("leaf с пробелом.txt")};
        CObjectVector<CPanelItem> items;
        for (int i = 0; i < names.size(); ++i) {
            CPanelItem item;
            item.RelPath = names[i];
            item.IsFolder = i < 2;
            item.SizeDefined = true;
            item.Size = i == 2 ? 7 : 0;
            items.Add(item);
        }
        CMyComPtr<IFolderFolder> metadata =
            new QtPanelFolder(QString(), items, QtPanelFolder::PropertyCatalog::ArchiveTransport);
        CArc arc;
        QCOMPARE(metadata.QueryInterface(IID_IInArchive, &arc.Archive), HRESULT(S_OK));
        auto *parents = new CParentProperties;
        arc.GetRawProps = parents;
        CProxyArc2 proxy;
        QCOMPARE(proxy.Load(arc, nullptr), HRESULT(S_OK));
        for (unsigned i = 0; i < proxy.NumFiles; ++i) {
            QCOMPARE(QString::fromWCharArray(proxy.Files[i].Name), names[int(i)]);
        }
        const int root =
            proxy.FindItem(k_Proxy2_RootDirIndex, names[0].toStdWString().c_str(), true);
        QCOMPARE(root, 0);
        const unsigned childDir = unsigned(proxy.Files[1].DirIndex);
        QCOMPARE(proxy.Dirs[childDir].Size, UInt64(7));
        const int leaf = proxy.FindItem(childDir, names[2].toStdWString().c_str(), false);
        QCOMPARE(leaf, 0); // FindItem returns an index in that directory.
        QCOMPARE(proxy.GetRealIndex(childDir, unsigned(leaf)), unsigned(2));
        UString prefix = proxy.GetDirPath_as_Prefix(childDir);
        QCOMPARE(QDir::fromNativeSeparators(ToQString(prefix)), names[0] + '/' + names[1] + '/');
        CUIntVector indices;
        proxy.AddRealIndices_of_ArcItem(0, false, true, indices);
        QCOMPARE(indices.Size(), unsigned(3));
        auto *agentSpec = new CAgent;
        CMyComPtr<IFolderArcProps> agent = agentSpec;
        QCOMPARE(agentSpec->Init({}, metadata, UString("tree")), S_OK);
        auto *folderSpec = new CAgentFolder;
        CMyComPtr<IFolderFolder> folder = folderSpec;
        folderSpec->Init(nullptr, &proxy, k_Proxy2_RootDirIndex, agentSpec);
        QCOMPARE(folder->LoadItems(), S_OK);
        CMyComPtr<IFolderAltStreams> streams;
        QCOMPARE(folder.QueryInterface(IID_IFolderAltStreams, &streams), S_OK);
        Int32 supported = 0;
        QCOMPARE(streams->AreAltStreamsSupported(UInt32(-1), &supported), S_OK);
        QCOMPARE(supported, Int32(1));
        QCOMPARE(streams->AreAltStreamsSupported(0, &supported), S_OK);
        QCOMPARE(supported, Int32(0));
        QCOMPARE(streams->AreAltStreamsSupported(99, &supported), E_INVALIDARG);
        CMyComPtr<IFolderFolder> altRoot;
        QCOMPARE(streams->BindToAltStreams(UInt32(-1), &altRoot), S_OK);
        QVERIFY(altRoot);
        QCOMPARE(altRoot->LoadItems(), S_OK);
        UInt32 altCount = 99;
        QCOMPARE(altRoot->GetNumberOfItems(&altCount), S_OK);
        QCOMPARE(altCount, UInt32(0));
        CMyComPtr<IFolderAltStreams> altStreams;
        QCOMPARE(altRoot.QueryInterface(IID_IFolderAltStreams, &altStreams), S_OK);
        QCOMPARE(altStreams->AreAltStreamsSupported(UInt32(-1), &supported), S_OK);
        QCOMPARE(supported, Int32(0));
        CMyComPtr<IFolderFolder> nestedAlt;
        QCOMPARE(altStreams->BindToAltStreams(UInt32(-1), &nestedAlt), S_OK);
        QVERIFY(!nestedAlt);
        QCOMPARE(streams->BindToAltStreams(L"missing", &nestedAlt), E_INVALIDARG);
        QCOMPARE(streams->BindToAltStreams(99u, &nestedAlt), E_INVALIDARG);
        CMyComPtr<IFolderFolder> altParent;
        QCOMPARE(altRoot->BindToParentFolder(&altParent), S_OK);
        QVERIFY(altParent);
        NWindows::NCOM::CPropVariant rootPath;
        QCOMPARE(altParent->GetFolderProperty(kpidPath, &rootPath), S_OK);
        QCOMPARE(rootPath.vt, VARTYPE(VT_BSTR));
        QCOMPARE(QString::fromWCharArray(rootPath.bstrVal), QString());
        UInt32 count = 0;
        QCOMPARE(folder->GetNumberOfItems(&count), S_OK);
        QCOMPARE(count, UInt32(1));
        QCOMPARE(QString::fromWCharArray(folderSpec->GetName(0)), names[0]);
        QCOMPARE(folderSpec->GetRealIndex(0), 0);
        CMyComPtr<IFolderFolder> child;
        QCOMPARE(folder->BindToFolder(0u, &child), S_OK);
        QCOMPARE(child->LoadItems(), S_OK);
        NWindows::NCOM::CPropVariant path, size;
        QCOMPARE(child->GetFolderProperty(kpidPath, &path), S_OK);
        QCOMPARE(path.vt, VARTYPE(VT_BSTR));
        QCOMPARE(QDir::fromNativeSeparators(QString::fromWCharArray(path.bstrVal)), names[0] + '/');
        QCOMPARE(child->GetFolderProperty(kpidSize, &size), S_OK);
        QCOMPARE(size.vt, VARTYPE(VT_UI8));
        QCOMPARE(size.uhVal.QuadPart, UInt64(7));
        CMyComPtr<IFolderFolder> parentFolder;
        QCOMPARE(child->BindToParentFolder(&parentFolder), S_OK);
        QVERIFY(parentFolder);
        QCOMPARE(parentFolder->GetNumberOfItems(&count), S_OK);
        QCOMPARE(count, UInt32(1));
        CMyComPtr<IFolderSetFlatMode> flat;
        QCOMPARE(folder.QueryInterface(IID_IFolderSetFlatMode, &flat), S_OK);
        QCOMPARE(flat->SetFlatMode(1), S_OK);
        QCOMPARE(folder->LoadItems(), S_OK);
        QCOMPARE(folder->GetNumberOfItems(&count), S_OK);
        QCOMPARE(count, UInt32(3));
        QCOMPARE(folderSpec->GetRealIndex(2), 2);
        QCOMPARE(QString::fromWCharArray(folderSpec->GetName(2)), names[2]);
        const UInt32 selection[] = {0};
        folderSpec->GetRealIndices(selection, 1, false, true, indices);
        QCOMPARE(indices.Size(), unsigned(3));
        QCOMPARE(indices[0], UInt32(0));
        QCOMPARE(indices[1], UInt32(1));
        QCOMPARE(indices[2], UInt32(2));
        parents->InvalidParent = true;
        QCOMPARE(proxy.Load(arc, nullptr), HRESULT(E_FAIL));
        parents->InvalidParent = false;
        proxy.MemUsage_Limit = 1;
        QCOMPARE(proxy.Load(arc, nullptr), HRESULT(E_OUTOFMEMORY));
        proxy.MemUsage_Limit = UInt64(-1);
        QCOMPARE(proxy.Load(arc, nullptr), HRESULT(S_OK));
        QCOMPARE(QString::fromWCharArray(proxy.Files[2].Name), names[2]);
    }
    void implicitDirectoriesAndRecursiveProperties() {
        const QList<ArchiveEntry> entries = {{QStringLiteral("папка/один файл.txt"), 13, 5},
                                             {QStringLiteral("папка/sub/ещё.txt"), 7, 3},
                                             {"root.bin", 4, 2},
                                             {"empty/", 0, 0, {}, true}};
        CProxyArc proxy;
        QCOMPARE(load(proxy, entries), HRESULT(S_OK));
        const int folder = QtArchiveDirectoryIndex(proxy, QStringLiteral("папка"));
        QVERIFY(folder > 0);
        QCOMPARE(proxy.Dirs[folder].Size, quint64(20));
        QCOMPARE(proxy.Dirs[folder].PackSize, quint64(8));
        QCOMPARE(proxy.Dirs[folder].NumSubFiles, quint32(2));
        QCOMPARE(proxy.Dirs[folder].NumSubDirs, quint32(1));
        QVERIFY(QtArchiveDirectoryIndex(proxy, "empty") > 0);
        QCOMPARE(QtArchiveDirectoryIndex(proxy, "missing"), -1);
        CUIntVector indices;
        proxy.AddRealIndices(unsigned(folder), indices);
        QCOMPARE(indices.Size(), 2u);
        QVERIFY(indices[0] == 0u || indices[1] == 0u);
        QVERIFY(indices[0] == 1u || indices[1] == 1u);
        CMyComPtr<IFolderFolder> root;
        QCOMPARE(CreateArchiveFolder(entries, UString("zip"), &root), S_OK);
        QCOMPARE(root->LoadItems(), S_OK);
        UInt32 count;
        QCOMPARE(root->GetNumberOfItems(&count), S_OK);
        QCOMPARE(count, 3u);
        CMyComPtr<IFolderFolder> child;
        QCOMPARE(root->BindToFolder(L"папка", &child), S_OK);
        CMyComPtr<IFolderSetFlatMode> flat;
        QCOMPARE(child.QueryInterface(IID_IFolderSetFlatMode, &flat), S_OK);
        QCOMPARE(flat->SetFlatMode(1), S_OK);
        root.Release();
        QCOMPARE(child->LoadItems(), S_OK);
        QCOMPARE(child->GetNumberOfItems(&count), S_OK);
        QCOMPARE(count, 3u);
    }
    void largeUnsortedDirectorySetRetainsEveryChild() {
        QList<ArchiveEntry> entries;
        for (int i = 4500; i > 0; --i)
            entries.append(
                ArchiveEntry{QString("directory-%1/file.bin").arg(i, 5, 10, QChar('0')), 1, 1});
        CProxyArc proxy;
        QCOMPARE(load(proxy, entries), HRESULT(S_OK));
        CMyComPtr<IFolderFolder> root;
        QCOMPARE(CreateArchiveFolder(entries, UString("zip"), &root), S_OK);
        QCOMPARE(root->LoadItems(), S_OK);
        UInt32 count;
        QCOMPARE(root->GetNumberOfItems(&count), S_OK);
        QCOMPARE(count, 4500u);
        QCOMPARE(proxy.Dirs[0].Size, quint64(4500));
        for (int i : {1, 2048, 4096, 4500}) {
            const int folder =
                QtArchiveDirectoryIndex(proxy, QString("directory-%1").arg(i, 5, 10, QChar('0')));
            QVERIFY(folder > 0);
            QCOMPARE(proxy.Dirs[folder].NumSubFiles, quint32(1));
        }
    }
    void explicitDirectoryReusesImplicitNodeAndReloadClearsState() {
        CProxyArc proxy;
        QCOMPARE(load(proxy,
                      {ArchiveEntry{"folder/file", 10, 6}, ArchiveEntry{"folder", 0, 0, {}, true}}),
                 HRESULT(S_OK));
        QCOMPARE(proxy.Dirs.Size(), unsigned(2));
        const int folder = QtArchiveDirectoryIndex(proxy, "folder");
        QVERIFY(folder > 0);
        QCOMPARE(QtArchiveDirectoryIndex(proxy, "FOLDER"), g_CaseSensitive ? -1 : folder);
        QVERIFY(proxy.Dirs[folder].IsLeaf());
        CUIntVector indices;
        proxy.AddRealIndices(unsigned(folder), indices);
        QCOMPARE(indices.Size(), 2u);
        QCOMPARE(indices[0], 1u);
        QCOMPARE(indices[1], 0u);
        QCOMPARE(load(proxy, {}), HRESULT(S_OK));
        QCOMPARE(proxy.Dirs.Size(), unsigned(1));
        QCOMPARE(QtArchiveDirectoryIndex(proxy, "folder"), -1);
        QCOMPARE(proxy.Dirs[0].Size, quint64(0));
    }
    void unknownSizeDoesNotWrapDirectoryTotals() {
        CProxyArc proxy;
        QCOMPARE(load(proxy,
                      {ArchiveEntry{"folder/known", 17, 8}, ArchiveEntry{"folder/unknown", -1, -1},
                       ArchiveEntry{"unknown-only/file", -1, -1}}),
                 HRESULT(S_OK));
        const int folder = QtArchiveDirectoryIndex(proxy, "folder");
        QVERIFY(folder > 0);
        QCOMPARE(proxy.Dirs[folder].Size, quint64(17));
        QCOMPARE(proxy.Dirs[folder].PackSize, quint64(8));
        const int unknown = QtArchiveDirectoryIndex(proxy, "unknown-only");
        QVERIFY(unknown > 0);
        QCOMPARE(proxy.Dirs[unknown].Size, quint64(0));
        QCOMPARE(proxy.Dirs[unknown].PackSize, quint64(0));
        QCOMPARE(proxy.Dirs[0].Size, quint64(17));
        QCOMPARE(proxy.Dirs[0].PackSize, quint64(8));
    }
};
QTEST_GUILESS_MAIN(ArchiveProxyTests)
#include "archive_proxy_tests.moc"
