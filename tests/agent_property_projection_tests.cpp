#include "CPP/7zip/UI/Agent/Agent.h"
#include "CPP/7zip/UI/FileManager/PropertyName.h"
#include "property_projection_support.h"
#include "qtlocationfolder.h"
#include <QtTest>

namespace {
class CPropertyFailure final : public IFolderFolder, public CMyUnknownImp {
    Z7_COM_UNKNOWN_IMP_1(IFolderFolder)
  public:
    CMyComPtr<IFolderFolder> Metadata;
    HRESULT CrcResult = S_OK;
    explicit CPropertyFailure(IFolderFolder *metadata) : Metadata(metadata) {}
    Z7_IFACE_COM7_IMP(IFolderFolder)
};
Z7_COM7F_IMF(CPropertyFailure::LoadItems()) { return Metadata->LoadItems(); }
Z7_COM7F_IMF(CPropertyFailure::GetNumberOfItems(UInt32 *count)) {
    return Metadata->GetNumberOfItems(count);
}
Z7_COM7F_IMF(CPropertyFailure::GetProperty(UInt32 index, PROPID id, PROPVARIANT *value)) {
    if (id == kpidCRC) {
        if (CrcResult != S_OK)
            return CrcResult;
    }
    return Metadata->GetProperty(index, id, value);
}
Z7_COM7F_IMF(CPropertyFailure::GetFolderProperty(PROPID id, PROPVARIANT *value)) {
    return Metadata->GetFolderProperty(id, value);
}
Z7_COM7F_IMF(CPropertyFailure::BindToFolder(UInt32 index, IFolderFolder **folder)) {
    return Metadata->BindToFolder(index, folder);
}
Z7_COM7F_IMF(CPropertyFailure::BindToFolder(const wchar_t *name, IFolderFolder **folder)) {
    return Metadata->BindToFolder(name, folder);
}
Z7_COM7F_IMF(CPropertyFailure::BindToParentFolder(IFolderFolder **folder)) {
    return Metadata->BindToParentFolder(folder);
}
Z7_COM7F_IMF(CPropertyFailure::GetNumberOfProperties(UInt32 *count)) {
    return Metadata->GetNumberOfProperties(count);
}
Z7_COM7F_IMF(CPropertyFailure::GetPropertyInfo(UInt32 index, BSTR *name, PROPID *id,
                                               VARTYPE *type)) {
    return Metadata->GetPropertyInfo(index, name, id, type);
}
int find(IFolderFolder *folder, const QString &name) {
    UInt32 count = 0;
    if (folder->GetNumberOfItems(&count) != S_OK)
        return -1;
    for (UInt32 i = 0; i < count; ++i) {
        NWindows::NCOM::CPropVariant value;
        if (folder->GetProperty(i, kpidName, &value) == S_OK && value.vt == VT_BSTR &&
            QString::fromWCharArray(value.bstrVal) == name)
            return int(i);
    }
    return -1;
}
QString crcText(const CListViewDialog &dialog) {
    const UString name = GetNameOfProperty(kpidCRC, nullptr);
    FOR_VECTOR(i, dialog.Strings)
    if (dialog.Strings[i] == name)
        return ToQString(dialog.Values[i]);
    return {};
}
QStringList crcValues(const CListViewDialog &dialog) {
    QStringList result;
    const UString name = GetNameOfProperty(kpidCRC, nullptr);
    FOR_VECTOR(i, dialog.Strings)
    if (dialog.Strings[i] == name)
        result << ToQString(dialog.Values[i]);
    return result;
}
ArchiveEntry entry(const QString &path, qint64 size, std::optional<quint32> crc,
                   bool directory = false) {
    ArchiveEntry result{path, size, 0, {}, directory};
    result.crc = crc;
    return result;
}
} // namespace

