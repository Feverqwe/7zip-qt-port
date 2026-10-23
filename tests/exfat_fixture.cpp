#include "exfat_fixture.h"
#include <QtEndian>
#include <algorithm>

namespace {
void put16(QByteArray &bytes, int offset, quint16 value) {
    qToLittleEndian(value, bytes.data() + offset);
}
QByteArray entrySet(const QString &name, unsigned cluster, quint64 size, quint64 valid,
                    bool directory, bool contiguous) {
    const int names = (name.size() + 14) / 15;
    QByteArray set((2 + names) * 32, '\0');
    set[0] = char(0x85);
    set[1] = char(1 + names);
    put16(set, 4, directory ? 0x10 : 0x20);
    put16(set, 8, (12 << 11) | (34 << 5) | 28);
    put16(set, 10, ((2026 - 1980) << 9) | (10 << 5) | 17);
    put16(set, 12, (12 << 11) | (34 << 5) | 28);
    put16(set, 14, ((2026 - 1980) << 9) | (10 << 5) | 17);
    set[20] = 123;
    set[21] = 123;
    set[22] = char(0x80 | 12); // UTC+3
    set[23] = char(0x80 | 12);
    set[24] = char(0x80 | 12);
    set[32] = char(0xc0);
    set[33] = contiguous ? 3 : 1;
    set[35] = char(name.size());
    quint16 hash = 0;
    for (const QChar ch : name) {
        const auto upper = ch.toUpper().unicode();
        hash = quint16((hash >> 1) | (hash << 15)) + quint8(upper);
        hash = quint16((hash >> 1) | (hash << 15)) + quint8(upper >> 8);
    }
    put16(set, 36, hash);
    qToLittleEndian(valid, set.data() + 40);
    qToLittleEndian(quint32(cluster), set.data() + 52);
    qToLittleEndian(size, set.data() + 56);
    for (int i = 0; i < names; ++i) {
        set[(i + 2) * 32] = char(0xc1);
        for (int j = 0; j < 15 && i * 15 + j < name.size(); ++j)
            put16(set, (i + 2) * 32 + 2 + j * 2, name[i * 15 + j].unicode());
    }
    quint16 sum = 0;
    for (int i = 0; i < set.size(); ++i)
        if (i != 2 && i != 3)
            sum = quint16((sum >> 1) | (sum << 15)) + quint8(set[i]);
    put16(set, 2, sum);
    return set;
}
} // namespace

void ExfatFixture::set32(int offset, quint32 value) {
    qToLittleEndian(value, image.data() + offset);
}
void ExfatFixture::emptyDirectory(int entry, const QString &name, unsigned cluster) {
    const QByteArray records = entrySet(name, cluster, 0, 0, true, true);
    for (int i = 0; i < records.size() / 32; ++i)
        std::copy_n(records.constData() + i * 32, 32, image.data() + rootOffset(entry + i));
}
void ExfatFixture::set64(int offset, quint64 value) {
    qToLittleEndian(value, image.data() + offset);
}
void ExfatFixture::setNestedFile(const QString &name, const QByteArray &payload) {
    const auto records =
        entrySet(name, 7, quint64(payload.size()), quint64(payload.size()), false, true);
    Q_ASSERT(records.size() <= sectorSize && payload.size() <= 2 * sectorSize);
    nestedName = name;
    std::fill_n(image.data() + clusterOffset(6), sectorSize, '\0');
    std::copy(records.begin(), records.end(), image.data() + clusterOffset(6));
    std::fill_n(image.data() + clusterOffset(7), 2 * sectorSize, '\0');
    std::copy(payload.begin(), payload.end(), image.data() + clusterOffset(7));
}
void ExfatFixture::fat(unsigned cluster, unsigned next) {
    set32(FatSector * sectorSize + cluster * 4, next);
}
void ExfatFixture::directoryFault(const QString &fault) {
    const int directory = 2 + (2 + (fragmentedName.size() + 14) / 15);
    if (fault == "entry-checksum")
        image[rootOffset(2) + 4] ^= 1;
    else if (fault == "extent-range") {
        set32(rootOffset(3) + 20, 10000);
        refreshEntryChecksum(2);
    } else if (fault == "name-length") {
        image[rootOffset(3) + 3] = 1;
        refreshEntryChecksum(2);
    } else if (fault == "traversal-name") {
        image[rootOffset(4) + 2] = '/';
        refreshEntryChecksum(2);
    } else if (fault == "directory-cycle") {
        set32(rootOffset(directory + 1) + 20, 2);
        refreshEntryChecksum(directory);
    } else if (fault == "child-checksum") {
        image[clusterOffset(6) + 4] ^= 1;
    } else if (fault == "child-chain") {
        image[rootOffset(directory + 1) + 1] = 1;
        set64(rootOffset(directory + 1) + 8, 2 * sectorSize);
        set64(rootOffset(directory + 1) + 24, 2 * sectorSize);
        refreshEntryChecksum(directory);
        fat(6, 0);
        const int records = 2 + (nestedName.size() + 14) / 15;
        std::fill_n(image.data() + clusterOffset(6) + records * 32, sectorSize - records * 32,
                    char(0x80));
    } else if (fault == "alias") {
        fat(10, 11);
        fat(11, 0xffffffff);
        const int bitmap = clusterOffset(3) + 1;
        image[bitmap] = char(quint8(image[bitmap]) | 2);
        emptyDirectory(31, "alias", 6);
        set64(rootOffset(32) + 8, sectorSize);
        set64(rootOffset(32) + 24, sectorSize);
        refreshEntryChecksum(31);
    } else {
        Q_ASSERT(false);
    }
}
int ExfatFixture::rootOffset(int entry) const {
    const int perCluster = sectorSize / 32;
    const unsigned clusters[] = {2, 10, 11};
    return clusterOffset(clusters[entry / perCluster]) + (entry % perCluster) * 32;
}
void ExfatFixture::refreshEntryChecksum(int entry) {
    const int count = quint8(image[rootOffset(entry) + 1]) + 1;
    quint16 sum = 0;
    for (int i = 0; i < count * 32; ++i)
        if (i != 2 && i != 3)
            sum = quint16((sum >> 1) | (sum << 15)) +
                  quint8(image[rootOffset(entry + i / 32) + i % 32]);
    put16(image, rootOffset(entry) + 2, sum);
}
void ExfatFixture::refreshBootChecksum() {
    quint32 sum = 0;
    for (int i = 0; i < 11 * sectorSize; ++i)
        if (i != 106 && i != 107 && i != 112)
            sum = ((sum >> 1) | (sum << 31)) + quint8(image[i]);
    for (int i = 11 * sectorSize; i < 12 * sectorSize; i += 4)
        set32(i, sum);
    std::copy_n(image.constData(), 12 * sectorSize, image.data() + 12 * sectorSize);
}

