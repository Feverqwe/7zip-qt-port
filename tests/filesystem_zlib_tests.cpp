// clang-format off
#include "Common/MyInitGuid.h"
// clang-format on
#include "Common/MyCom.h"
#include "erofsreader.h"
#include "squashfszlib.h"
#include <QtTest>
#include <algorithm>
#include <cerrno>

namespace {
class Input final : public ISequentialInStream, public CMyUnknownImp {
    Z7_COM_UNKNOWN_IMP_1(ISequentialInStream)
  public:
    QByteArray bytes;
    UInt32 position = 0, chunk = UINT32_MAX, failAt = UINT32_MAX;
    Z7_COM7F_IMF(Read(void *data, UInt32 size, UInt32 *processed)) override {
        *processed = 0;
        if (failAt >= position && failAt - position < size)
            return E_ABORT;
        size = std::min({size, chunk, UInt32(bytes.size()) - position});
        if (size)
            memcpy(data, bytes.constData() + position, size);
        position += size;
        *processed = size;
        return S_OK;
    }
};
class Output final : public ISequentialOutStream, public CMyUnknownImp {
    Z7_COM_UNKNOWN_IMP_1(ISequentialOutStream)
  public:
    QByteArray bytes;
    bool fail = false;
    Z7_COM7F_IMF(Write(const void *data, UInt32 size, UInt32 *processed)) override {
        *processed = 0;
        if (fail)
            return E_FAIL;
        // Exercise short successful writes at both the window and final flush.
        size = std::min<UInt32>(size, 101);
        bytes.append(static_cast<const char *>(data), size);
        *processed = size;
        return S_OK;
    }
};
QByteArray packed(const QByteArray &bytes) { return qCompress(bytes, 9).sliced(4); }
} // namespace