class AgentPropertyProjectionTests final : public QObject {
    Q_OBJECT
  private slots:
    void directoryPropertiesReachSourceConsumers_data() {
        QTest::addColumn<bool>("flat");
        QTest::addColumn<bool>("child");
        for (bool flat : {false, true})
            for (bool child : {false, true})
                QTest::newRow(qPrintable(QString("%1-%2").arg(flat).arg(child))) << flat << child;
    }
    void directoryPropertiesReachSourceConsumers() {
        QFETCH(bool, flat);
        QFETCH(bool, child);
        const QString base = child ? QStringLiteral("base 空/") : QString();
        const QString high = QStringLiteral("a high 空"), low = QStringLiteral("z low й");
        const QList<ArchiveEntry> entries{entry(base + high + "/first", 5, 0xfffffff0u),
                                          entry(base + high + "/deep/second", 3, 0x31u),
                                          entry(base + low + "/first", 2, 0x10u),
                                          entry(base + "unknown/file", 1, {}),
                                          entry(base + "zero/empty", 0, {}),
                                          entry(base + "explicit/", 0, {}, true),
                                          entry(base + "explicit/first", 1, 0x55u),
                                          entry(base + "defined/", 0, 0x77u, true),
                                          entry(base + "defined/first", 1, 0x99u)};
        CMyComPtr<IFolderFolder> root;
        QCOMPARE(CreateArchiveFolder(entries, UString("zip"), &root), S_OK);
        CMyComPtr<IFolderFolder> folder = root;
        if (child) {
            folder.Release();
            QCOMPARE(root->BindToFolder(L"base 空", &folder), S_OK);
            root.Release();
        }
        CMyComPtr<IFolderSetFlatMode> mode;
        QCOMPARE(folder.QueryInterface(IID_IFolderSetFlatMode, &mode), S_OK);
        QCOMPARE(mode->SetFlatMode(flat), S_OK);
        QCOMPARE(folder->LoadItems(), S_OK);
        const int highIndex = find(folder, high), lowIndex = find(folder, low);
        QVERIFY(highIndex >= 0 && lowIndex >= 0);
        CMyComPtr<IFolderCompare> compare;
        QCOMPARE(folder.QueryInterface(IID_IFolderCompare, &compare), S_OK);
        // UInt32 overflow is the original additive directory CRC policy.
        QVERIFY(compare->CompareItems(highIndex, lowIndex, kpidCRC, 0) > 0);
        CPanel panel(folder);
        QCOMPARE(panel.InitColumns(), S_OK);
        const int crcColumn = panel.GetColumns().FindItem_for_PropID(kpidCRC);
        QVERIFY(crcColumn >= 0);
        const auto &column = panel.GetColumns()[crcColumn];
        for (const auto &expected :
             QList<QPair<QString, QString>>{{high, "00000021"},
                                            {low, "00000010"},
                                            {"unknown", ""},
                                            {"zero", "00000000"},
                                            {"explicit", "00000055"},
                                            // Source detaches its local empty variant after
                                            // a directory provider writes a defined CRC.
                                            {"defined", ""}}) {
            const int index = find(folder, expected.first);
            QVERIFY(index >= 0);
            NWindows::NCOM::CPropVariant value;
            QCOMPARE(folder->GetProperty(index, kpidCRC, &value), S_OK);
            QCOMPARE(value.vt, VARTYPE(expected.second.isEmpty() ? VT_EMPTY : VT_UI4));
            if (!expected.second.isEmpty())
                QCOMPARE(value.ulVal, expected.second.toUInt(nullptr, 16));
            QCOMPARE(panel.GetItemText(index, column), expected.second);
            CRecordVector<UInt32> indices;
            indices.Add(index);
            PropertyProjection properties(panel, indices);
            QCOMPARE(crcText(properties), expected.second);
        }
        CMyComPtr<IFolderFolder> nested;
        const auto highName = high.toStdWString();
        QCOMPARE(folder->BindToFolder(highName.c_str(), &nested), S_OK);
        QCOMPARE(nested->LoadItems(), S_OK);
        NWindows::NCOM::CPropVariant aggregate;
        QCOMPARE(nested->GetFolderProperty(kpidCRC, &aggregate), S_OK);
        QCOMPARE(aggregate.vt, VARTYPE(VT_UI4));
        QCOMPARE(aggregate.ulVal, UInt32(0x21));
    }
    void directoryMetadataFailureRemainsRetryable_data() {
        QTest::addColumn<int>("result");
        QTest::newRow("failure") << int(E_FAIL);
        QTest::newRow("false") << int(S_FALSE);
        QTest::newRow("cancel") << int(E_ABORT);
    }
    void directoryMetadataFailureRemainsRetryable() {
        QFETCH(int, result);
        CMyComPtr<IFolderFolder> metadata;
        QCOMPARE(
            CreateArchiveMetadata(
                {entry("explicit/", 0, {}, true), entry("explicit/first", 1, 0x55u)}, &metadata),
            S_OK);
        auto *failure = new CPropertyFailure(metadata);
        CMyComPtr<IFolderFolder> provider = failure;
        auto *agentSpec = new CAgent;
        CMyComPtr<IInFolderArchive> agent = agentSpec;
        QCOMPARE(agentSpec->Init(provider, UString("zip")), S_OK);
        CMyComPtr<IFolderFolder> folder;
        QCOMPARE(agent->BindToRootFolder(&folder), S_OK);
        metadata.Release();
        agent.Release();
        provider.Release();
        QCOMPARE(folder->LoadItems(), S_OK);
        failure->CrcResult = HRESULT(result);
        NWindows::NCOM::CPropVariant value;
        QCOMPARE(folder->GetProperty(0, kpidCRC, &value), HRESULT(result));
        CPanel panel(folder);
        QCOMPARE(panel.InitColumns(), S_OK);
        const int crcColumn = panel.GetColumns().FindItem_for_PropID(kpidCRC);
        QVERIFY(crcColumn >= 0);
        QCOMPARE(panel.GetItemText(0, panel.GetColumns()[crcColumn]), QString("Error: "));
        CRecordVector<UInt32> indices;
        indices.Add(0);
        PropertyProjection failed(panel, indices);
        // The failed selected-item query is omitted. The distinct folder
        // summary section still legitimately contains the cached aggregate.
        QCOMPARE(crcValues(failed), QStringList{"00000055"});
        failure->CrcResult = S_OK;
        QCOMPARE(folder->GetProperty(0, kpidCRC, &value), S_OK);
        QCOMPARE(value.vt, VARTYPE(VT_UI4));
        QCOMPARE(value.ulVal, UInt32(0x55));
        QCOMPARE(panel.GetItemText(0, panel.GetColumns()[crcColumn]), QString("00000055"));
        PropertyProjection repaired(panel, indices);
        QCOMPARE(crcValues(repaired), (QStringList{"00000055", "00000055"}));
    }
};
QTEST_MAIN(AgentPropertyProjectionTests)
#include "agent_property_projection_tests.moc"
