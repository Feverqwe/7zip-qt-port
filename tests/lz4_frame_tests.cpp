#include "7zip/Common/StreamObjects.h"
#include "archivebackend.h"
#include "archivecodeccatalog.h"
#include "fixtures/codec_runtime.h"
#include "fixtures/frame_io.h"
#include "lz4framestream.h"
#include <QFile>
#include <QProcess>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QtTest>

#include <QtEndian>
#include <lz4.h>
#include <lz4frame.h>

#ifdef SEVENZIP_USE_LZ4_MODULE
import sevenzip.dependencies.lz4;
#else
#include "dependencies/lz4.h"
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
QByteArray Frame(const QByteArray &bytes, LZ4F_preferences_t preferences = {}) {
    QByteArray result(int(LZ4F_compressFrameBound(size_t(bytes.size()), &preferences)), '\0');
    const size_t size = LZ4F_compressFrame(result.data(), size_t(result.size()), bytes.constData(),
                                           size_t(bytes.size()), &preferences);
    if (LZ4F_isError(size))
        return {};
    result.resize(qsizetype(size));
    return result;
}
QByteArray Skip(int id, const QByteArray &bytes) {
    QByteArray result(8, '\0');
    qToLittleEndian<quint32>(0x184d2a50 + id, result.data());
    qToLittleEndian<quint32>(quint32(bytes.size()), result.data() + 4);
    return result + bytes;
}
} // namespace

class Lz4FrameTests final : public QObject {
    Q_OBJECT
    QTemporaryDir runtime_;
    QString engine_, module_;
    std::shared_ptr<const ArchiveCodecCatalog> catalog_;

