#include "7zip/Common/StreamObjects.h"
#include "archivebackend.h"
#include "archivecodeccatalog.h"
#include "fixtures/codec_runtime.h"
#include "fixtures/frame_io.h"
#include "lzipstream.h"
#include <QFile>
#include <QProcess>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QtTest>

#include <QtEndian>
#include <cstdint>
#include <lzlib.h>
#include <memory>

#ifdef SEVENZIP_USE_LZLIB_MODULE
import sevenzip.dependencies.lzlib;
#else
#include "dependencies/lzlib.h"
#endif

namespace {
class Input final : public IInStream, public CMyUnknownImp {
    Z7_COM_UNKNOWN_IMP_2(ISequentialInStream, IInStream)
    QByteArray bytes_;
    UInt64 position_ = 0;

  public:
    HRESULT readResult = S_OK, seekResult = S_OK;
    explicit Input(QByteArray bytes) : bytes_(std::move(bytes)) {}
    Z7_COM7F_IMF(Read(void *data, UInt32 size, UInt32 *processed)) override {
        if (processed)
            *processed = 0;
        if (readResult != S_OK)
            return readResult;
        size = UInt32(qMin<UInt64>(qMin<UInt32>(size, 13), UInt64(bytes_.size()) - position_));
        memcpy(data, bytes_.constData() + position_, size);
        position_ += size;
        if (processed)
            *processed = size;
        return S_OK;
    }
    Z7_COM7F_IMF(Seek(Int64 offset, UInt32 origin, UInt64 *position)) override {
        if (seekResult != S_OK)
            return seekResult;
        if (origin != STREAM_SEEK_SET || offset < 0 || UInt64(offset) > UInt64(bytes_.size()))
            return E_INVALIDARG;
        position_ = UInt64(offset);
        if (position)
            *position = position_;
        return S_OK;
    }
};
QByteArray Read(const QString &path) {
    QFile file(path);
    return file.open(QIODevice::ReadOnly) ? file.readAll() : QByteArray();
}
bool Write(const QString &path, const QByteArray &bytes) {
    QFile file(path);
    return file.open(QIODevice::WriteOnly) && file.write(bytes) == bytes.size();
}
class COutput final : public ISequentialOutStream, public CMyUnknownImp {
    Z7_COM_UNKNOWN_IMP_1(ISequentialOutStream)
  public:
    QByteArray Bytes;
    bool Fail = false;
    Z7_COM7F_IMF(Write(const void *data, UInt32 size, UInt32 *processed)) override {
        if (processed)
            *processed = 0;
        if (Fail)
            return E_FAIL;
        Bytes.append(static_cast<const char *>(data), size);
        if (processed)
            *processed = size;
        return S_OK;
    }
};
class CExtract final : public IArchiveExtractCallback, public CMyUnknownImp {
    Z7_COM_UNKNOWN_IMP_1(IArchiveExtractCallback)
  public:
    CMyComPtr2<ISequentialOutStream, COutput> Output;
    Int32 Result = -1;
    bool Cancel = false;
    CExtract() { Output.Create_if_Empty(); }
    Z7_COM7F_IMF(SetTotal(UInt64)) override { return S_OK; }
    Z7_COM7F_IMF(SetCompleted(const UInt64 *)) override {
        return Cancel && !Output->Bytes.isEmpty() ? E_ABORT : S_OK;
    }
    Z7_COM7F_IMF(GetStream(UInt32, ISequentialOutStream **stream, Int32 mode)) override {
        *stream = nullptr;
        if (mode == NArchive::NExtract::NAskMode::kExtract) {
            *stream = Output.Interface();
            (*stream)->AddRef();
        }
        return S_OK;
    }
    Z7_COM7F_IMF(PrepareOperation(Int32)) override { return S_OK; }
    Z7_COM7F_IMF(SetOperationResult(Int32 result)) override {
        Result = result;
        return S_OK;
    }
};
} // namespace

