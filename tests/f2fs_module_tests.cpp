// SPDX-License-Identifier: MIT
// clang-format off
#include "Common/MyInitGuid.h"
// clang-format on
#include "7zip/Archive/IArchive.h"
#include "Common/MyCom.h"
#include "Windows/PropVariant.h"
#include <QCryptographicHash>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLibrary>
#include <QtEndian>
#include <QtTest>
#include <algorithm>

namespace {
QByteArray fixture(const char *variant) {
    QFile f(QString::fromUtf8(SEVENZIP_F2FS_FIXTURES) + '/' + variant + ".f2fs.zlib");
    if (!f.open(QIODevice::ReadOnly))
        return {};
    QByteArray compressed(4, '\0');
    qToBigEndian(quint32(128 * 1024 * 1024), compressed.data());
    compressed += f.readAll();
    return qUncompress(compressed);
}
int inodeOffset(const QByteArray &bytes, UInt32 ino) {
    for (int offset = 0; offset + 4096 <= bytes.size(); offset += 4096) {
        const char *footer = bytes.constData() + offset + 4072;
        if (qFromLittleEndian<quint32>(footer) == ino &&
            qFromLittleEndian<quint32>(footer + 4) == ino)
            return offset;
    }
    return -1;
}
class Input final : public IInStream, public CMyUnknownImp {
  public:
    Z7_COM_UNKNOWN_IMP_2(ISequentialInStream, IInStream)
    UInt64 position_ = 0;

