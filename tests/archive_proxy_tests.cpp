#include "CPP/7zip/UI/Agent/Agent.h"
#include <QtTest>

class ArchiveProxyTests : public QObject {
    Q_OBJECT
  private slots:
    void implicitDirectoriesAndRecursiveProperties() {
        CProxyArc proxy;
        proxy.Load({ArchiveEntry{QStringLiteral("папка/один файл.txt"), 13, 5},
                    ArchiveEntry{QStringLiteral("папка/sub/ещё.txt"), 7, 3},
                    ArchiveEntry{"root.bin", 4, 2}, ArchiveEntry{"empty/", 0, 0, {}, true}});
        const int folder = proxy.FindDir(QStringLiteral("папка"));
        QVERIFY(folder > 0);
        QCOMPARE(proxy.Dirs[folder].Size, quint64(20));
        QCOMPARE(proxy.Dirs[folder].PackSize, quint64(8));
        QCOMPARE(proxy.Dirs[folder].NumSubFiles, quint32(2));
        QCOMPARE(proxy.Dirs[folder].NumSubDirs, quint32(1));
        QVERIFY(proxy.FindDir("empty") > 0);
        QCOMPARE(proxy.FindDir("missing"), -1);
        std::vector<unsigned> indices;
        proxy.AddRealIndices(unsigned(folder), indices);
        QCOMPARE(indices.size(), size_t(2));
        QVERIFY(std::find(indices.begin(), indices.end(), 0u) != indices.end());
        QVERIFY(std::find(indices.begin(), indices.end(), 1u) != indices.end());
        CAgentFolder root(proxy, 0, false);
        root.LoadItems();
        QCOMPARE(root.GetNumberOfItems(), 3u);
        CAgentFolder flat(proxy, unsigned(folder), true);
        flat.LoadItems();
        QCOMPARE(flat.GetNumberOfItems(), 3u);
    }
    void largeUnsortedDirectorySetRetainsEveryChild() {
        QList<ArchiveEntry> entries;
        for (int i = 4500; i > 0; --i)
            entries.append(
                ArchiveEntry{QString("directory-%1/file.bin").arg(i, 5, 10, QChar('0')), 1, 1});
        CProxyArc proxy;
        proxy.Load(entries);
        CAgentFolder root(proxy, 0, false);
        root.LoadItems();
        QCOMPARE(root.GetNumberOfItems(), 4500u);
        QCOMPARE(proxy.Dirs[0].Size, quint64(4500));
        for (int i : {1, 2048, 4096, 4500}) {
            const int folder = proxy.FindDir(QString("directory-%1").arg(i, 5, 10, QChar('0')));
            QVERIFY(folder > 0);
            QCOMPARE(proxy.Dirs[folder].NumSubFiles, quint32(1));
        }
    }
    void explicitDirectoryReusesImplicitNodeAndReloadClearsState() {
        CProxyArc proxy;
        proxy.Load({ArchiveEntry{"folder/file", 10, 6}, ArchiveEntry{"folder", 0, 0, {}, true}});
        QCOMPARE(proxy.Dirs.size(), size_t(2));
        const int folder = proxy.FindDir("FOLDER");
        QVERIFY(folder > 0);
        QVERIFY(proxy.Dirs[folder].IsLeaf());
        std::vector<unsigned> indices;
        proxy.AddRealIndices(unsigned(folder), indices);
        QCOMPARE(indices, (std::vector<unsigned>{1, 0}));
        proxy.Load({});
        QCOMPARE(proxy.Dirs.size(), size_t(1));
        QCOMPARE(proxy.FindDir("folder"), -1);
        QCOMPARE(proxy.Dirs[0].Size, quint64(0));
    }
    void unknownSizeDoesNotWrapDirectoryTotals() {
        CProxyArc proxy;
        proxy.Load({ArchiveEntry{"folder/known", 17, 8}, ArchiveEntry{"folder/unknown", -1, -1},
                    ArchiveEntry{"unknown-only/file", -1, -1}});
        const int folder = proxy.FindDir("folder");
        QVERIFY(folder > 0);
        QCOMPARE(proxy.Dirs[folder].Size, quint64(17));
        QCOMPARE(proxy.Dirs[folder].PackSize, quint64(8));
        const int unknown = proxy.FindDir("unknown-only");
        QVERIFY(unknown > 0);
        QCOMPARE(proxy.Dirs[unknown].Size, quint64(0));
        QCOMPARE(proxy.Dirs[unknown].PackSize, quint64(0));
        QCOMPARE(proxy.Dirs[0].Size, quint64(17));
        QCOMPARE(proxy.Dirs[0].PackSize, quint64(8));
    }
};
QTEST_GUILESS_MAIN(ArchiveProxyTests)
#include "archive_proxy_tests.moc"
