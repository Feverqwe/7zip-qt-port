#include "7zip/Common/StreamObjects.h"
#include "7zip/Compress/ZlibDecoder.h"
#include "Xxh64.h"
#include <QTest>
#include <algorithm>
#include <array>
#include <cstdint>
#include <vector>

extern "C" uint32_t xxh32(const void *, size_t, uint32_t);

namespace {
std::vector<Byte> bytes(size_t size, size_t offset = 0) {
    std::vector<Byte> result(size + offset);
    for (size_t i = 0; i < size; ++i)
        result[offset + i] = Byte(i * 37 + 11);
    return result;
}

struct HashVector {
    size_t size;
    UInt64 hash64;
    uint32_t hash32;
    uint32_t seeded32;
};

// Frozen results from the original pinned SDK/Linux implementations, before replacement.
const HashVector vectors[] = {
    {0, 0xef46db3751d8e999ULL, 0x02cc5d05U, 0x9061da9dU},
    {1, 0xf592c0c7639c4cb6ULL, 0x3fe5a29dU, 0xbba06164U},
    {3, 0x22c08528601d4f27ULL, 0x4021af28U, 0x750ca85bU},
    {15, 0x90a9714eb00e8d29ULL, 0x162986ebU, 0x65542919U},
    {16, 0xc6843cef99721174ULL, 0x96ac9fa6U, 0xec271498U},
    {17, 0xf72e5e622b272ad6ULL, 0x3cdfb1a2U, 0x9b7d8e5eU},
    {31, 0xe4a0e629e519a4aeULL, 0xdf853fbfU, 0xecd504fbU},
    {32, 0xcc6b8aaada790b2dULL, 0xda8072dcU, 0xa260c79dU},
    {33, 0x35ec49850475a832ULL, 0x3c8fd07dU, 0x656480b0U},
    {63, 0xbf9f0ba3cf95b28aULL, 0x00bf10c5U, 0x37b89d50U},
    {64, 0x155ccce4bf32befcULL, 0x043913c6U, 0x497339d3U},
    {65, 0xda5ee441a595cfb8ULL, 0x3922f755U, 0x815de58cU},
    {5550, 0x9b31258061cc115aULL, 0x6a574f68U, 0x0e3ca735U},
    {5552, 0x5d32532a996f4790ULL, 0x3cd9994bU, 0xab35b780U},
    {65537, 0x9aa1cda8727b92f5ULL, 0x3bdf0243U, 0xe0269980U},
};
} // namespace

class SharedChecksumTests : public QObject {
    Q_OBJECT
  private slots:
    void xxhashVectorsAndStreaming() {
        for (const auto &v : vectors) {
            for (size_t offset : {0U, 1U, 3U, 7U}) {
                const auto storage = bytes(v.size + 1, offset);
                const Byte *data = storage.data() + offset;
                QCOMPARE(xxh32(data, v.size, 0), v.hash32);
                QCOMPARE(xxh32(data, v.size, UINT32_MAX), v.seeded32);
                for (size_t chunk : {1U, 17U, 32U, 257U, 4096U}) {
                    CXxh64 state;
                    Xxh64_Init(&state);
                    Xxh64_Update(&state, nullptr, 0);
                    for (size_t pos = 0; pos < v.size;) {
                        const size_t size = std::min(chunk, v.size - pos);
                        Xxh64_Update(&state, data + pos, size);
                        // Digest must allow the caller to continue the same stream.
                        const auto first = Xxh64_Digest(&state);
                        QCOMPARE(Xxh64_Digest(&state), first);
                        pos += size;
                    }
                    QCOMPARE(Xxh64_Digest(&state), v.hash64);
                }
                CXxh64State blocks;
                Xxh64State_Init(&blocks);
                const size_t full = v.size & ~size_t(31);
                for (size_t pos = 0; pos < full; pos += 32)
                    Xxh64State_UpdateBlocks(&blocks, data + pos, data + pos + 32);
                QCOMPARE(Xxh64State_Digest(&blocks, data + full, v.size), v.hash64);
            }
        }
    }

