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
    static constexpr int DataSector = 64;
    static constexpr int FatSector = 24;
    int clusterOffset(unsigned cluster) const { return (DataSector + cluster - 2) * sectorSize; }
    void fat(unsigned cluster, unsigned next);
    void set32(int offset, quint32 value);
    void set64(int offset, quint64 value);
    void refreshBootChecksum();
    void refreshEntryChecksum(int logicalRootEntry);
    int rootOffset(int logicalEntry) const;
};
