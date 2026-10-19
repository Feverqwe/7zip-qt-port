"""Deterministic FAT volumes for fragmented reads and directory recovery."""
import struct


PAYLOAD = bytes(range(256)) * 4 + b"fragmented FAT tail\n"
LONG_NAME = "данные 空 fragmented.bin"


def checksum(alias):
    value = 0
    for byte in alias:
        value = (((value & 1) << 7) + (value >> 1) + byte) & 255
    return value


def entry(alias, cluster=0, size=0, attr=0x20):
    result = bytearray(32)
    result[:11] = alias
    result[11] = attr
    struct.pack_into("<H", result, 20, cluster >> 16)
    struct.pack_into("<H", result, 26, cluster & 65535)
    struct.pack_into("<I", result, 28, size)
    return result


def long_entries(name, alias):
    raw = name.encode("utf-16le", errors="surrogatepass") + b"\0\0"
    raw += b"\xff\xff" * ((-len(raw) // 2) % 13)
    result = []
    for index in reversed(range(len(raw) // 26)):
        part = raw[index * 26:(index + 1) * 26]
        record = bytearray(32)
        record[0] = index + 1 | (0x40 if not result else 0)
        record[11] = 0x0F
        record[13] = checksum(alias)
        record[1:11], record[14:26], record[28:32] = part[:10], part[10:22], part[22:]
        result.append(record)
    return result


class Volume:
    def __init__(self, bits=12, sector=512, sectors_per_cluster=1, fats=2):
        self.bits, self.sector, self.spc, self.fats = bits, sector, sectors_per_cluster, fats
        self.cluster_size = sector * sectors_per_cluster
        self.clusters = 768 if bits == 12 else 4096 if bits == 16 else 1024
        if bits == 32 and self.cluster_size >= len(PAYLOAD):
            self.clusters = 32
        self.fat_sectors = ((self.clusters + 2) * bits + 8 * sector - 1) // (8 * sector)
        self.reserved = 32 if bits == 32 else 1
        self.root_sectors = 0 if bits == 32 else 4
        self.root_sector = self.reserved + self.fat_sectors * fats
        self.data_sector = self.root_sector + self.root_sectors
        self.sectors = self.data_sector + self.clusters * sectors_per_cluster
        needed = (len(PAYLOAD) + self.cluster_size - 1) // self.cluster_size
        self.chain = ([341, 700, 24] if bits == 12 and needed > 1 else [3, 17, 8])[:needed]
        self.data = bytearray(self.sectors * sector)
        self.data[:11] = b"\xeb\x58\x90MSDOS5.0"
        struct.pack_into("<HBHBHHBHHHII", self.data, 11, sector, sectors_per_cluster,
                         self.reserved, fats, self.root_sectors * sector // 32,
                         self.sectors if self.sectors < 65536 else 0,
                         0xF8, self.fat_sectors if bits != 32 else 0, 32, 64, 0,
                         self.sectors if self.sectors >= 65536 else 0)
        self.data[510:512] = b"\x55\xaa"
        self.eof = (1 << min(bits, 28)) - 1
        if bits == 32:
            struct.pack_into("<IHHIHH", self.data, 36, self.fat_sectors, 0, 0, 2, 1, 6)
            self.data[66] = 0x29
            struct.pack_into("<I", self.data, sector, 0x41615252)
            struct.pack_into("<III", self.data, sector + 484, 0x61417272, 0xFFFFFFFF, 3)
            struct.pack_into("<I", self.data, sector + 508, 0xAA550000)
            self.root = self.offset(2)
            self.set_fat(2, self.eof)
        else:
            self.data[38] = 0x29
            self.root = self.root_sector * sector
        self.set_fat(0, self.eof & ~255 | 0xF8)
        self.set_fat(1, self.eof)
        for index, cluster in enumerate(self.chain):
            self.set_fat(cluster, self.chain[index + 1] if index + 1 < needed else self.eof)
            chunk = PAYLOAD[index * self.cluster_size:(index + 1) * self.cluster_size]
            self.data[self.offset(cluster):self.offset(cluster) + len(chunk)] = chunk
        self.alias = b"DATA~1  BIN"
        self.records = long_entries(LONG_NAME, self.alias)
        self.records.append(entry(self.alias, self.chain[0], len(PAYLOAD)))
        self.records.append(entry(b"EMPTY   TXT"))
        self.write_directory()

    def offset(self, cluster):
        return (self.data_sector + (cluster - 2) * self.spc) * self.sector

    def set_fat(self, cluster, value, copy=None):
        for index in range(self.fats) if copy is None else [copy]:
            base = (self.reserved + index * self.fat_sectors) * self.sector
            if self.bits == 12:
                pos = base + cluster * 3 // 2
                old = struct.unpack_from("<H", self.data, pos)[0]
                encoded = old & (0x000F if cluster & 1 else 0xF000) | value << (4 if cluster & 1 else 0)
                struct.pack_into("<H", self.data, pos, encoded)
            else:
                struct.pack_into("<H" if self.bits == 16 else "<I", self.data,
                                 base + cluster * (self.bits // 8), value)

    def write_directory(self):
        raw = b"".join(self.records) + bytes(32)
        capacity = self.cluster_size if self.bits == 32 else self.root_sectors * self.sector
        assert len(raw) <= capacity
        self.data[self.root:self.root + capacity] = bytes(capacity)
        self.data[self.root:self.root + len(raw)] = raw


def legacy_volume(kib=160):
    spc, root_entries, media, fat_sectors = {
        160: (1, 64, 0xFE, 1), 180: (1, 64, 0xFC, 2),
        320: (2, 112, 0xFF, 1), 360: (2, 112, 0xFD, 2),
    }[kib]
    v = object.__new__(Volume)
    v.bits, v.sector, v.spc, v.fats, v.reserved = 12, 512, spc, 2, 1
    v.cluster_size, v.fat_sectors = spc * 512, fat_sectors
    v.root_sector = 1 + 2 * fat_sectors
    v.root_sectors = root_entries // 16
    v.data_sector = v.root_sector + v.root_sectors
    v.sectors, v.eof = kib * 2, 0xFFF
    v.clusters = (v.sectors - v.data_sector) // spc
    v.data = bytearray(kib * 1024)
    v.data[:3] = b"\xeb\x3c\x90"
    v.root = v.root_sector * 512
    v.set_fat(0, 0xF00 | media)
    v.set_fat(1, v.eof)
    needed = (len(PAYLOAD) + v.cluster_size - 1) // v.cluster_size
    v.chain = [2, 5, 9][:needed]
    for i, cluster in enumerate(v.chain):
        v.set_fat(cluster, v.chain[i + 1] if i + 1 < needed else v.eof)
        part = PAYLOAD[i * v.cluster_size:(i + 1) * v.cluster_size]
        v.data[v.offset(cluster):v.offset(cluster) + len(part)] = part
    v.alias = b"LEGACY  BIN"
    v.records = [entry(v.alias, v.chain[0], len(PAYLOAD))]
    v.write_directory()
    return v


if __name__ == "__main__":
    import sys
    sys.stdout.buffer.write(legacy_volume().data)