    void adlerStreamingAndShortWrites() {
        // Independent Python zlib.adler32 results for the byte sequence above.
        const std::pair<size_t, UInt32> cases[] = {{0, 1},
                                                   {1, 0x000c000c},
                                                   {15, 0x312706d3},
                                                   {16, 0x38300709},
                                                   {31, 0xe04f0e8b},
                                                   {32, 0xef600f11},
                                                   {5550, 0xd286cbe8},
                                                   {5552, 0x6b2ccc6f},
                                                   {65537, 0xfd56877d}};
        for (const auto &v : cases) {
            const auto data = bytes(v.first);
            for (size_t chunk : {1U, 17U, 5550U, 65537U}) {
                CMyComPtr2_Create<ISequentialOutStream, NCompress::NZlib::COutStreamWithAdler>
                    stream;
                stream->Init();
                for (size_t pos = 0; pos < data.size();) {
                    const auto count = UInt32(std::min(chunk, data.size() - pos));
                    UInt32 written = 0;
                    QCOMPARE(stream.Interface()->Write(data.data() + pos, count, &written), S_OK);
                    QCOMPARE(written, count);
                    pos += written;
                    QCOMPARE(stream.Interface()->Write(nullptr, 0, &written), S_OK);
                }
                QCOMPARE(stream->GetAdler(), v.second);
                QCOMPARE(stream->GetSize(), UInt64(v.first));
            }
        }
        // A bounded output accepts only part of the requested write.
        std::array<Byte, 3> output{};
        CMyComPtr2_Create<ISequentialOutStream, CBufPtrSeqOutStream> limited;
        limited->Init(output.data(), output.size());
        CMyComPtr2_Create<ISequentialOutStream, NCompress::NZlib::COutStreamWithAdler> stream;
        stream->SetStream(limited);
        stream->Init();
        UInt32 written = 0;
        QCOMPARE(stream.Interface()->Write("abcdef", 6, &written), S_OK);
        QCOMPARE(written, UInt32(3));
        QCOMPARE(stream->GetAdler(), UInt32(0x024d0127)); // Adler-32 of abc only.
        QCOMPARE(stream->GetSize(), UInt64(3));
    }

    void optionalAdlerTrailer_data() {
        QTest::addColumn<QByteArray>("packed");
        QTest::addColumn<bool>("optional");
        QTest::addColumn<bool>("success");
        const QByteArray valid = QByteArray::fromHex("789c4b4c4a0600024d0127");
        for (bool optional : {false, true}) {
            const QByteArray prefix = optional ? "optional-" : "required-";
            QTest::newRow((prefix + "valid").constData()) << valid << optional << true;
            QByteArray corrupt = valid;
            corrupt[corrupt.size() - 1] = char(0);
            QTest::newRow((prefix + "corrupt").constData()) << corrupt << optional << false;
            QTest::newRow((prefix + "absent").constData())
                << valid.left(valid.size() - 4) << optional << optional;
            QTest::newRow((prefix + "partial").constData())
                << valid.left(valid.size() - 2) << optional << false;
        }
    }

    void optionalAdlerTrailer() {
        QFETCH(QByteArray, packed);
        QFETCH(bool, optional);
        QFETCH(bool, success);
        CMyComPtr2_Create<IInStream, CBufInStream> input;
        input->Init(reinterpret_cast<const Byte *>(packed.constData()), packed.size());
        CMyComPtr2_Create<ISequentialOutStream, CDynBufSeqOutStream> output;
        CMyComPtr2_Create<ICompressCoder, NCompress::NZlib::CDecoder> decoder;
        decoder->IsAdlerOptional = optional;
        QCOMPARE(decoder.Interface()->Code(input, output, nullptr, nullptr, nullptr),
                 success ? S_OK : S_FALSE);
        QCOMPARE(QByteArray(reinterpret_cast<const char *>(output->GetBuffer()), output->GetSize()),
                 QByteArray("abc"));
    }
};

QTEST_GUILESS_MAIN(SharedChecksumTests)
#include "shared_checksum_tests.moc"