class FilesystemZlibTests final : public QObject {
    Q_OBJECT
  private slots:
    void squashfsVerifiedFirstStream() {
        const auto expected = QByteArray("abc\0", 4).repeated(20000);
        const auto compressed = packed(expected);
        for (const auto &tail : {QByteArray(), QByteArray("tail"), packed("second")}) {
            for (UInt32 chunk : {1U, 7U, UINT32_MAX}) {
                CMyComPtr2<ISequentialInStream, Input> input;
                CMyComPtr2<ISequentialOutStream, Output> output;
                input.Create_if_Empty();
                output.Create_if_Empty();
                input->bytes = compressed + tail;
                input->chunk = chunk;
                QCOMPARE(SquashfsDecodeZlib(input, output, expected.size()), S_OK);
                QCOMPARE(output->bytes, expected);
            }
        }
    }
    void squashfsRejectsInvalidBlocks() {
        const auto expected = QByteArray("bounded data").repeated(100);
        const auto compressed = packed(expected);
        auto badHeader = compressed, badChecksum = compressed;
        badHeader[0] = 0;
        badChecksum.back() ^= 1;
        const QList<QByteArray> invalid{QByteArray(),
                                        compressed.first(1),
                                        badHeader,
                                        compressed.first(compressed.size() - 4),
                                        compressed.first(compressed.size() - 1),
                                        badChecksum,
                                        QByteArray::fromHex("789c07")};
        for (const auto &bytes : invalid) {
            CMyComPtr2<ISequentialInStream, Input> input;
            CMyComPtr2<ISequentialOutStream, Output> output;
            input.Create_if_Empty();
            output.Create_if_Empty();
            input->bytes = bytes;
            QCOMPARE(SquashfsDecodeZlib(input, output, expected.size()), S_FALSE);
        }
        CMyComPtr2<ISequentialInStream, Input> input;
        CMyComPtr2<ISequentialOutStream, Output> output;
        input.Create_if_Empty();
        output.Create_if_Empty();
        input->bytes = compressed;
        QCOMPARE(SquashfsDecodeZlib(input, output, expected.size() - 1), S_FALSE);
        QVERIFY(output->bytes.size() <= expected.size() - 1);
    }
    void squashfsErrorPriority() {
        const auto compressed = packed("verified output");
        for (bool validHeader : {false, true}) {
            CMyComPtr2<ISequentialInStream, Input> input;
            CMyComPtr2<ISequentialOutStream, Output> output;
            input.Create_if_Empty();
            output.Create_if_Empty();
            input->bytes = compressed;
            if (!validHeader)
                input->bytes[0] = 0;
            input->failAt = 2;
            QCOMPARE(SquashfsDecodeZlib(input, output, 100), validHeader ? E_ABORT : S_FALSE);
            QVERIFY(output->bytes.isEmpty());
        }
        // Output failure precedes a trailer error, after successful DEFLATE.
        CMyComPtr2<ISequentialInStream, Input> input;
        CMyComPtr2<ISequentialOutStream, Output> output;
        input.Create_if_Empty();
        output.Create_if_Empty();
        input->bytes = compressed;
        input->bytes.back() ^= 1;
        output->fail = true;
        QCOMPARE(SquashfsDecodeZlib(input, output, 100), E_FAIL);
        input->position = 0;
        // A body error precedes failure of the deferred output flush.
        input->bytes = compressed.first(compressed.size() - 5);
        QCOMPARE(SquashfsDecodeZlib(input, output, 100), S_FALSE);
    }
    void erofsFullAndPrefix() {
        const auto expected = QByteArray("abc").repeated(30000);
        const auto wrapped = packed(expected);
        const auto raw = wrapped.sliced(2, wrapped.size() - 6);
        for (qsizetype length : {1, 32768, 89999, 90000}) {
            for (const auto &input : {raw, raw + "padding"}) {
                QByteArray output(length + 16, '?');
                QCOMPARE(erofs_decode_native(2, 0, input.constData(), input.size(), output.data(),
                                             length, 1),
                         0);
                QCOMPARE(output.first(length), expected.first(length));
                QCOMPARE(output.sliced(length), QByteArray(16, '?'));
            }
        }
        for (qsizetype length : {89999, 90000, 90001}) {
            QByteArray output(length + 16, '?');
            QCOMPARE(
                erofs_decode_native(2, 0, raw.constData(), raw.size(), output.data(), length, 0),
                length == expected.size() ? 0 : -EILSEQ);
            QCOMPARE(output.sliced(length), QByteArray(16, '?'));
            if (length == expected.size())
                QCOMPARE(output.first(length), expected);
        }
        const auto unfinished = raw.first(raw.size() - 1);
        QByteArray output(expected.size(), '?');
        QCOMPARE(erofs_decode_native(2, 0, unfinished.constData(), unfinished.size(), output.data(),
                                     output.size(), 0),
                 -EILSEQ);
        QCOMPARE(erofs_decode_native(2, 0, unfinished.constData(), unfinished.size(), output.data(),
                                     17, 1),
                 0);
        const auto invalid = QByteArray::fromHex("07");
        QCOMPARE(
            erofs_decode_native(2, 0, invalid.constData(), invalid.size(), output.data(), 17, 1),
            -EILSEQ);
    }
    void validCompressionVariants() {
        QByteArray varied;
        for (int i = 0; i < 70000; ++i)
            varied.append(char((i * 31 + i / 251) & 255));
        for (const auto &expected : {QByteArray("a"), QByteArray("abc").repeated(11000), varied}) {
            for (int level : {0, 1, 9}) {
                const auto wrapped = qCompress(expected, level).sliced(4);
                const auto raw = wrapped.sliced(2, wrapped.size() - 6);
                CMyComPtr2<ISequentialInStream, Input> input;
                CMyComPtr2<ISequentialOutStream, Output> output;
                input.Create_if_Empty();
                output.Create_if_Empty();
                input->bytes = wrapped;
                QCOMPARE(SquashfsDecodeZlib(input, output, expected.size()), S_OK);
                QCOMPARE(output->bytes, expected);
                for (bool partial : {false, true}) {
                    const auto length =
                        partial ? std::max<qsizetype>(1, expected.size() / 2) : expected.size();
                    for (const auto &bytes : {raw, raw + "padding"}) {
                        QByteArray decoded(length + 16, '?');
                        QCOMPARE(erofs_decode_native(2, 0, bytes.constData(), bytes.size(),
                                                     decoded.data(), length, partial),
                                 0);
                        QCOMPARE(decoded.first(length), expected.first(length));
                        QCOMPARE(decoded.sliced(length), QByteArray(16, '?'));
                    }
                }
            }
        }
    }
};
QTEST_GUILESS_MAIN(FilesystemZlibTests)
#include "filesystem_zlib_tests.moc"
