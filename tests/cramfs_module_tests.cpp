#include "7zip/Archive/IArchive.h"
#include "Common/MyCom.h"
#include "Windows/PropVariant.h"
#include "archivebackend.h"
#include "archivecodeccatalog.h"
#include <QDir>
#include <QFile>
#include <QLibrary>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QtEndian>
#include <QtTest>

namespace {
const QString name = QStringLiteral("файл 空.bin");
const QByteArray contents = QByteArray(4096, 'A') + "tail";
QByteArray fixture() {
    QByteArray filename = name.toUtf8();
    filename.append(QByteArray((4 - filename.size() % 4) % 4, '\0'));
    const int root = 512 + 76, table = root + 12 + filename.size(), data = table + 8;
    QByteArray image(data + contents.size(), '\0');
    auto put = [&](int offset, quint32 value) { qToLittleEndian(value, image.data() + offset); };
    put(512, 0x28cd3d45);
    put(512 + 8, 0x800); // Version 1, extended block pointers.
    memcpy(image.data() + 512 + 16, "Compressed ROMFS", 16);
    put(512 + 64, 0x41ed);
    put(512 + 68, 12 + filename.size());
    put(512 + 72, (root / 4) << 6);
    put(root, 0x81a4 | (1234u << 16));
    put(root + 4, quint32(contents.size()) | (42u << 24));
    put(root + 8, ((table / 4) << 6) | (filename.size() / 4));
    memcpy(image.data() + root + 12, filename.constData(), filename.size());
    put(table, 0xc0000000 | (data / 4));
    put(table + 4, 0xc0000000 | ((data + 4096) / 4));
    memcpy(image.data() + data, contents.constData(), contents.size());
    return image;
}
class Input final : public IInStream, public CMyUnknownImp {
    Z7_COM_UNKNOWN_IMP_2(ISequentialInStream, IInStream)
    QByteArray bytes_;
    UInt64 position_ = 0;
    UInt64 tail_ = 0;
    UInt64 length_;

