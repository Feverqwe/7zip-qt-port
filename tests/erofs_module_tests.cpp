#include "7zip/Archive/IArchive.h"
#include "Common/MyCom.h"
#include "Windows/PropVariant.h"
#include "archivebackend.h"
#include "archivecodeccatalog.h"
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLibrary>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QtEndian>
#include <QtTest>
#include <algorithm>

namespace {
const QString name = QStringLiteral("данные 空.bin");
QJsonObject manifest() {
    QFile file(QString::fromUtf8(SEVENZIP_EROFS_FIXTURES) + "/manifest.json");
    if (!file.open(QIODevice::ReadOnly))
        return {};
    return QJsonDocument::fromJson(file.readAll()).object();
}
QByteArray decodeFixture(const QString &filename, quint32 size) {
    QFile file(QString::fromUtf8(SEVENZIP_EROFS_FIXTURES) + '/' + filename);
    if (!file.open(QIODevice::ReadOnly))
        return {};
    QByteArray bytes(4, '\0');
    qToBigEndian(size, bytes.data());
    bytes += file.readAll();
    return qUncompress(bytes);
}
QByteArray image(const QString &variant = "lz4") {
    return decodeFixture(
        variant + ".erofs.zlib",
        quint32(manifest()["images"].toObject()[variant].toObject()["size"].toInt()));
}
QByteArray payload() {
    return decodeFixture("payload.bin.zlib", quint32(manifest()["payload_size"].toInt()));
}
QByteArray blob() {
    return decodeFixture("blob.bin.zlib", quint32(manifest()["images"]
                                                      .toObject()["external-device"]
                                                      .toObject()["devices"]
                                                      .toObject()["blob.bin"]
                                                      .toObject()["size"]
                                                      .toInt()));
}
class Input final : public IInStream, public CMyUnknownImp {
    Z7_COM_UNKNOWN_IMP_2(ISequentialInStream, IInStream)
    QByteArray bytes_;
    UInt64 position_ = 0;