namespace {
QByteArray Payload() {
    QByteArray bytes(400000, 'a');
    quint32 state = 0x12345678;
    for (int i = 0; i < bytes.size(); ++i) {
        state ^= state << 13;
        state ^= state >> 17;
        state ^= state << 5;
        if ((i / 65536) % 2)
            bytes[i] = char(state);
    }
    return bytes;
}
// Fixture compression loop adapted from lzlib ffexample.c::ffcompress.
// Copyright (C) 2010-2026 Antonio Diaz Diaz.
// Unlimited permission to copy, distribute, and modify this example.
QByteArray Frame(const QByteArray &bytes, int dictionary = 1 << 20) {
    std::unique_ptr<LZ_Encoder, decltype(&LZ_compress_close)> encoder(
        LZ_compress_open(dictionary, 16, INT64_MAX), LZ_compress_close);
    if (!encoder || LZ_compress_errno(encoder.get()) != LZ_ok)
        return {};
    QByteArray result;
    int position = 0;
    uint8_t buffer[16384];
    for (;;) {
        const int count = qMin(int(bytes.size()) - position, LZ_compress_write_size(encoder.get()));
        if (LZ_compress_write(encoder.get(),
                              reinterpret_cast<const uint8_t *>(bytes.constData()) + position,
                              count) != count)
            return {};
        position += count;
        if (position == bytes.size())
            LZ_compress_finish(encoder.get());
        const int size = LZ_compress_read(encoder.get(), buffer, sizeof(buffer));
        if (size < 0)
            return {};
        result.append(reinterpret_cast<const char *>(buffer), size);
        if (LZ_compress_finished(encoder.get()) == 1)
            return result;
    }
}
} // namespace

class LzipTests final : public QObject {
    Q_OBJECT
    QTemporaryDir runtime_;
    QString engine_, module_;
    std::shared_ptr<const ArchiveCodecCatalog> catalog_;

    void check(const QByteArray &bytes, const QByteArray &expected, Int32 result = 0) {
        QTemporaryDir files;
        const QString path = files.filePath(QStringLiteral("данные 空.bin"));
        QVERIFY(Write(path, bytes));
        CArchiveLink archive;
        QCOMPARE(catalog_->openArchive(archive, path, result == 0 ? QString() : QString("Lzip"),
                                       nullptr),
                 S_OK);
        CMyComPtr2<IArchiveExtractCallback, CExtract> callback;
        callback.Create_if_Empty();
        QCOMPARE(archive.GetArc()->Archive->Extract(nullptr, UInt32(-1), 0, callback), S_OK);
        QCOMPARE(callback->Result, result);
        if (result == NArchive::NExtract::NOperationResult::kOK) {
            QCOMPARE(callback->Output->Bytes, expected);
            callback->Output->Bytes.clear();
            QCOMPARE(archive.GetArc()->Archive->Extract(nullptr, UInt32(-1), 0, callback), S_OK);
            QCOMPARE(callback->Output->Bytes, expected);
            callback->Output->Bytes.clear();
            QCOMPARE(archive.GetArc()->Archive->Extract(nullptr, UInt32(-1), 1, callback), S_OK);
            QCOMPARE(callback->Result, result);
            QVERIFY(callback->Output->Bytes.isEmpty());
        } else {
            NWindows::NCOM::CPropVariant size;
            QCOMPARE(archive.GetArc()->Archive->GetProperty(0, kpidSize, &size), S_OK);
            QCOMPARE(size.vt, VARTYPE(VT_EMPTY));
        }
        QCOMPARE(Read(path), bytes);
    }

