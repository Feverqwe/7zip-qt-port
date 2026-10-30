#include "Windows/PropVariant.h"
#include "formats/fat/support/fat_test_support.h"
#include <QLibrary>
#include <QSet>
#include <QVector>
#include <QtEndian>
#include <QtTest>

namespace {
// FAT32's maximum Linux geometry includes this physical entry, but the
// same value in a next-link field is a terminal BAD marker.
constexpr UInt32 Bad = 0x0ffffff7;
constexpr UInt32 End = 0x0fffffff;
constexpr UInt32 FatSectors = 0x200000;
constexpr UInt32 DataSector = 32 + FatSectors;
constexpr UInt64 FileSize = UInt64(DataSector + Bad - 1) * 512;

class MaximumInput final : public IInStream, public CMyUnknownImp {
    Z7_COM_UNKNOWN_IMP_2(ISequentialInStream, IInStream)
    UInt64 position_ = 0;
    struct Region {
        UInt64 offset;
        QByteArray bytes;
    };
    QVector<Region> regions_;
    void word(QByteArray &bytes, int offset, UInt32 value) {
        qToLittleEndian(value, bytes.data() + offset);
    }
    void entry(QByteArray &directory, int index, const char *name, UInt32 cluster, UInt32 size,
               Byte attributes = 0x20) {
        char *record = directory.data() + index * 32;
        memcpy(record, name, 11);
        record[11] = char(attributes);
        qToLittleEndian(UInt16(cluster), record + 26);
        qToLittleEndian(UInt16(cluster >> 16), record + 20);
        qToLittleEndian(size, record + 28);
    }

  public:
    MaximumInput() {
        QByteArray header(512, '\0');
        header.replace(0, 3, QByteArray::fromHex("eb5890"));
        qToLittleEndian(UInt16(512), header.data() + 11);
        header[13] = 1;
        qToLittleEndian(UInt16(32), header.data() + 14);
        header[16] = 1;
        header[21] = char(0xf8);
        word(header, 32, UInt32(FileSize / 512));
        word(header, 36, FatSectors);
        word(header, 44, 2);
        qToLittleEndian(UInt16(1), header.data() + 48);
        header[66] = 0x29;
        qToLittleEndian(UInt16(0xaa55), header.data() + 510);
        regions_.append({0, header});
        QByteArray info(512, '\0');
        word(info, 0, 0x41615252);
        word(info, 484, 0x61417272);
        word(info, 488, 0xffffffff);
        word(info, 492, 3);
        word(info, 508, 0xaa550000);
        regions_.append({512, info});
        QByteArray fat(512, '\0');
        word(fat, 0, 0x0ffffff8);
        word(fat, 4, End);
        word(fat, 8, End);
        word(fat, 12, Bad);
        word(fat, 16, End);
        word(fat, 20, Bad);
        regions_.append({32 * 512, fat});
        QByteArray last(4, '\0');
        word(last, 0, End);
        regions_.append({UInt64(32) * 512 + UInt64(Bad) * 4, last});
        QByteArray directory(512, '\0');
        entry(directory, 0, "BROKEN  BIN", 3, 1024);
        entry(directory, 1, "HIGH    BIN", Bad, 512);
        entry(directory, 2, "GOOD    BIN", 4, 512);
        entry(directory, 3, "FOLDER     ", 5, 0, 0x10);
        regions_.append({UInt64(DataSector) * 512, directory});
        regions_.append({UInt64(DataSector + 1) * 512, QByteArray(512, 'A')});
        regions_.append({UInt64(DataSector + 2) * 512, QByteArray(512, 'C')});
        QByteArray child(512, '\0');
        entry(child, 0, "KNOWN   TXT", 4, 512);
        for (int i = 1; i < 16; ++i)
            child[i * 32] = char(0xe5);
        regions_.append({UInt64(DataSector + 3) * 512, child});
        regions_.append({UInt64(DataSector + Bad - 2) * 512, QByteArray(512, 'B')});
    }
    Z7_COM7F_IMF(Read(void *data, UInt32 size, UInt32 *processed)) override {
        if (processed)
            *processed = 0;
        size = UInt32(qMin<UInt64>(size, position_ < FileSize ? FileSize - position_ : 0));
        if (size) {
            memset(data, 0, size);
            for (const auto &region : regions_) {
                const UInt64 begin = qMax(position_, region.offset);
                const UInt64 end =
                    qMin(position_ + size, region.offset + UInt64(region.bytes.size()));
                if (begin < end)
                    memcpy(static_cast<char *>(data) + (begin - position_),
                           region.bytes.constData() + (begin - region.offset), size_t(end - begin));
            }
        }
        position_ += size;
        if (processed)
            *processed = size;
        return S_OK;
    }
    Z7_COM7F_IMF(Seek(Int64 offset, UInt32 origin, UInt64 *position)) override {
        if (origin > STREAM_SEEK_END)
            return E_INVALIDARG;
        const Int64 base = origin == STREAM_SEEK_SET   ? 0
                           : origin == STREAM_SEEK_CUR ? Int64(position_)
                                                       : Int64(FileSize);
        if (offset < -base || (offset > 0 && base > INT64_MAX - offset))
            return E_INVALIDARG;
        position_ = UInt64(base + offset);
        if (position)
            *position = position_;
        return S_OK;
    }
};
} // namespace