  public:
    bool fail = false;
    UInt64 failAt = UINT64_MAX, notifyAt = UINT64_MAX;
    HRESULT atResult = E_FAIL;
    bool notified = false;
    explicit Input(QByteArray bytes) : bytes_(std::move(bytes)) {}
    Z7_COM7F_IMF(Read(void *data, UInt32 size, UInt32 *processed)) override {
        if (processed)
            *processed = 0;
        if (fail)
            return E_FAIL;
        if (failAt >= position_ && failAt - position_ < size)
            return atResult;
        if (notifyAt >= position_ && notifyAt - position_ < size)
            notified = true;
        size = UInt32(
            qMin<UInt64>(size, position_ < UInt64(bytes_.size()) ? bytes_.size() - position_ : 0));
        if (size)
            memcpy(data, bytes_.constData() + position_, size);
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
                                                       : bytes_.size();
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
    bool cancelAfterOutput = false;
    UInt64 completed = 0;
    Int32 result = -1;
    Extract() { output.Create_if_Empty(); }
    Z7_COM7F_IMF(SetTotal(UInt64)) override { return S_OK; }
    Z7_COM7F_IMF(SetCompleted(const UInt64 *value)) override {
        if (value)
            completed = *value;
        return cancel || (cancelAfterOutput && !output->bytes.isEmpty()) ? E_ABORT : S_OK;
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
class OpenCallback final : public IArchiveOpenCallback,
                           public IArchiveOpenVolumeCallback,
                           public CMyUnknownImp {
    Z7_COM_UNKNOWN_IMP_2(IArchiveOpenCallback, IArchiveOpenVolumeCallback)
  public:
    bool cancel = false, cancelDevice = false;
    const Input *observed = nullptr;
    QByteArray device;
    QString requested;
    Z7_COM7F_IMF(SetTotal(const UInt64 *, const UInt64 *)) override { return S_OK; }
    Z7_COM7F_IMF(SetCompleted(const UInt64 *, const UInt64 *)) override {
        return cancel || (observed && observed->notified) ? E_ABORT : S_OK;
    }
    Z7_COM7F_IMF(GetProperty(PROPID, PROPVARIANT *value)) override {
        NWindows::NCOM::CPropVariant prop;
        prop.Detach(value);
        return S_OK;
    }
    Z7_COM7F_IMF(GetStream(const wchar_t *filename, IInStream **stream)) override {
        *stream = nullptr;
        requested = QString::fromWCharArray(filename);
        if (cancelDevice)
            return E_ABORT;
        if (requested != "blob.bin" || device.isEmpty())
            return S_FALSE;
        CMyComPtr<IInStream> input = new Input(device);
        *stream = input.Detach();
        return S_OK;
    }
};
QByteArray wrappedFlatExtent() {
    QByteArray data(3 * 65536, '\0');
    auto u16 = [&](int offset, quint16 value) { qToLittleEndian(value, data.data() + offset); };
    auto u32 = [&](int offset, quint32 value) { qToLittleEndian(value, data.data() + offset); };
    auto u64 = [&](int offset, quint64 value) { qToLittleEndian(value, data.data() + offset); };
    u32(1024, 0xe0f5e1e2);
    data[1036] = 16;
    u64(1040, 2);
    u32(1060, 3);
    u32(1064, 1);
    u32(1104, 0x80);
    const int root = 65536, file = root + 128;
    u16(root, 4);
    u16(root + 4, 0040755);
    u16(root + 6, 2);
    u32(root + 8, 42);
    const int entries = root + 32;
    u64(entries, 0);
    u16(entries + 8, 36);
    data[entries + 10] = 2;
    u64(entries + 12, 0);
    u16(entries + 20, 37);
    data[entries + 22] = 2;
    u64(entries + 24, 4);
    u16(entries + 32, 39);
    data[entries + 34] = 1;
    memcpy(data.data() + entries + 36, "...bad", 6);
    u16(file, 1);
    u16(file + 4, 0100644);
    u16(file + 6, 0xffff);
    u64(file + 8, 196608);
    u32(file + 16, 0xfffffffe);
    u32(file + 44, 1);
    return data;
}
} // namespace

class ErofsModuleTests final : public QObject {
    Q_OBJECT
    QLibrary module_{QString::fromUtf8(SEVENZIP_EROFS_MODULE)};
    CMyComPtr<IInArchive> archive_;
    HRESULT open(QByteArray data, IArchiveOpenCallback *callback = nullptr) {
        CMyComPtr<IInStream> source = new Input(std::move(data));
        return archive_->Open(source, nullptr, callback);
    }
    UInt32 fileIndex(const QString &path = name) {
        UInt32 count = 0;
        archive_->GetNumberOfItems(&count);
        for (UInt32 i = 0; i < count; ++i) {
            NWindows::NCOM::CPropVariant value;
            if (archive_->GetProperty(i, kpidPath, &value) == S_OK && value.vt == VT_BSTR &&
                QDir::fromNativeSeparators(QString::fromWCharArray(value.bstrVal)) == path)
                return i;
        }
        return UInt32(-1);
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
        CMyComPtr2<IArchiveOpenCallback, OpenCallback> cancelled;
        cancelled.Create_if_Empty();
        cancelled->cancel = true;
        QCOMPARE(open(image(), cancelled), E_ABORT);
        UInt32 count = 99;
        QCOMPARE(archive_->GetNumberOfItems(&count), S_OK);
        QCOMPARE(count, 0u);
        auto *input = new Input(image());
        CMyComPtr<IInStream> source = input;
        input->fail = true;
        QCOMPARE(archive_->Open(source, nullptr, nullptr), E_FAIL);
        input->fail = false;
        QCOMPARE(archive_->Open(source, nullptr, nullptr), S_OK);
        const UInt32 index = fileIndex();
        QVERIFY(index != UInt32(-1));
        CMyComPtr2<IArchiveExtractCallback, Extract> callback;
        callback.Create_if_Empty();
        callback->cancel = true;
        QCOMPARE(archive_->Extract(&index, 1, 0, callback), E_ABORT);
        callback->cancel = false;
        input->fail = true;
        QCOMPARE(archive_->Extract(&index, 1, 0, callback), E_FAIL);
        input->fail = false;
        callback->output->fail = true;
        QCOMPARE(archive_->Extract(&index, 1, 0, callback), E_FAIL);
        callback->output->fail = false;
        callback->output->bytes.clear();
        QCOMPARE(archive_->Extract(&index, 1, 0, callback), S_OK);
        QCOMPARE(callback->result, Int32(NArchive::NExtract::NOperationResult::kOK));
        QCOMPARE(callback->output->bytes, payload());
    }
    void streamSurvivesCloseReopenAndRelease() {
        CMyComPtr<IInStream> embedded = new Input(QByteArray(512, 'x') + image());
        QCOMPARE(embedded->Seek(512, STREAM_SEEK_SET, nullptr), S_OK);
        QCOMPARE(archive_->Open(embedded, nullptr, nullptr), S_OK);
        CMyComPtr<IInArchiveGetStream> provider;
        QCOMPARE(
            archive_->QueryInterface(IID_IInArchiveGetStream, reinterpret_cast<void **>(&provider)),
            S_OK);
        CMyComPtr<ISequentialInStream> sequential;
        QCOMPARE(provider->GetStream(fileIndex(), &sequential), S_OK);
        CMyComPtr<IInStream> stream;
        QCOMPARE(sequential->QueryInterface(IID_IInStream, reinterpret_cast<void **>(&stream)),
                 S_OK);
        QCOMPARE(archive_->Close(), S_OK);
        QCOMPARE(open(image("plain")), S_OK);
        provider.Release();
        archive_.Release();
        sequential.Release();
        const auto expected = payload();
        for (qint64 offset : {qint64(65530), qint64(0), qint64(expected.size() - 17)}) {
            QCOMPARE(stream->Seek(offset, STREAM_SEEK_SET, nullptr), S_OK);
            char bytes[37];
            UInt32 count = 0;
            QCOMPARE(stream->Read(bytes, sizeof(bytes), &count), S_OK);
            QCOMPARE(QByteArray(bytes, count), expected.mid(offset, sizeof(bytes)));
        }
        createArchive();
    }
    void auxiliaryDevicesAndCancellation() {
        CMyComPtr2<IArchiveOpenCallback, OpenCallback> callback;
        callback.Create_if_Empty();
        QCOMPARE(open(image("external-device"), callback), S_FALSE);
        callback->device = blob();
        callback->cancelDevice = true;
        QCOMPARE(open(image("external-device"), callback), E_ABORT);
        callback->cancelDevice = false;
        QCOMPARE(open(image("external-device"), callback), S_OK);
        QCOMPARE(callback->requested, QString("blob.bin"));
        CMyComPtr2<IArchiveExtractCallback, Extract> extracted;
        extracted.Create_if_Empty();
        UInt32 index = fileIndex();
        QCOMPARE(archive_->Extract(&index, 1, 0, extracted), S_OK);
        QCOMPARE(extracted->result, Int32(NArchive::NExtract::NOperationResult::kOK));
        QCOMPARE(extracted->output->bytes, payload());
        callback->device.clear();
        QCOMPARE(open(image("unified-device"), callback), S_OK);
        index = fileIndex();
        extracted->output->bytes.clear();
        QCOMPARE(archive_->Extract(&index, 1, 0, extracted), S_OK);
        QCOMPARE(extracted->output->bytes, payload());
    }
    void rejectsWrappedFlatExtentWithoutPublishingPartialTree() {
        QCOMPARE(open(wrappedFlatExtent()), S_FALSE);
        UInt32 count = 99;
        QCOMPARE(archive_->GetNumberOfItems(&count), S_OK);
        QCOMPARE(count, 0u);
        QCOMPARE(open(image()), S_OK);
        QVERIFY(fileIndex() != UInt32(-1));
    }
    void backendDiscoveryAndExtraction() {
        QTemporaryDir runtime;
        QVERIFY(runtime.isValid());
#ifdef Q_OS_WIN
        const QString engineName = "7z.exe", coreName = "7z.dll";
#else
        const QString engineName = "7z", coreName = "7z.so";
#endif
        const QDir prepared(QString::fromUtf8(SEVENZIP_EROFS_RUNTIME));
        const QString engine = runtime.filePath(engineName), core = runtime.filePath(coreName);
        QVERIFY(QFile::copy(prepared.filePath(engineName), engine));
        QVERIFY(QFile::copy(prepared.filePath(coreName), core));
        QVERIFY(QDir(runtime.path()).mkdir("Formats"));
        const QString module = QString::fromUtf8(SEVENZIP_EROFS_MODULE);
        QVERIFY(QFile::copy(module, runtime.filePath("Formats/" + QFileInfo(module).fileName())));
        const QString path = runtime.filePath(QStringLiteral("образ 空.img"));
        QFile source(path);
        QVERIFY(source.open(QIODevice::WriteOnly));
        const auto data = image("xattr-values-metabox");
        QCOMPARE(source.write(data), data.size());
        source.close();
        ArchiveBackend backend(nullptr, engine);
        QSignalSpy ready(&backend, &ArchiveBackend::codecCatalogReady);
        QVERIFY(backend.discoverCodecCatalog(core));
        QTRY_COMPARE_WITH_TIMEOUT(ready.size(), 1, 15000);
        QVERIFY2(ready[0][0].toBool(), qPrintable(ready[0][1].toString()));
        QVERIFY(backend.codecCatalog()->formatIndex("EROFS") >= 0);
        QVERIFY(!backend.codecCatalog()->canUpdate("EROFS"));
        QList<ArchiveEntry> entries;
        connect(&backend, &ArchiveBackend::entriesReady, this,
                [&](const QList<ArchiveEntry> &result) { entries = result; });
        QSignalSpy finished(&backend, &ArchiveBackend::finished);
        backend.list(path);
        QTRY_COMPARE_WITH_TIMEOUT(finished.size(), 1, 10000);
        QVERIFY2(finished[0][0].toBool(), qPrintable(finished[0][1].toString()));
        QVERIFY(std::any_of(entries.begin(), entries.end(),
                            [](const ArchiveEntry &entry) { return entry.path == name; }));
        finished.clear();
        backend.test(path);
        QTRY_COMPARE_WITH_TIMEOUT(finished.size(), 1, 10000);
        QVERIFY2(finished[0][0].toBool(), qPrintable(finished[0][1].toString()));
        finished.clear();
        const QString output = runtime.filePath("output");
        backend.extract(path, output, {name});
        QTRY_COMPARE_WITH_TIMEOUT(finished.size(), 1, 10000);
        QVERIFY2(finished[0][0].toBool(), qPrintable(finished[0][1].toString()));
        QFile result(QDir(output).filePath(name));
        QVERIFY(result.open(QIODevice::ReadOnly));
        QCOMPARE(result.readAll(), payload());
        for (const QString &entry :
             {name + ":$XATTR.user.large", name + ":$XATTR.system.posix_acl_access",
              QString::fromUtf8("папка:$XATTR.system.posix_acl_default"),
              QString("[ROOT]_:$XATTR.user.root")}) {
            QVERIFY(std::any_of(entries.cbegin(), entries.cend(), [&](const ArchiveEntry &value) {
                return value.path == entry && !value.directory &&
                       value.properties.value("Alternate Stream") == "+";
            }));
            finished.clear();
            const QString saved = runtime.filePath("selected.bin");
            QFile::remove(saved);
            backend.extractSourceEntry(path, entry, saved, "EROFS", {}, true);
            QTRY_COMPARE_WITH_TIMEOUT(finished.size(), 1, 10000);
            QVERIFY2(finished[0][0].toBool(), qPrintable(finished[0][1].toString()));
            QFile extracted(saved);
            QVERIFY(extracted.open(QIODevice::ReadOnly));
            const int colon = entry.indexOf(':');
            const QString host = entry.left(colon) == "[ROOT]_" ? QString() : entry.left(colon);
            const QString attribute = entry.mid(colon + QString(":$XATTR.").size());
            const auto values = manifest()["images"]
                                    .toObject()["xattr-values-metabox"]
                                    .toObject()["attributes"]
                                    .toObject()[host]
                                    .toObject();
            QCOMPARE(extracted.readAll(),
                     QByteArray::fromHex(values[attribute].toString().toLatin1()));
        }
        QVERIFY(source.open(QIODevice::ReadOnly));
        QCOMPARE(source.readAll(), data);
    }
    void metadataStreamOwnershipAndParent() {
        QCOMPARE(open(image("xattr-values-prefix")), S_OK);
        const UInt32 host = fileIndex();
        const UInt32 index = fileIndex(name + ":$XATTR.user.large");
        QVERIFY(host != UInt32(-1) && index != UInt32(-1));
        NWindows::NCOM::CPropVariant prop;
        QCOMPARE(archive_->GetProperty(index, kpidIsAltStream, &prop), S_OK);
        QCOMPARE(prop.vt, VARTYPE(VT_BOOL));
        QCOMPARE(prop.boolVal, VARIANT_TRUE);
        CMyComPtr<IArchiveGetRawProps> raw;
        QCOMPARE(archive_->QueryInterface(IID_IArchiveGetRawProps, reinterpret_cast<void **>(&raw)),
                 S_OK);
        UInt32 parent = 0, type = 0;
        QCOMPARE(raw->GetParent(index, &parent, &type), S_OK);
        QCOMPARE(parent, host);
        QCOMPARE(type, UInt32(NParentType::kAltStream));
        CMyComPtr<IInArchiveGetStream> provider;
        QCOMPARE(
            archive_->QueryInterface(IID_IInArchiveGetStream, reinterpret_cast<void **>(&provider)),
            S_OK);
        CMyComPtr<ISequentialInStream> stream, empty;
        QCOMPARE(provider->GetStream(index, &stream), S_OK);
        QCOMPARE(provider->GetStream(fileIndex(name + ":$XATTR.user.empty"), &empty), S_OK);
        QCOMPARE(archive_->Close(), S_OK);
        QCOMPARE(open(image()), S_OK);
        provider.Release();
        raw.Release();
        archive_.Release();
        QByteArray actual(9000, '\0');
        UInt32 processed = 0;
        QCOMPARE(stream->Read(actual.data(), actual.size(), &processed), S_OK);
        QCOMPARE(processed, UInt32(actual.size()));
        const auto expected = manifest()["images"]
                                  .toObject()["xattr-values-prefix"]
                                  .toObject()["attributes"]
                                  .toObject()[name]
                                  .toObject()["user.large"]
                                  .toString();
        QCOMPARE(actual, QByteArray::fromHex(expected.toLatin1()));
        char byte = 'x';
        QCOMPARE(empty->Read(&byte, 1, &processed), S_OK);
        QCOMPARE(processed, UInt32(0));
        QCOMPARE(byte, 'x');
        createArchive();
    }
    void metadataReadFailureCancellationAndReuse() {
        QByteArray data = image("xattr-values-shared");
        const unsigned bits = quint8(data[1036]);
        const int old = int(qFromLittleEndian<quint32>(data.constData() + 1068) << bits);
        const QByteArray shared = data.mid(old);
        const int relocated = (data.size() + 65535) & ~65535;
        data.resize(relocated);
        data.append(shared);
        data.resize((data.size() + (1 << bits) - 1) & ~((1 << bits) - 1));
        qToLittleEndian(quint32(relocated >> bits), data.data() + 1068);
        qToLittleEndian(quint32(data.size() >> bits), data.data() + 1060);
        qToLittleEndian(qFromLittleEndian<quint32>(data.constData() + 1032) & ~quint32(1),
                        data.data() + 1032);
        CMyComPtr2<IInStream, Input> source;
        source.SetFromCls(new Input(data));
        source->failAt = UInt64(relocated);
        for (HRESULT result : {E_FAIL, E_ABORT}) {
            QCOMPARE(source->Seek(0, STREAM_SEEK_SET, nullptr), S_OK);
            source->atResult = result;
            QCOMPARE(archive_->Open(source, nullptr, nullptr), result);
            UInt32 count = 99;
            QCOMPARE(archive_->GetNumberOfItems(&count), S_OK);
            QCOMPARE(count, UInt32(0));
        }
        source->notifyAt = source->failAt;
        source->failAt = UINT64_MAX;
        CMyComPtr2<IArchiveOpenCallback, OpenCallback> callback;
        callback.SetFromCls(new OpenCallback);
        callback->observed = source.ClsPtr();
        QCOMPARE(source->Seek(0, STREAM_SEEK_SET, nullptr), S_OK);
        QCOMPARE(archive_->Open(source, nullptr, callback), E_ABORT);
        source->notified = false;
        source->notifyAt = UINT64_MAX;
        QCOMPARE(source->Seek(0, STREAM_SEEK_SET, nullptr), S_OK);
        QCOMPARE(archive_->Open(source, nullptr, callback), S_OK);
        const UInt32 index = fileIndex(name + ":$XATTR.user.large");
        QVERIFY(index != UInt32(-1));
        CMyComPtr2<IArchiveExtractCallback, Extract> output;
        output.Create_if_Empty();
        QCOMPARE(archive_->Extract(&index, 1, 0, output), S_OK);
        QCOMPARE(output->result, Int32(NArchive::NExtract::NOperationResult::kOK));
        const auto expected = manifest()["images"]
                                  .toObject()["xattr-values-shared"]
                                  .toObject()["attributes"]
                                  .toObject()[name]
                                  .toObject()["user.large"]
                                  .toString();
        QCOMPARE(output->output->bytes, QByteArray::fromHex(expected.toLatin1()));
        QCOMPARE(output->completed, UInt64(9000));
        output->cancelAfterOutput = true;
        output->output->bytes.clear();
        QCOMPARE(archive_->Extract(&index, 1, 0, output), E_ABORT);
        output->cancelAfterOutput = false;
        output->output->bytes.clear();
        output->output->fail = true;
        QCOMPARE(archive_->Extract(&index, 1, 0, output), E_FAIL);
        output->output->fail = false;
        QCOMPARE(archive_->Extract(&index, 1, 0, output), S_OK);
        QCOMPARE(output->output->bytes, QByteArray::fromHex(expected.toLatin1()));
    }
    void cleanupTestCase() { archive_.Release(); }
};
QTEST_GUILESS_MAIN(ErofsModuleTests)
#include "erofs_module_tests.moc"