  private slots:
    void initTestCase() {
        QVERIFY(runtime_.isValid());
        const QDir prepared(QString::fromUtf8(SEVENZIP_LZIP_RUNTIME));
#ifdef Q_OS_WIN
        const QString engineName = "7z.exe", moduleName = "7z.dll", suffix = ".dll";
#else
        const QString engineName = "7z", moduleName = "7z.so", suffix = ".so";
#endif
        engine_ = runtime_.filePath(engineName);
        module_ = runtime_.filePath(moduleName);
        QVERIFY(CopyCodecRuntime(prepared, QDir(runtime_.path())));
        QVERIFY(QDir(runtime_.path()).mkdir("Formats"));
        QVERIFY(QFile::copy(QString::fromUtf8(SEVENZIP_LZIP_MODULE),
                            runtime_.filePath(QStringLiteral("Formats/Lzip 空") + suffix)));
        ArchiveBackend backend(nullptr, engine_);
        QSignalSpy ready(&backend, &ArchiveBackend::codecCatalogReady);
        QVERIFY(backend.discoverCodecCatalog(module_));
        QTRY_COMPARE_WITH_TIMEOUT(ready.size(), 1, 15000);
        QVERIFY2(ready[0][0].toBool(), qPrintable(ready[0][1].toString()));
        catalog_ = backend.codecCatalog();
    }
    void streams() {
        check(Frame({}), {});
        for (int dictionary : {4096, 65536, 1 << 20, 3 << 20})
            check(Frame(Payload(), dictionary), Payload());
        check(Frame("one") + Frame({}) + Frame("two"), "onetwo");
        const QString fixtures = QString::fromUtf8(SEVENZIP_LZIP_FIXTURES);
        check(Read(fixtures + "/fox.lz"), "The quick brown fox jumps over the lazy dog.\n");
    }
    void corruption() {
        using namespace NArchive::NExtract::NOperationResult;
        const auto original = Frame(Payload());
        check(original.chopped(1), {}, kUnexpectedEnd);
        check(original.left(4), {}, kUnexpectedEnd);
        check(original.left(original.size() / 2), {}, kUnexpectedEnd);
        for (int index : {4, 5, 12, int(original.size()) - 20, int(original.size()) - 16,
                          int(original.size()) - 8}) {
            auto broken = original;
            if (index == 5)
                broken[index] = 0; // Invalid dictionary code.
            else
                broken[index] ^= 1;
            check(broken, {}, kDataError);
        }
        check(Frame("valid") + "tail", {}, kDataAfterEnd);
        check(Frame("valid") + "LZI", {}, kUnexpectedEnd);
        check(Frame("valid") + original.chopped(3), {}, kUnexpectedEnd);
    }
    void cancellationAndOutputFailure() {
        QTemporaryDir files;
        const QString path = files.filePath("large.lz");
        QVERIFY(Write(path, Frame(Payload())));
        CArchiveLink archive;
        QCOMPARE(catalog_->openArchive(archive, path, {}, nullptr), S_OK);
        CMyComPtr2<IArchiveExtractCallback, CExtract> callback;
        callback.Create_if_Empty();
        callback->Cancel = true;
        QCOMPARE(archive.GetArc()->Archive->Extract(nullptr, UInt32(-1), 0, callback), E_ABORT);
        QVERIFY(!callback->Output->Bytes.isEmpty());
        QVERIFY(callback->Output->Bytes.size() < Payload().size());
        QCOMPARE(callback->Result, -1);
        callback->Cancel = false;
        callback->Output->Fail = true;
        QCOMPARE(archive.GetArc()->Archive->Extract(nullptr, UInt32(-1), 0, callback), E_FAIL);
        callback->Output->Fail = false;
        callback->Output->Bytes.clear();
        QCOMPARE(archive.GetArc()->Archive->Extract(nullptr, UInt32(-1), 0, callback), S_OK);
        QCOMPARE(callback->Output->Bytes, Payload());
    }
    void rejectsOversizedReadBeforeDecoding() {
        OversizedFrameRead source;
        const auto result = sevenzip::lzlib::decode(source, {.offset = 0, .size = 128}, 17);
        QCOMPARE(result.status, sevenzip::lzlib::Status::io_error);
        QCOMPARE(result.produced, std::uint64_t(17));
        QVERIFY(!source.wrote);
    }
    void boundedSourceAndFailures() {
        const QByteArray frame = Frame(Payload());
        CMyComPtr2<IInStream, Input> input;
        input.SetFromCls(new Input(frame + "following container"));
        CMyComPtr2<ISequentialOutStream, COutput> output;
        output.Create_if_Empty();
        UInt64 consumed = 0, produced = 0;
        Int32 result = -1;
        QCOMPARE(
            SevenzipDecodeLzip(input, 0, frame.size(), output, nullptr, consumed, produced, result),
            S_OK);
        QCOMPARE(result, Int32(NArchive::NExtract::NOperationResult::kOK));
        QCOMPARE(consumed, UInt64(frame.size()));
        QCOMPARE(output->Bytes, Payload());
        for (HRESULT failure : {E_FAIL, E_ABORT}) {
            input->readResult = failure;
            QCOMPARE(SevenzipDecodeLzip(input, 0, frame.size(), output, nullptr, consumed, produced,
                                        result),
                     failure);
            input->readResult = S_OK;
            input->seekResult = failure;
            QCOMPARE(SevenzipDecodeLzip(input, 0, frame.size(), output, nullptr, consumed, produced,
                                        result),
                     failure);
            input->seekResult = S_OK;
        }
    }
    void backendAndNestedTar() {
        QTemporaryDir files;
        const QString name = QStringLiteral("данные 空.txt");
        QVERIFY(Write(files.filePath(name), "nested TAR payload"));
        const QString tar = files.filePath("inner.tar");
        QProcess process;
        process.setWorkingDirectory(files.path());
        process.start(engine_, {"a", "-ttar", tar, name});
        QVERIFY(process.waitForFinished(10000));
        QCOMPARE(process.exitCode(), 0);
        const QString path = files.filePath(QStringLiteral("архив 空.tar.lz"));
        const QByteArray original = Frame(Read(tar));
        QVERIFY(Write(path, original));
        ArchiveBackend backend(nullptr, engine_);
        QSignalSpy ready(&backend, &ArchiveBackend::codecCatalogReady);
        QVERIFY(backend.discoverCodecCatalog(module_));
        QTRY_COMPARE_WITH_TIMEOUT(ready.size(), 1, 15000);
        QVERIFY2(ready[0][0].toBool(), qPrintable(ready[0][1].toString()));
        QSignalSpy finished(&backend, &ArchiveBackend::finished);
        QList<ArchiveEntry> entries;
        connect(&backend, &ArchiveBackend::entriesReady, this,
                [&](const QList<ArchiveEntry> &value) { entries = value; });
        backend.list(path);
        QTRY_COMPARE(finished.size(), 1);
        QVERIFY2(finished[0][0].toBool(), qPrintable(finished[0][1].toString()));
        QCOMPARE(entries.size(), 1);
        QCOMPARE(entries.front().path, QStringLiteral("архив 空.tar"));
        finished.clear();
        const QString output = files.filePath("output");
        backend.extract(path, output);
        QTRY_COMPARE(finished.size(), 1);
        QVERIFY2(finished[0][0].toBool(), qPrintable(finished[0][1].toString()));
        const QString inner = QDir(output).filePath(QStringLiteral("архив 空.tar"));
        QCOMPARE(Read(inner), Read(tar));
        finished.clear();
        backend.list(inner);
        QTRY_COMPARE(finished.size(), 1);
        QVERIFY2(finished[0][0].toBool(), qPrintable(finished[0][1].toString()));
        QCOMPARE(entries.size(), 1);
        QCOMPARE(entries.front().path, name);
        finished.clear();
        backend.extract(inner, files.filePath("unpacked"));
        QTRY_COMPARE(finished.size(), 1);
        QVERIFY2(finished[0][0].toBool(), qPrintable(finished[0][1].toString()));
        QCOMPARE(Read(files.filePath("unpacked/" + name)), QByteArray("nested TAR payload"));
        QCOMPARE(Read(path), original);
        process.start(engine_, {"x", "-so", "-tLzip", path});
        QVERIFY(process.waitForFinished(10000));
        QCOMPARE(process.exitCode(), 0);
        QCOMPARE(process.readAllStandardOutput(), Read(tar));
        QVERIFY(!catalog_->canUpdate("Lzip"));
        const auto truncated = original.chopped(1);
        QVERIFY(Write(path, truncated));
        finished.clear();
        backend.extract(path, files.filePath("partial"));
        QTRY_COMPARE(finished.size(), 1);
        QVERIFY(!finished[0][0].toBool());
        QCOMPARE(Read(path), truncated);
    }
};
QTEST_GUILESS_MAIN(LzipTests)
#include "lzip_tests.moc"