ExfatFixture::ExfatFixture(int logicalSector)
    : image(512 * logicalSector, '\0'), sectorSize(logicalSector) {
    image[0] = char(0xeb);
    image[1] = 0x76;
    image[2] = char(0x90);
    std::copy_n("EXFAT   ", 8, image.data() + 3);
    set64(72, 512);
    set32(80, FatSector);
    set32(84, fatSectors);
    set32(88, dataSector);
    set32(92, 384);
    set32(96, 2);
    put16(image, 104, 0x100);
    image[108] = logicalSector == 4096 ? 12 : 9;
    image[109] = 0;
    image[110] = 1;
    image[111] = char(0x80);
    image[112] = char(0xff);
    put16(image, 510, 0xaa55);
    for (int sector = 1; sector <= 8; ++sector)
        set32(sector * sectorSize + sectorSize - 4, 0xaa550000);
    fat(0, 0xfffffff8);
    fat(1, 0xffffffff);
    fat(2, 10);
    fat(10, 0xffffffff);
    fat(3, 0xffffffff);
    fat(4, 0xffffffff);
    fat(5, 9);
    fat(9, 0xffffffff);
    const int root = clusterOffset(2);
    image[root] = char(0x81);
    set32(root + 20, 3);
    set64(root + 24, 48);
    image[root + 32] = char(0x82);
    set32(root + 32 + 20, 20);
    set64(root + 32 + 24, 65536 * 2);
    quint32 upcaseChecksum = 0;
    for (unsigned i = 0; i < 65536; ++i) {
        const quint16 value = QChar(ushort(i)).toUpper().unicode();
        put16(image, clusterOffset(20) + i * 2, value);
        upcaseChecksum = ((upcaseChecksum >> 1) | (upcaseChecksum << 31)) + quint8(value);
        upcaseChecksum = ((upcaseChecksum >> 1) | (upcaseChecksum << 31)) + quint8(value >> 8);
    }
    set32(root + 32 + 4, upcaseChecksum);
    auto allocated = [&](unsigned cluster) {
        const int offset = clusterOffset(3) + (cluster - 2) / 8;
        image[offset] = char(quint8(image[offset]) | (1u << ((cluster - 2) % 8)));
    };
    for (unsigned cluster : {2u, 3u, 5u, 6u, 7u, 8u, 9u, 10u, 12u})
        allocated(cluster);
    for (unsigned cluster = 20; cluster < 276; ++cluster)
        allocated(cluster);
    int entry = 2;
    auto append = [&](const QByteArray &set) {
        for (int i = 0; i < set.size() / 32; ++i)
            std::copy_n(set.constData() + i * 32, 32, image.data() + rootOffset(entry++));
    };
    append(entrySet(fragmentedName, 5, sectorSize + 188, sectorSize + 188, false, false));
    append(entrySet(directoryName, 6, sectorSize, sectorSize, true, true));
    append(entrySet(QStringLiteral("empty.txt"), 0, 0, 0, false, true));
    append(entrySet(longName, 12, 3, 3, false, true));
    const auto nested = entrySet(nestedName, 7, 2 * sectorSize, 4, false, true);
    std::copy(nested.begin(), nested.end(), image.data() + clusterOffset(6));
    std::fill_n(image.data() + clusterOffset(5), sectorSize, 'A');
    std::fill_n(image.data() + clusterOffset(9), 188, 'B');
    std::fill_n(image.data() + clusterOffset(7), 2 * sectorSize, 'S');
    std::copy_n("tail", 4, image.data() + clusterOffset(7));
    std::copy_n("end", 3, image.data() + clusterOffset(12));
    refreshBootChecksum();
}