    void check(const QByteArray &bytes, const QByteArray &expected, Int32 result = 0) {
        QTemporaryDir files;
        const QString path = files.filePath(QStringLiteral("данные 空.bin"));
        QVERIFY(Write(path, bytes));
        CArchiveLink archive;
        QCOMPARE(
            catalog_->openArchive(archive, path, result == 0 ? QString() : QString("Lz4"), nullptr),
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
        const QDir prepared(QString::fromUtf8(SEVENZIP_LZ4_RUNTIME));
#ifdef Q_OS_WIN
        const QString engineName = "7z.exe", moduleName = "7z.dll", suffix = ".dll";
#else
        const QString engineName = "7z", moduleName = "7z.so", suffix = ".so";
#endif
        engine_ = runtime_.filePath(engineName);
        module_ = runtime_.filePath(moduleName);
        QVERIFY(CopyCodecRuntime(prepared, QDir(runtime_.path())));
        QVERIFY(QDir(runtime_.path()).mkdir("Formats"));
        QVERIFY(QFile::copy(QString::fromUtf8(SEVENZIP_LZ4_MODULE),
                            runtime_.filePath(QStringLiteral("Formats/Lz4 空") + suffix)));
        QVERIFY(QFile::copy(QString::fromUtf8(SEVENZIP_LZ4_LEGACY_MODULE),
                            runtime_.filePath(QStringLiteral("Formats/Legacy 空") + suffix)));
        ArchiveBackend backend(nullptr, engine_);
        QSignalSpy ready(&backend, &ArchiveBackend::codecCatalogReady);
        QVERIFY(backend.discoverCodecCatalog(module_));
        QTRY_COMPARE_WITH_TIMEOUT(ready.size(), 1, 15000);
        QVERIFY2(ready[0][0].toBool(), qPrintable(ready[0][1].toString()));
        catalog_ = backend.codecCatalog();
    }
    void frames_data() {
        QTest::addColumn<int>("block");
        QTest::addColumn<bool>("linked");
        QTest::addColumn<bool>("checksum");
        for (int block = LZ4F_max64KB; block <= LZ4F_max4MB; ++block)
            for (bool linked : {false, true})
                for (bool checksum : {false, true})
                    QTest::newRow(
                        qPrintable(QString("%1-%2-%3").arg(block).arg(linked).arg(checksum)))
                        << block << linked << checksum;
    }
    void frames() {
        QFETCH(int, block);
        QFETCH(bool, linked);
        QFETCH(bool, checksum);
        LZ4F_preferences_t prefs{};
        prefs.frameInfo.blockSizeID = LZ4F_blockSizeID_t(block);
        prefs.frameInfo.blockMode = linked ? LZ4F_blockLinked : LZ4F_blockIndependent;
        prefs.frameInfo.contentChecksumFlag =
            checksum ? LZ4F_contentChecksumEnabled : LZ4F_noContentChecksum;
        prefs.frameInfo.blockChecksumFlag =
            checksum ? LZ4F_blockChecksumEnabled : LZ4F_noBlockChecksum;
        const QByteArray payload = Payload();
        prefs.frameInfo.contentSize = checksum ? size_t(payload.size()) : 0;
        check(Frame(payload, prefs), payload);
    }
    void concatenationAndSkippable() {
        check(Frame({}), {});
        for (int id = 0; id < 16; ++id) {
            const auto skip = Skip(id, QByteArray(100003, 's'));
            check(skip + Frame("one") + skip + Frame("two") + skip, "onetwo");
        }
        check(Frame("one") + Frame({}) + Frame("two"), "onetwo");
    }
    void legacyCoexistence() {
        const auto payload = Payload();
        QByteArray block(LZ4_compressBound(int(payload.size())), '\0');
        const int size = LZ4_compress_default(payload.constData(), block.data(),
                                              int(payload.size()), int(block.size()));
        QVERIFY(size > 0);
        block.resize(size);
        QByteArray legacy(8, '\0');
        qToLittleEndian<quint32>(0x184c2102, legacy.data());
        qToLittleEndian<quint32>(quint32(size), legacy.data() + 4);
        legacy += block;
        QTemporaryDir files;
        const QString path = files.filePath(QStringLiteral("общий суффикс.lz4"));
        for (const auto &bytes : {legacy, Frame(payload)}) {
            QVERIFY(Write(path, bytes));
            CArchiveLink archive;
            QCOMPARE(catalog_->openArchive(archive, path, {}, nullptr), S_OK);
            CMyComPtr2<IArchiveExtractCallback, CExtract> callback;
            callback.Create_if_Empty();
            QCOMPARE(archive.GetArc()->Archive->Extract(nullptr, UInt32(-1), 0, callback), S_OK);
            QCOMPARE(callback->Result, Int32(NArchive::NExtract::NOperationResult::kOK));
            QCOMPARE(callback->Output->Bytes, payload);
        }
    }
    void corruption() {
        using namespace NArchive::NExtract::NOperationResult;
        LZ4F_preferences_t prefs{};
        prefs.frameInfo.contentChecksumFlag = LZ4F_contentChecksumEnabled;
        prefs.frameInfo.blockChecksumFlag = LZ4F_blockChecksumEnabled;
        const auto original = Frame(Payload(), prefs);
        check(original.left(original.size() - 1), {}, kUnexpectedEnd);
        check(original.left(5), {}, kUnexpectedEnd);
        auto broken = original;
        broken[6] ^= 1; // Header checksum.
        check(broken, {}, kDataError);
        broken = original;
        broken[20] ^= 1; // Block payload/checksum.
        check(broken, {}, kDataError);
        broken = original;
        broken.back() ^= 1; // Content checksum.
        check(broken, {}, kDataError);
        check(Frame("valid") + "tail", {}, kDataAfterEnd);
        check(Frame("valid") + original.left(original.size() - 3), {}, kUnexpectedEnd);
        check(Skip(0, "ignored").chopped(1), {}, kUnexpectedEnd);
        prefs.frameInfo.dictID = 123;
        // Dict-ID is only a lookup hint; it does not prove the block needs a dictionary.
        check(Frame("dictionary", prefs), "dictionary");
    }
    void cancellationAndOutputFailure() {
        QTemporaryDir files;
        const QString path = files.filePath("large.lz4");
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
        const auto result = sevenzip::lz4::decode(source, {.offset = 0, .size = 128}, 17);
        QCOMPARE(result.status, sevenzip::lz4::Status::io_error);
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
        QCOMPARE(SevenzipDecodeLz4Frame(input, 0, frame.size(), output, nullptr, consumed, produced,
                                        result),
                 S_OK);
        QCOMPARE(result, Int32(NArchive::NExtract::NOperationResult::kOK));
        QCOMPARE(consumed, UInt64(frame.size()));
        QCOMPARE(output->Bytes, Payload());
        for (HRESULT failure : {E_FAIL, E_ABORT}) {
            input->readResult = failure;
            QCOMPARE(SevenzipDecodeLz4Frame(input, 0, frame.size(), output, nullptr, consumed,
                                            produced, result),
                     failure);
            input->readResult = S_OK;
            input->seekResult = failure;
            QCOMPARE(SevenzipDecodeLz4Frame(input, 0, frame.size(), output, nullptr, consumed,
                                            produced, result),
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
        const QString path = files.filePath(QStringLiteral("архив 空.tar.lz4"));
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
        process.start(engine_, {"x", "-so", "-tLz4", path});
        QVERIFY(process.waitForFinished(10000));
        QCOMPARE(process.exitCode(), 0);
        QCOMPARE(process.readAllStandardOutput(), Read(tar));
        QVERIFY(!catalog_->canUpdate("Lz4"));
    }
};
QTEST_GUILESS_MAIN(Lz4FrameTests)
#include "lz4_frame_tests.moc"
