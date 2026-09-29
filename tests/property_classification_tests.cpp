#include "7zip/PropID.h"
#include "CPP/7zip/UI/FileManager/Panel.h"

#include <QTest>

class PropertyClassificationTests : public QObject {
    Q_OBJECT

  private slots:
    void groupedNumericProperties_data() {
        QTest::addColumn<quint32>("property");
        QTest::addColumn<bool>("grouped");
        QTest::newRow("file bytes") << quint32(kpidSize) << true;
        QTest::newRow("allocated bytes") << quint32(kpidPackSize) << true;
        QTest::newRow("folder count") << quint32(kpidNumSubDirs) << true;
        QTest::newRow("file count") << quint32(kpidNumSubFiles) << true;
        QTest::newRow("offset") << quint32(kpidOffset) << true;
        QTest::newRow("hard link count") << quint32(kpidLinks) << true;
        QTest::newRow("stream count") << quint32(kpidNumAltStreams) << true;
        QTest::newRow("stream bytes") << quint32(kpidAltStreamsSize) << true;
        QTest::newRow("error count") << quint32(kpidNumErrors) << true;
        QTest::newRow("archive tail") << quint32(kpidTailSize) << true;
        QTest::newRow("embedded stub") << quint32(kpidEmbeddedStubSize) << true;
        QTest::newRow("timestamp") << quint32(kpidMTime) << false;
        QTest::newRow("checksum") << quint32(kpidCRC) << false;
        QTest::newRow("inode identifier") << quint32(kpidINode) << false;
        QTest::newRow("virtual address") << quint32(kpidVa) << false;
        QTest::newRow("volume index") << quint32(kpidVolumeIndex) << false;
        QTest::newRow("error flags") << quint32(kpidErrorFlags) << false;
        QTest::newRow("name") << quint32(kpidName) << false;
        QTest::newRow("extension property") << quint32(1000) << false;
        QTest::newRow("unknown property") << quint32(0xFFFFFFFF) << false;
    }

    void groupedNumericProperties() {
        QFETCH(quint32, property);
        QFETCH(bool, grouped);
        QCOMPARE(IsSizeProp(property), grouped);
    }
};

QTEST_GUILESS_MAIN(PropertyClassificationTests)
#include "property_classification_tests.moc"