void TexfatFixture::fatCopy(unsigned copy, unsigned cluster, unsigned next) {
    set32((FatSector + copy * fatSectors) * sectorSize + cluster * 4, next);
}

int TexfatFixture::bitmapOffset(unsigned bitmap, unsigned cluster) const {
    const unsigned byte = (cluster - 2) / 8;
    const unsigned start = bitmap ? 13 : 3;
    const unsigned tail = bitmap ? 15 : 14;
    return clusterOffset(byte < unsigned(sectorSize) ? start : tail) + byte % sectorSize;
}

void TexfatFixture::allocated(unsigned bitmap, unsigned cluster, bool used) {
    const int offset = bitmapOffset(bitmap, cluster);
    const unsigned mask = 1u << ((cluster - 2) % 8);
    image[offset] = char(used ? quint8(image[offset]) | mask : quint8(image[offset]) & ~mask);
}

TexfatFixture::TexfatFixture(unsigned activeFat, int logicalSector, bool fragmentedBitmaps)
    : ExfatFixture(logicalSector) {
    unsigned clusterCount = 384;
    unsigned dataTail = 9;
    if (fragmentedBitmaps) {
        // A >512-byte bitmap needs multiple clusters. Move the original heap
        // after two larger FATs and keep its logical cluster numbers intact.
        QByteArray original = image;
        image = QByteArray(8192 * sectorSize, '\0');
        std::copy_n(original.constData(), 24 * sectorSize, image.data());
        std::copy_n(original.constData() + FatSector * sectorSize, fatSectors * sectorSize,
                    image.data() + FatSector * sectorSize);
        const int originalHeap = dataSector * sectorSize;
        dataSector = 256;
        fatSectors = 64;
        std::copy_n(original.constData() + originalHeap, 384 * sectorSize,
                    image.data() + dataSector * sectorSize);
        clusterCount = 8192 - dataSector;
        set64(72, 8192);
        set32(84, fatSectors);
        set32(88, dataSector);
        set32(92, clusterCount);
        dataTail = 5000;
        std::fill_n(image.data() + clusterOffset(dataTail), 188, 'B');
        fat(5, dataTail);
        fat(dataTail, 0xffffffff);
        fat(3, 14);
        fat(14, 0xffffffff);
        fat(13, 15);
        fat(15, 0xffffffff);
        allocated(0, 14, true);
        allocated(0, 15, true);
        allocated(0, dataTail, true);
    } else {
        fat(13, 0xffffffff);
    }
    fat(10, 11);
    fat(11, 0xffffffff);
    allocated(0, 11, true);
    allocated(0, 13, true);
    const unsigned bitmapSize = (clusterCount + 7) / 8;
    set64(rootOffset(0) + 24, bitmapSize);
    const int second = rootOffset(31);
    image[second] = char(0x81);
    image[second + 1] = 1;
    set32(second + 20, 13);
    set64(second + 24, bitmapSize);
    image[rootOffset(32)] = char(0xa1); // TexFAT padding: benign primary entry.
    image[rootOffset(33)] = char(0xa2); // Legacy benign security-table descriptor.
    for (unsigned byte = 0; byte < bitmapSize; ++byte) {
        const unsigned cluster = byte * 8 + 2;
        image[bitmapOffset(1, cluster)] = image[bitmapOffset(0, cluster)];
    }
    const int firstFat = FatSector * sectorSize;
    std::copy_n(image.constData() + firstFat, fatSectors * sectorSize,
                image.data() + firstFat + fatSectors * sectorSize);
    const unsigned inactive = activeFat ^ 1;
    fatCopy(inactive, 5, 8); // Old data would return the 'S' payload, not the new 'B'.
    allocated(inactive, dataTail, false);
    image[110] = 2;
    put16(image, 106, quint16(activeFat));
    refreshBootChecksum();
    // Backup VolumeFlags are stale by definition; only the main copy is current.
    put16(image, 12 * sectorSize + 106, quint16(inactive | 2));
}