class FatMaximumTests final : public QObject {
    Q_OBJECT
  private slots:
    void terminalLinkIsNotAPhysicalAddress() {
        QLibrary module(QString::fromUtf8(SEVENZIP_FAT_MODULE));
        QVERIFY(module.load());
        const auto property =
            reinterpret_cast<Func_GetHandlerProperty2>(module.resolve("GetHandlerProperty2"));
        const auto create = reinterpret_cast<Func_CreateObject>(module.resolve("CreateObject"));
        const auto formats =
            reinterpret_cast<Func_GetNumberOfFormats>(module.resolve("GetNumberOfFormats"));
        QVERIFY(property && create && formats);
        UInt32 formatCount = 0;
        QCOMPARE(formats(&formatCount), S_OK);
        GUID guid;
        bool found = false;
        for (UInt32 i = 0; i < formatCount; ++i) {
            NWindows::NCOM::CPropVariant name, clsid;
            QCOMPARE(property(i, NArchive::NHandlerPropID::kName, &name), S_OK);
            if (name.vt != VT_BSTR || QString::fromWCharArray(name.bstrVal) != "FAT")
                continue;
            QCOMPARE(property(i, NArchive::NHandlerPropID::kClassID, &clsid), S_OK);
            QCOMPARE(clsid.vt, VARTYPE(VT_BSTR));
            memcpy(&guid, clsid.bstrVal, sizeof(guid));
            found = true;
            break;
        }
        QVERIFY(found);
        CMyComPtr<IInArchive> archive;
        QCOMPARE(create(&guid, &IID_IInArchive, reinterpret_cast<void **>(&archive)), S_OK);
        CMyComPtr<IInStream> source = new MaximumInput;
        QCOMPARE(archive->Open(source, nullptr, nullptr), S_OK);
        UInt32 count = 0;
        QCOMPARE(archive->GetNumberOfItems(&count), S_OK);
        QCOMPARE(count, UInt32(5));
        QSet<QString> paths;
        for (UInt32 i = 0; i < count; ++i) {
            NWindows::NCOM::CPropVariant path, error;
            QCOMPARE(archive->GetProperty(i, kpidPath, &path), S_OK);
            QCOMPARE(path.vt, VARTYPE(VT_BSTR));
            const QString name = QString::fromWCharArray(path.bstrVal).replace('\\', '/');
            paths.insert(name);
            const bool broken = name == "BROKEN.BIN";
            const bool directory = name == "FOLDER";
            QVERIFY(broken || directory || name == "HIGH.BIN" || name == "GOOD.BIN" ||
                    name == "FOLDER/KNOWN.TXT");
            CMyComPtr2<IArchiveExtractCallback, FatTestSupport::Extract> callback;
            callback.Create_if_Empty();
            QCOMPARE(archive->Extract(&i, 1, 0, callback), S_OK);
            QCOMPARE(callback->output->bytes,
                     QByteArray(directory ? 0 : 512, broken               ? 'A'
                                                     : name == "HIGH.BIN" ? 'B'
                                                                          : 'C'));
            QCOMPARE(callback->result,
                     Int32(broken || directory ? NArchive::NExtract::NOperationResult::kDataError
                                               : NArchive::NExtract::NOperationResult::kOK));
            QCOMPARE(archive->GetProperty(i, kpidError, &error), S_OK);
            QCOMPARE(error.vt, VARTYPE(broken || directory ? VT_BSTR : VT_EMPTY));
        }
        const QSet<QString> expectedPaths{"BROKEN.BIN", "HIGH.BIN", "GOOD.BIN", "FOLDER",
                                          "FOLDER/KNOWN.TXT"};
        QCOMPARE(paths, expectedPaths);
        QCOMPARE(archive->Close(), S_OK);
    }
};
QTEST_GUILESS_MAIN(FatMaximumTests)
#include "fat_maximum_tests.moc"
