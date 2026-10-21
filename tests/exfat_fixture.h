#pragma once
#include <QByteArray>
#include <QString>

// Deterministic, specification-built volume: fragmented data, a nested file with
// ValidDataLength < DataLength, empty file, and a 255-code-unit filename whose
// entry set crosses a fragmented root-directory cluster boundary.
struct ExfatFixture {
    QByteArray image;
    QString fragmentedName = QStringLiteral("fragment имя 空.bin");
    QString directoryName = QStringLiteral("папка 空");
    QString nestedName = QStringLiteral("tail 😀 空.txt");
    QString longName = QString(251, QChar('x')) + QStringLiteral(".bin");
    explicit ExfatFixture(int logicalSector = 512);
    int sectorSize;
    int dataSector = 64;
    int fatSectors = 4;
    static constexpr int FatSector = 24;
    int clusterOffset(unsigned cluster) const { return (dataSector + cluster - 2) * sectorSize; }
    void fat(unsigned cluster, unsigned next);
    void set32(int offset, quint32 value);
    void set64(int offset, quint64 value);
    void refreshBootChecksum();
    void refreshEntryChecksum(int logicalRootEntry);
    void emptyDirectory(int rootEntry, const QString &name, unsigned cluster = 0);
    int rootOffset(int logicalEntry) const;
};

// The inactive pair deliberately describes stale data. Reading it either
// returns different payload bytes or disagrees with the active bitmap.
struct TexfatFixture : ExfatFixture {
    explicit TexfatFixture(unsigned activeFat, int logicalSector = 512,
                           bool fragmentedBitmaps = false);
    void fatCopy(unsigned copy, unsigned cluster, unsigned next);
    void allocated(unsigned bitmap, unsigned cluster, bool used);
    int bitmapOffset(unsigned bitmap, unsigned cluster) const;
};