  public:
    QByteArray bytes;
    HRESULT failure = S_OK;
    explicit Input(QByteArray data) : bytes(std::move(data)) {}
    Z7_COM7F_IMF(Read(void *data, UInt32 size, UInt32 *processed)) override {
        if (processed)
            *processed = 0;
        if (failure != S_OK)
            return failure;
        const UInt32 count = UInt32(std::min<UInt64>(
            size, position_ < UInt64(bytes.size()) ? bytes.size() - position_ : 0));
        if (count)
            memcpy(data, bytes.constData() + position_, count);
        position_ += count;
        if (processed)
            *processed = count;
        return S_OK;
    }
    Z7_COM7F_IMF(Seek(Int64 offset, UInt32 origin, UInt64 *position)) override {
        if (origin > STREAM_SEEK_END)
            return E_INVALIDARG;
        const Int64 base = origin == STREAM_SEEK_SET   ? 0
                           : origin == STREAM_SEEK_CUR ? Int64(position_)
                                                       : bytes.size();
        if (offset < -base || (offset > 0 && base > INT64_MAX - offset))
            return E_INVALIDARG;
        position_ = UInt64(base + offset);
        if (position)
            *position = position_;
        return S_OK;
    }
};
class Output final : public ISequentialOutStream, public CMyUnknownImp {
    Z7_COM_UNKNOWN_IMP_1(ISequentialOutStream)
  public:
    QByteArray bytes;
    Z7_COM7F_IMF(Write(const void *data, UInt32 size, UInt32 *processed)) override {
        bytes.append(static_cast<const char *>(data), size);
        if (processed)
            *processed = size;
        return S_OK;
    }
};
class Extract final : public IArchiveExtractCallback, public CMyUnknownImp {
    Z7_COM_UNKNOWN_IMP_2(IArchiveExtractCallback, IProgress)
  public:
    CMyComPtr2<ISequentialOutStream, Output> output;
    bool cancel = false;
    Int32 result = -1;
    UInt64 total = 0, completed = 0;
    Extract() { output.Create_if_Empty(); }
    Z7_COM7F_IMF(SetTotal(UInt64 value)) override {
        total = value;
        return S_OK;
    }
    Z7_COM7F_IMF(SetCompleted(const UInt64 *value)) override {
        if (value)
            completed = *value;
        return cancel && completed ? E_ABORT : S_OK;
    }
    Z7_COM7F_IMF(GetStream(UInt32, ISequentialOutStream **stream, Int32 mode)) override {
        *stream = nullptr;
        if (mode == NArchive::NExtract::NAskMode::kExtract) {
            *stream = output.Interface();
            (*stream)->AddRef();
        }
        return S_OK;
    }
    Z7_COM7F_IMF(PrepareOperation(Int32)) override { return S_OK; }
    Z7_COM7F_IMF(SetOperationResult(Int32 value)) override {
        result = value;
        return S_OK;
    }
};
class Opening final : public IArchiveOpenCallback, public CMyUnknownImp {
    Z7_COM_UNKNOWN_IMP_1(IArchiveOpenCallback)
  public:
    Z7_COM7F_IMF(SetTotal(const UInt64 *, const UInt64 *)) override { return S_OK; }
    Z7_COM7F_IMF(SetCompleted(const UInt64 *, const UInt64 *)) override { return E_ABORT; }
};
} // namespace
class F2fsModuleTests final : public QObject {
    Q_OBJECT
    QLibrary module_{QString::fromUtf8(SEVENZIP_F2FS_MODULE)};
    GUID clsid_{};
    Func_CreateObject create_ = nullptr;
    CMyComPtr<IInArchive> archive() {
        CMyComPtr<IInArchive> result;
        if (create_)
            create_(&clsid_, &IID_IInArchive, reinterpret_cast<void **>(&result));
        return result;
    }
    UInt32 find(IInArchive *archive, const QString &path) {
        UInt32 count = 0;
        if (archive->GetNumberOfItems(&count) != S_OK)
            return UINT32_MAX;
        for (UInt32 i = 0; i < count; ++i) {
            NWindows::NCOM::CPropVariant value;
            if (archive->GetProperty(i, kpidPath, &value) == S_OK && value.vt == VT_BSTR &&
                QString::fromWCharArray(value.bstrVal) == path)
                return i;
        }
        return UINT32_MAX;
    }
  private slots:
    void initTestCase() {
        QVERIFY2(module_.load(), qPrintable(module_.errorString()));
        auto property =
            reinterpret_cast<Func_GetHandlerProperty2>(module_.resolve("GetHandlerProperty2"));
        create_ = reinterpret_cast<Func_CreateObject>(module_.resolve("CreateObject"));
        QVERIFY(property && create_);
        NWindows::NCOM::CPropVariant value;
        QCOMPARE(property(0, NArchive::NHandlerPropID::kClassID, &value), S_OK);
        QCOMPARE(value.vt, VT_BSTR);
        memcpy(&clsid_, value.bstrVal, sizeof(clsid_));
    }
    void readTree_data() {
        QTest::addColumn<QByteArray>("variant");
        QTest::newRow("basic") << QByteArray("basic");
        QTest::newRow("extra-attributes-checksums") << QByteArray("modern");
    }
    void readTree() {
        QFETCH(QByteArray, variant);
        const QByteArray original = fixture(variant.constData());
        QVERIFY(!original.isEmpty());
        CMyComPtr<IInStream> input = new Input(original);
        auto reader = archive();
        QCOMPARE(reader->Open(input, nullptr, nullptr), S_OK);
        const UInt32 large = find(reader, "large.bin");
        const UInt32 tiny = find(reader, QString::fromUtf8("папка 空/tiny.txt"));
        QVERIFY(large != UINT32_MAX && tiny != UINT32_MAX);
        QVERIFY(find(reader, "empty") != UINT32_MAX);
        NWindows::NCOM::CPropVariant creation;
        QCOMPARE(reader->GetProperty(large, kpidCTime, &creation), S_OK);
        QCOMPARE(creation.vt, VT_EMPTY); // No on-disk birth time in these fixtures.
        const UInt32 link = find(reader, "link");
        QVERIFY(link != UINT32_MAX);
        NWindows::NCOM::CPropVariant target;
        QCOMPARE(reader->GetProperty(link, kpidSymLink, &target), S_OK);
        QCOMPARE(target.vt, VT_BSTR);
        QCOMPARE(QString::fromWCharArray(target.bstrVal), QString::fromUtf8("папка 空/tiny.txt"));
        CMyComPtr2<IArchiveExtractCallback, Extract> callback;
        callback.Create_if_Empty();
        QCOMPARE(reader->Extract(&large, 1, 0, callback), S_OK);
        QCOMPARE(callback->result, Int32(NArchive::NExtract::NOperationResult::kOK));
        QCOMPARE(callback->completed, callback->total);
        QFile manifest(QString::fromUtf8(SEVENZIP_F2FS_FIXTURES) + "/manifest.json");
        QVERIFY(manifest.open(QIODevice::ReadOnly));
        QCOMPARE(
            QCryptographicHash::hash(callback->output->bytes, QCryptographicHash::Sha256).toHex(),
            QJsonDocument::fromJson(manifest.readAll())
                .object()["large_sha256"]
                .toString()
                .toLatin1());
        CMyComPtr<IInArchiveGetStream> streams;
        QCOMPARE(
            reader->QueryInterface(IID_IInArchiveGetStream, reinterpret_cast<void **>(&streams)),
            S_OK);
        CMyComPtr<ISequentialInStream> first, second;
        QCOMPARE(streams->GetStream(tiny, &first), S_OK);
        QCOMPARE(streams->GetStream(large, &second), S_OK);
        QCOMPARE(reader->Close(), S_OK);
        char bytes[32]{};
        UInt32 done = 0;
        QCOMPARE(first->Read(bytes, sizeof(bytes), &done), S_OK);
        QCOMPARE(QByteArray(bytes, done), QByteArray("inline F2FS\n"));
        QCOMPARE(second->Read(bytes, sizeof(bytes), &done), S_OK);
        QCOMPARE(done, UInt32(sizeof(bytes)));
        for (int i = 0; i < int(sizeof(bytes)); ++i)
            QCOMPARE(static_cast<unsigned char>(bytes[i]), i);
        QCOMPARE(static_cast<Input *>(input.Interface())->bytes, original);
    }
    void cancellationAndFailures() {
        auto reader = archive();
        auto *source = new Input(fixture("basic"));
        CMyComPtr<IInStream> input = source;
        CMyComPtr<IArchiveOpenCallback> opening = new Opening;
        QCOMPARE(reader->Open(input, nullptr, opening), E_ABORT);
        QCOMPARE(reader->Open(input, nullptr, nullptr), S_OK);
        const UInt32 large = find(reader, "large.bin");
        QVERIFY(large != UINT32_MAX);
        CMyComPtr2<IArchiveExtractCallback, Extract> callback;
        callback.Create_if_Empty();
        callback->cancel = true;
        QCOMPARE(reader->Extract(&large, 1, 0, callback), E_ABORT);
        QVERIFY(!callback->output->bytes.isEmpty());
        QVERIFY(callback->completed < callback->total);
        callback->cancel = false;
        source->failure = E_FAIL;
        QCOMPARE(reader->Extract(&large, 1, 0, callback), E_FAIL);
        source->failure = S_OK;
        source->bytes.truncate(12 * 1024 * 1024);
        QCOMPARE(reader->Extract(&large, 1, 1, callback), S_OK);
        QCOMPARE(callback->result, Int32(NArchive::NExtract::NOperationResult::kDataError));
        source->bytes.truncate(2048);
        QVERIFY(reader->Open(input, nullptr, nullptr) != S_OK);
    }
    void sparseUnsupportedAndCorruptMetadata() {
        const QByteArray original = fixture("basic");
        auto reader = archive();
        CMyComPtr<IInStream> initial = new Input(original);
        QCOMPARE(reader->Open(initial, nullptr, nullptr), S_OK);
        UInt32 index = find(reader, "large.bin");
        QVERIFY(index != UINT32_MAX);
        NWindows::NCOM::CPropVariant ino;
        QCOMPARE(reader->GetProperty(index, kpidINode, &ino), S_OK);
        QCOMPARE(ino.vt, VT_UI8);
        const int offset = inodeOffset(original, UInt32(ino.uhVal.QuadPart));
        QVERIFY(offset > 0);

        QByteArray sparse = original;
        qToLittleEndian(quint32(0), sparse.data() + offset + 360);
        CMyComPtr<IInStream> holes = new Input(sparse);
        QCOMPARE(reader->Open(holes, nullptr, nullptr), S_OK);
        index = find(reader, "large.bin");
        CMyComPtr<IInArchiveGetStream> streams;
        QCOMPARE(
            reader->QueryInterface(IID_IInArchiveGetStream, reinterpret_cast<void **>(&streams)),
            S_OK);
        CMyComPtr<ISequentialInStream> stream;
        QCOMPARE(streams->GetStream(index, &stream), S_OK);
        QByteArray bytes(4096, 'x');
        UInt32 done = 0;
        QCOMPARE(stream->Read(bytes.data(), bytes.size(), &done), S_OK);
        QCOMPARE(done, UInt32(4096));
        QCOMPARE(bytes, QByteArray(4096, '\0'));

        QByteArray encrypted = original;
        encrypted[offset + 2] = char(encrypted[offset + 2] | 0x04);
        CMyComPtr<IInStream> crypt = new Input(encrypted);
        QCOMPARE(reader->Open(crypt, nullptr, nullptr), S_OK);
        index = find(reader, "large.bin");
        CMyComPtr2<IArchiveExtractCallback, Extract> callback;
        callback.Create_if_Empty();
        QCOMPARE(reader->Extract(&index, 1, 0, callback), S_OK);
        QCOMPARE(callback->result, Int32(NArchive::NExtract::NOperationResult::kUnsupportedMethod));
        QVERIFY(callback->output->bytes.isEmpty());
        stream.Release();
        QCOMPARE(streams->GetStream(index, &stream), E_NOTIMPL);

        // A directory encryption policy must not hide readable siblings or
        // report a successful recursive extraction of its unavailable children.
        const UInt32 directory = find(reader, QString::fromUtf8("папка 空"));
        QVERIFY(directory != UINT32_MAX);
        QCOMPARE(reader->GetProperty(directory, kpidINode, &ino), S_OK);
        const int directoryOffset = inodeOffset(original, UInt32(ino.uhVal.QuadPart));
        QVERIFY(directoryOffset > 0);
        QByteArray opaqueDirectory = original;
        opaqueDirectory[directoryOffset + 2] = char(opaqueDirectory[directoryOffset + 2] | 0x04);
        CMyComPtr<IInStream> opaque = new Input(opaqueDirectory);
        QCOMPARE(reader->Open(opaque, nullptr, nullptr), S_OK);
        QVERIFY(find(reader, "large.bin") != UINT32_MAX);
        QCOMPARE(find(reader, QString::fromUtf8("папка 空/tiny.txt")), UINT32_MAX);
        const UInt32 unavailable = find(reader, QString::fromUtf8("папка 空"));
        QVERIFY(unavailable != UINT32_MAX);
        QCOMPARE(reader->Extract(&unavailable, 1, 0, callback), S_OK);
        QCOMPARE(callback->result, Int32(NArchive::NExtract::NOperationResult::kUnsupportedMethod));

        QByteArray malicious = original;
        malicious.replace("large.bin", "../evilxx");
        CMyComPtr<IInStream> unsafe = new Input(malicious);
        QVERIFY(reader->Open(unsafe, nullptr, nullptr) != S_OK);

        // The optimized read-only layout uses different active log counts;
        // recognizing its bit must not imply that its mapper is implemented.
        QByteArray optimized = original;
        qToLittleEndian(quint32(0x4000), optimized.data() + 1024 + 2180);
        CMyComPtr<IInStream> roLayout = new Input(optimized);
        QCOMPARE(reader->Open(roLayout, nullptr, nullptr), E_NOTIMPL);

        QByteArray checksummed = fixture("modern");
        CMyComPtr<IInStream> modern = new Input(checksummed);
        QCOMPARE(reader->Open(modern, nullptr, nullptr), S_OK);
        index = find(reader, "large.bin");
        QCOMPARE(reader->GetProperty(index, kpidINode, &ino), S_OK);
        const int modernOffset = inodeOffset(checksummed, UInt32(ino.uhVal.QuadPart));
        QVERIFY(modernOffset > 0);
        checksummed[modernOffset + 16] = char(checksummed[modernOffset + 16] ^ 1);
        CMyComPtr<IInStream> corrupt = new Input(checksummed);
        QCOMPARE(reader->Open(corrupt, nullptr, nullptr), S_FALSE);
    }
};
QTEST_GUILESS_MAIN(F2fsModuleTests)
#include "f2fs_module_tests.moc"