  public:
    bool fail = false;
    explicit Input(QByteArray bytes, bool distant = false) : bytes_(std::move(bytes)) {
        if (distant) {
            tail_ = UInt64(1) << 30;
            const int table = bytes_.size() - contents.size() - 8;
            qToLittleEndian(quint32(0xc0000000 | (tail_ / 4)), bytes_.data() + table);
            qToLittleEndian(quint32(0xc0000000 | ((tail_ + 4096) / 4)), bytes_.data() + table + 4);
        }
        length_ = tail_ ? tail_ + contents.size() : bytes_.size();
    }
    Z7_COM7F_IMF(Read(void *data, UInt32 size, UInt32 *processed)) override {
        if (processed)
            *processed = 0;
        if (fail)
            return E_FAIL;
        size = UInt32(qMin<UInt64>(size, position_ < length_ ? length_ - position_ : 0));
        for (UInt32 i = 0; i < size; ++i) {
            const UInt64 pos = position_ + i;
            static_cast<char *>(data)[i] = !tail_         ? bytes_[qsizetype(pos)]
                                           : pos >= tail_ ? contents[qsizetype(pos - tail_)]
                                           : pos < UInt64(bytes_.size() - contents.size())
                                               ? bytes_[qsizetype(pos)]
                                               : '\0';
        }
        position_ += size;
        if (processed)
            *processed = size;
        return S_OK;
    }
    Z7_COM7F_IMF(Seek(Int64 offset, UInt32 origin, UInt64 *position)) override {
        const Int64 base = origin == STREAM_SEEK_SET   ? 0
                           : origin == STREAM_SEEK_CUR ? Int64(position_)
                                                       : Int64(length_);
        if (origin > STREAM_SEEK_END || offset < -base || (offset > 0 && base > INT64_MAX - offset))
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
    bool fail = false;
    Z7_COM7F_IMF(Write(const void *data, UInt32 size, UInt32 *processed)) override {
        if (processed)
            *processed = 0;
        if (fail)
            return E_FAIL;
        bytes.append(static_cast<const char *>(data), size);
        if (processed)
            *processed = size;
        return S_OK;
    }
};
class Extract final : public IArchiveExtractCallback, public CMyUnknownImp {
    Z7_COM_UNKNOWN_IMP_1(IArchiveExtractCallback)
  public:
    CMyComPtr2<ISequentialOutStream, Output> output;
    bool cancel = false;
    Int32 result = -1;
    Extract() { output.Create_if_Empty(); }
    Z7_COM7F_IMF(SetTotal(UInt64)) override { return S_OK; }
    Z7_COM7F_IMF(SetCompleted(const UInt64 *)) override { return cancel ? E_ABORT : S_OK; }
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
class CancelOpen final : public IArchiveOpenCallback, public CMyUnknownImp {
    Z7_COM_UNKNOWN_IMP_1(IArchiveOpenCallback)
  public:
    Z7_COM7F_IMF(SetTotal(const UInt64 *, const UInt64 *)) override { return S_OK; }
    Z7_COM7F_IMF(SetCompleted(const UInt64 *, const UInt64 *)) override { return E_ABORT; }
};
} // namespace

class CramfsModuleTests final : public QObject {
    Q_OBJECT
    QLibrary module_{QString::fromUtf8(SEVENZIP_CRAMFS_MODULE)};
    CMyComPtr<IInArchive> archive_;
    HRESULT open(QByteArray data, IArchiveOpenCallback *callback = nullptr) {
        CMyComPtr<IInStream> source = new Input(std::move(data));
        return archive_->Open(source, nullptr, callback);
    }
    void createArchive() {
        archive_.Release();
        QVERIFY2(module_.load(), qPrintable(module_.errorString()));
        const auto property =
            reinterpret_cast<Func_GetHandlerProperty2>(module_.resolve("GetHandlerProperty2"));
        const auto create = reinterpret_cast<Func_CreateObject>(module_.resolve("CreateObject"));
        QVERIFY(property && create);
        NWindows::NCOM::CPropVariant clsid;
        QCOMPARE(property(0, NArchive::NHandlerPropID::kClassID, &clsid), S_OK);
        GUID guid;
        memcpy(&guid, clsid.bstrVal, sizeof(guid));
        QCOMPARE(create(&guid, &IID_IInArchive, reinterpret_cast<void **>(&archive_)), S_OK);
    }
  private slots:
    void initTestCase() { createArchive(); }
    void failuresCancellationAndReuse() {
        CMyComPtr<IArchiveOpenCallback> cancelled = new CancelOpen;
        QCOMPARE(open(fixture(), cancelled), E_ABORT);
        UInt32 count = 99;
        QCOMPARE(archive_->GetNumberOfItems(&count), S_OK);
        QCOMPARE(count, 0u);
        auto *input = new Input(fixture());
        CMyComPtr<IInStream> source = input;
        input->fail = true;
        QCOMPARE(archive_->Open(source, nullptr, nullptr), E_FAIL);
        QCOMPARE(open(fixture()), S_OK);
        const UInt32 index = 0;
        CMyComPtr2<IArchiveExtractCallback, Extract> callback;
        callback.Create_if_Empty();
        callback->cancel = true;
        QCOMPARE(archive_->Extract(&index, 1, 0, callback), E_ABORT);
        callback->cancel = false;
        callback->output->fail = true;
        QCOMPARE(archive_->Extract(&index, 1, 0, callback), E_FAIL);
        callback->output->fail = false;
        callback->output->bytes.clear();
        QCOMPARE(archive_->Extract(&index, 1, 0, callback), S_OK);
        QCOMPARE(callback->result, Int32(NArchive::NExtract::NOperationResult::kOK));
        QCOMPARE(callback->output->bytes, contents);
    }
    void randomAccessAndStreamLifetime() {
        QCOMPARE(open(fixture()), S_OK);
        CMyComPtr<IInArchiveGetStream> provider;
        QCOMPARE(
            archive_->QueryInterface(IID_IInArchiveGetStream, reinterpret_cast<void **>(&provider)),
            S_OK);
        CMyComPtr<ISequentialInStream> sequential;
        QCOMPARE(provider->GetStream(0, &sequential), S_OK);
        CMyComPtr<IInStream> stream;
        QCOMPARE(sequential->QueryInterface(IID_IInStream, reinterpret_cast<void **>(&stream)),
                 S_OK);
        QCOMPARE(stream->Seek(4096, STREAM_SEEK_SET, nullptr), S_OK);
        char buffer[4];
        UInt32 processed = 0;
        QCOMPARE(stream->Read(buffer, 4, &processed), S_OK);
        QCOMPARE(QByteArray(buffer, processed), QByteArray("tail"));
        QCOMPARE(stream->Seek(0, STREAM_SEEK_SET, nullptr), S_OK);
        QCOMPARE(stream->Read(buffer, 4, &processed), S_OK);
        QCOMPARE(QByteArray(buffer, processed), QByteArray("AAAA"));
        provider.Release();
        archive_.Release();
        QCOMPARE(stream->Seek(4096, STREAM_SEEK_SET, nullptr), S_OK);
        QCOMPARE(stream->Read(buffer, 4, &processed), S_OK);
        QCOMPARE(QByteArray(buffer, processed), QByteArray("tail"));
    }
    void distantDirectExtentsAndReadFailure() {
        createArchive();
        auto *input = new Input(fixture(), true);
        CMyComPtr<IInStream> source = input;
        QCOMPARE(archive_->Open(source, nullptr, nullptr), S_OK);
        CMyComPtr2<IArchiveExtractCallback, Extract> callback;
        callback.Create_if_Empty();
        const UInt32 index = 0;
        input->fail = true;
        QCOMPARE(archive_->Extract(&index, 1, 0, callback), E_FAIL);
        input->fail = false;
        QCOMPARE(archive_->Extract(&index, 1, 0, callback), S_OK);
        QCOMPARE(callback->result, Int32(NArchive::NExtract::NOperationResult::kOK));
        QCOMPARE(callback->output->bytes, contents);
    }
    void backendDiscoveryListingTestAndExtraction() {
        QTemporaryDir runtime;
        QVERIFY(runtime.isValid());
#ifdef Q_OS_WIN
        const QString engineName = "7z.exe", coreName = "7z.dll";
#else
        const QString engineName = "7z", coreName = "7z.so";
#endif
        const QDir prepared(QString::fromUtf8(SEVENZIP_CRAMFS_RUNTIME));
        const QString engine = runtime.filePath(engineName), core = runtime.filePath(coreName);
        QVERIFY(QFile::copy(prepared.filePath(engineName), engine));
        QVERIFY(QFile::copy(prepared.filePath(coreName), core));
        QVERIFY(QDir(runtime.path()).mkdir("Formats"));
        const QString module = QString::fromUtf8(SEVENZIP_CRAMFS_MODULE);
        QVERIFY(QFile::copy(module, runtime.filePath("Formats/" + QFileInfo(module).fileName())));
        const QString image = runtime.filePath(QStringLiteral("образ 空.img"));
        QFile source(image);
        QVERIFY(source.open(QIODevice::WriteOnly));
        const auto bytes = fixture();
        QCOMPARE(source.write(bytes), bytes.size());
        source.close();
        ArchiveBackend backend(nullptr, engine);
        QSignalSpy ready(&backend, &ArchiveBackend::codecCatalogReady);
        QVERIFY(backend.discoverCodecCatalog(core));
        QTRY_COMPARE_WITH_TIMEOUT(ready.size(), 1, 15000);
        QVERIFY2(ready[0][0].toBool(), qPrintable(ready[0][1].toString()));
        QVERIFY(!backend.codecCatalog()->canUpdate("CramFS"));
        QList<ArchiveEntry> entries;
        connect(&backend, &ArchiveBackend::entriesReady, this,
                [&](const QList<ArchiveEntry> &result) { entries = result; });
        QSignalSpy finished(&backend, &ArchiveBackend::finished);
        backend.list(image);
        QTRY_COMPARE_WITH_TIMEOUT(finished.size(), 1, 10000);
        QVERIFY2(finished[0][0].toBool(), qPrintable(finished[0][1].toString()));
        QCOMPARE(entries.size(), 1);
        QCOMPARE(entries[0].path, name);
        finished.clear();
        backend.test(image);
        QTRY_COMPARE_WITH_TIMEOUT(finished.size(), 1, 10000);
        QVERIFY2(finished[0][0].toBool(), qPrintable(finished[0][1].toString()));
        finished.clear();
        const QString output = runtime.filePath("output");
        backend.extract(image, output);
        QTRY_COMPARE_WITH_TIMEOUT(finished.size(), 1, 10000);
        QVERIFY2(finished[0][0].toBool(), qPrintable(finished[0][1].toString()));
        QFile result(QDir(output).filePath(name));
        QVERIFY(result.open(QIODevice::ReadOnly));
        QCOMPARE(result.readAll(), contents);
        QVERIFY(source.open(QIODevice::ReadOnly));
        QCOMPARE(source.readAll(), bytes);
    }
    void cleanupTestCase() { archive_.Release(); }
};
QTEST_GUILESS_MAIN(CramfsModuleTests)
#include "cramfs_module_tests.moc"
