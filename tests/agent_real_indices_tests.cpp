#include "7zip/PropID.h"
#include "CPP/7zip/UI/Agent/Agent.h"
#include "Windows/PropVariant.h"
#include "qtlocationfolder.h"
#include <QtTest>

class AgentRealIndicesTests : public QObject {
    Q_OBJECT

    static CMyComPtr<IFolderFolder> folder() {
        CMyComPtr<IFolderFolder> result;
        if (CreateArchiveFolder({{QStringLiteral("папка й/a space.txt"), 10, 4},
                                 {"root.bin", 11, 5},
                                 {QStringLiteral("папка й/deep/b.txt"), 12, 6},
                                 {"explicit/", 0, 0, {}, true},
                                 {"explicit/file.txt", 13, 7}},
                                UString("zip"), &result) != S_OK)
            return {};
        return result;
    }
    static int indexByPath(IFolderFolder *folder, const QString &path) {
        UInt32 count = 0;
        if (folder->GetNumberOfItems(&count) != S_OK)
            return -1;
        for (UInt32 i = 0; i < count; ++i) {
            NWindows::NCOM::CPropVariant name, prefix;
            if (folder->GetProperty(i, kpidName, &name) != S_OK || name.vt != VT_BSTR)
                continue;
            QString fullPath;
            if (folder->GetProperty(i, kpidPrefix, &prefix) == S_OK && prefix.vt == VT_BSTR)
                fullPath = QString::fromWCharArray(prefix.bstrVal);
            fullPath += QString::fromWCharArray(name.bstrVal);
            if (fullPath == path)
                return (int)i;
        }
        return -1;
    }
    static QList<UInt32> values(const CUIntVector &indices) {
        QList<UInt32> result;
        FOR_VECTOR(i, indices)
        result << indices[i];
        return result;
    }

  private slots:
    void normalSelectionExpandsDirectoriesAndSorts() {
        auto owner = folder();
        QVERIFY(owner);
        QCOMPARE(owner->LoadItems(), S_OK);
        auto *spec = static_cast<CAgentFolder *>((IFolderFolder *)owner);
        const int directory = indexByPath(owner, QStringLiteral("папка й"));
        const int explicitDirectory = indexByPath(owner, "explicit");
        const int rootFile = indexByPath(owner, "root.bin");
        QVERIFY(directory >= 0 && explicitDirectory >= 0 && rootFile >= 0);
        QCOMPARE(spec->GetRealIndex(directory), -1);
        QCOMPARE(spec->GetRealIndex(explicitDirectory), 3);
        QCOMPARE(spec->GetRealIndex(rootFile), 1);
        const UInt32 selected[] = {UInt32(rootFile), UInt32(directory)};
        CUIntVector real;
        real.Add(99); // Existing output must be replaced, never appended.
        spec->GetRealIndices(selected, 2, false, false, real);
        QCOMPARE(values(real), (QList<UInt32>{0, 1, 2}));
        const UInt32 explicitSelected[] = {UInt32(explicitDirectory)};
        spec->GetRealIndices(explicitSelected, 1, false, false, real);
        QCOMPARE(values(real), (QList<UInt32>{3, 4}));
        spec->GetRealIndices(nullptr, 0, false, false, real);
        QVERIFY(real.IsEmpty());
    }

    void flatSelectionHonorsExpansionAndPreservesOverlap() {
        auto owner = folder();
        QVERIFY(owner);
        CMyComPtr<IFolderSetFlatMode> flat;
        QCOMPARE(owner.QueryInterface(IID_IFolderSetFlatMode, &flat), S_OK);
        QCOMPARE(flat->SetFlatMode(1), S_OK);
        QCOMPARE(owner->LoadItems(), S_OK);
        auto *spec = static_cast<CAgentFolder *>((IFolderFolder *)owner);
        const int directory = indexByPath(owner, QStringLiteral("папка й"));
        const int explicitDirectory = indexByPath(owner, "explicit");
        const int file = indexByPath(owner, QStringLiteral("папка й/deep/b.txt"));
        QVERIFY(directory >= 0 && explicitDirectory >= 0 && file >= 0);
        QCOMPARE(spec->GetRealIndex(directory), -1);
        QCOMPARE(spec->GetRealIndex(explicitDirectory), 3);
        QCOMPARE(spec->GetRealIndex(file), 2);
        CUIntVector real;
        const UInt32 selected[] = {UInt32(directory)};
        spec->GetRealIndices(selected, 1, false, false, real);
        QVERIFY(real.IsEmpty());
        spec->GetRealIndices(selected, 1, false, true, real);
        QCOMPARE(values(real), (QList<UInt32>{0, 2}));
        const UInt32 explicitSelected[] = {UInt32(explicitDirectory)};
        spec->GetRealIndices(explicitSelected, 1, false, false, real);
        QCOMPARE(values(real), (QList<UInt32>{3}));
        spec->GetRealIndices(explicitSelected, 1, false, true, real);
        QCOMPARE(values(real), (QList<UInt32>{3, 4}));
        const UInt32 overlap[] = {UInt32(file), UInt32(directory), UInt32(file)};
        spec->GetRealIndices(overlap, 3, false, true, real);
        QCOMPARE(values(real), (QList<UInt32>{0, 2, 2, 2}));
        spec->GetRealIndices(nullptr, 0, false, true, real);
        QVERIFY(real.IsEmpty());
    }

    void largeReverseSelectionUsesArchiveOrder() {
        QList<ArchiveEntry> entries;
        for (int i = 0; i < 2048; ++i)
            entries.append({QString("file-%1").arg(i), 1, 1});
        CMyComPtr<IFolderFolder> owner;
        QCOMPARE(CreateArchiveFolder(entries, UString("zip"), &owner), S_OK);
        QCOMPARE(owner->LoadItems(), S_OK);
        CUIntVector selected;
        for (unsigned i = 2048; i > 0; --i)
            selected.Add(i - 1);
        CUIntVector real;
        static_cast<CAgentFolder *>((IFolderFolder *)owner)
            ->GetRealIndices(selected.ConstData(), selected.Size(), false, false, real);
        QCOMPARE(real.Size(), 2048u);
        FOR_VECTOR(i, real)
        QCOMPARE(real[i], i);
    }
};

QTEST_GUILESS_MAIN(AgentRealIndicesTests)
#include "agent_real_indices_tests.moc"
