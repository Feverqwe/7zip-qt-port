#include "7zip/Archive/IArchive.h"
#include "Common/MyCom.h"
#include "Windows/PropVariant.h"
#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLibrary>
#include <QProcess>
#include <QtEndian>
#include <QtTest>

namespace {
class Input final : public IInStream, public CMyUnknownImp {
    Z7_COM_UNKNOWN_IMP_2(ISequentialInStream, IInStream)
    QByteArray bytes_;
    UInt64 position_ = 0;

  public:
    bool fail = false;
    HRESULT readResult = S_OK;
    HRESULT seekResult = S_OK;
    UInt64 failAt = UINT64_MAX, notifyAt = UINT64_MAX;
    HRESULT atResult = E_FAIL;
    bool notified = false;
    explicit Input(QByteArray bytes) : bytes_(std::move(bytes)) {}
    Z7_COM7F_IMF(Read(void *data, UInt32 size, UInt32 *processed)) override {
        if (processed)
            *processed = 0;
        if (position_ <= failAt && failAt - position_ < size)
            return atResult;
        if (position_ <= notifyAt && notifyAt - position_ < size)
            notified = true;
        if (fail)
            return E_FAIL;
        if (readResult != S_OK)
            return readResult;
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
        if (seekResult != S_OK)
            return seekResult;
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
    Z7_COM7F_IMF(SetTotal(const UInt64 *, const UInt64 *)) override { return E_ABORT; }
    Z7_COM7F_IMF(SetCompleted(const UInt64 *, const UInt64 *)) override { return E_ABORT; }
};
class CancelXattrOpen final : public IArchiveOpenCallback, public CMyUnknownImp {
    Z7_COM_UNKNOWN_IMP_1(IArchiveOpenCallback)
  public:
    const Input *source = nullptr;
    Z7_COM7F_IMF(SetTotal(const UInt64 *, const UInt64 *)) override { return S_OK; }
    Z7_COM7F_IMF(SetCompleted(const UInt64 *, const UInt64 *)) override {
        return source && source->notified ? E_ABORT : S_OK;
    }
};
} // namespace

class ExtStreamTests final : public QObject {
    Q_OBJECT
    QLibrary module_{QString::fromUtf8(SEVENZIP_EXT_CORE)};
    CMyComPtr<IInArchive> archive_;
    QJsonObject signatures() {
        QFile file(QFileInfo(QString::fromUtf8(SEVENZIP_EXT_FIXTURE))
                       .dir()
                       .filePath("fixtures/ext4/verity-signatures.json"));
        if (!file.open(QIODevice::ReadOnly))
            return {};
        return QJsonDocument::fromJson(file.readAll()).object();
    }
    HRESULT signaturePolicy(bool require, const QString &certificate = {}) {
        CMyComPtr<ISetProperties> properties;
        const HRESULT status =
            archive_->QueryInterface(IID_ISetProperties, reinterpret_cast<void **>(&properties));
        if (status != S_OK)
            return status;
        const wchar_t *names[] = {L"verity.require", L"verity.trust"};
        NWindows::NCOM::CPropVariant values[2];
        values[0] = require;
        values[1] = certificate.toStdWString().c_str();
        return properties->SetProperties(names, values, certificate.isEmpty() ? 1 : 2);
    }
    QJsonObject bundle(const QString &layout) {
        QProcess generator;
        generator.start(QString::fromUtf8(SEVENZIP_EXT_PYTHON),
                        {QString::fromUtf8(SEVENZIP_EXT_FIXTURE), layout, "--bundle"});
        if (!generator.waitForFinished(10000) || generator.exitCode() != 0)
            return {};
        return QJsonDocument::fromJson(generator.readAllStandardOutput()).object();
    }
    static QByteArray image(const QJsonObject &record) {
        QByteArray data(4, '\0');
        qToBigEndian(quint32(record["size"].toInt()), data.data());
        data += QByteArray::fromBase64(record["image"].toString().toLatin1());
        return qUncompress(data);
    }
    UInt32 fileIndex(const QString &name = QStringLiteral("данные 空.bin")) {
        UInt32 count = 0;
        if (archive_->GetNumberOfItems(&count) != S_OK)
            return UInt32(-1);
        for (UInt32 i = 0; i < count; ++i) {
            NWindows::NCOM::CPropVariant path;
            if (archive_->GetProperty(i, kpidPath, &path) == S_OK && path.vt == VT_BSTR &&
                QDir::fromNativeSeparators(QString::fromWCharArray(path.bstrVal)) == name)
                return i;
        }
        return UInt32(-1);
    }
    void createArchive() {
        archive_.Release();
        QVERIFY2(module_.load(), qPrintable(module_.errorString()));
        const auto count =
            reinterpret_cast<Func_GetNumberOfFormats>(module_.resolve("GetNumberOfFormats"));
        const auto property =
            reinterpret_cast<Func_GetHandlerProperty2>(module_.resolve("GetHandlerProperty2"));
        const auto create = reinterpret_cast<Func_CreateObject>(module_.resolve("CreateObject"));
        QVERIFY(count && property && create);
        UInt32 formats = 0;
        QCOMPARE(count(&formats), S_OK);
        for (UInt32 i = 0; i < formats; ++i) {
            NWindows::NCOM::CPropVariant name;
            QCOMPARE(property(i, NArchive::NHandlerPropID::kName, &name), S_OK);
            if (name.vt != VT_BSTR || QString::fromWCharArray(name.bstrVal) != "Ext")
                continue;
            NWindows::NCOM::CPropVariant clsid;
            QCOMPARE(property(i, NArchive::NHandlerPropID::kClassID, &clsid), S_OK);
            GUID guid;
            memcpy(&guid, clsid.bstrVal, sizeof(guid));
            QCOMPARE(create(&guid, &IID_IInArchive, reinterpret_cast<void **>(&archive_)), S_OK);
            return;
        }
        QFAIL("Prepared engine has no Ext handler");
    }
  private slots:
    void initTestCase() { createArchive(); }
    void signedVerityStreamsKeepTheirPolicyAndInput() {
        const auto fixtures = signatures();
        const auto cases = fixtures["cases"].toArray();
        QVERIFY(cases.size() >= 2);
        for (const auto &value : cases) {
            const auto record = value.toObject();
            if (record["name"] == "unsigned")
                continue;
            createArchive();
            QCOMPARE(signaturePolicy(true, fixtures["root"].toString()), S_OK);
            CMyComPtr<IInStream> input = new Input(image(record));
            QCOMPARE(archive_->Open(input, nullptr, nullptr), S_OK);
            CMyComPtr<IInArchiveGetStream> provider;
            QCOMPARE(archive_->QueryInterface(IID_IInArchiveGetStream,
                                              reinterpret_cast<void **>(&provider)),
                     S_OK);
            CMyComPtr<ISequentialInStream> stream;
            QCOMPARE(provider->GetStream(fileIndex(), &stream), S_OK);
            QVERIFY(stream);
            QCOMPARE(archive_->Close(), S_OK);
            QCOMPARE(signaturePolicy(false), S_OK);
            provider.Release();
            archive_.Release();
            input.Release();
            QByteArray actual(record["file_size"].toInt(), '\0');
            UInt32 offset = 0;
            while (offset < UInt32(actual.size())) {
                UInt32 count = 0;
                QCOMPARE(
                    stream->Read(actual.data() + offset, UInt32(actual.size()) - offset, &count),
                    S_OK);
                QVERIFY(count > 0);
                offset += count;
            }
            QCOMPARE(QCryptographicHash::hash(actual, QCryptographicHash::Sha256).toHex(),
                     record["sha256"].toString().toLatin1());
        }
        createArchive();
    }
    void signaturePolicyRejectsMissingForeignAndDamagedSignatures() {
        const auto fixtures = signatures();
        for (const QString policy :
             {QString(), fixtures["collision"].toString(), fixtures["root"].toString()}) {
            for (const auto &value : fixtures["cases"].toArray()) {
                const auto record = value.toObject();
                const bool unsignedFile = record["name"] == "unsigned";
                createArchive();
                QCOMPARE(signaturePolicy(true, policy), S_OK);
                auto bytes = image(record);
                if (!unsignedFile && policy == fixtures["root"].toString())
                    bytes[record["signature_offset"].toInt() + record["signature_size"].toInt() -
                          1] ^= 1;
                CMyComPtr<IInStream> input = new Input(bytes);
                QCOMPARE(archive_->Open(input, nullptr, nullptr), S_OK);
                CMyComPtr<IInArchiveGetStream> provider;
                QCOMPARE(archive_->QueryInterface(IID_IInArchiveGetStream,
                                                  reinterpret_cast<void **>(&provider)),
                         S_OK);
                CMyComPtr<ISequentialInStream> stream;
                QVERIFY(provider->GetStream(fileIndex(), &stream) != S_OK);
                QVERIFY(!stream);
            }
        }
    }
    void signedVerityConfigurationFailureAndReadCancellationCanBeRetried() {
        const auto fixtures = signatures();
        const auto record = fixtures["cases"].toArray().first().toObject();
        createArchive();
        QCOMPARE(signaturePolicy(true, fixtures["root"].toString()), S_OK);
        QCOMPARE(signaturePolicy(false, "not a DER certificate"), E_INVALIDARG);
        auto *source = new Input(image(record));
        CMyComPtr<IInStream> input = source;
        QCOMPARE(archive_->Open(input, nullptr, nullptr), S_OK);
        source->failAt = UInt64(record["signature_offset"].toDouble());
        source->atResult = E_ABORT;
        CMyComPtr<IInArchiveGetStream> provider;
        QCOMPARE(
            archive_->QueryInterface(IID_IInArchiveGetStream, reinterpret_cast<void **>(&provider)),
            S_OK);
        CMyComPtr<ISequentialInStream> stream;
        QCOMPARE(provider->GetStream(fileIndex(), &stream), E_ABORT);
        QVERIFY(!stream);
        source->failAt = UINT64_MAX;
        QCOMPARE(provider->GetStream(fileIndex(), &stream), S_OK);
        QVERIFY(stream);
    }
    void xattrStreamsOwnBytesAndHaveParents() {
        const auto record = bundle("xattrs");
        CMyComPtr<IInStream> input = new Input(image(record));
        QCOMPARE(archive_->Open(input, nullptr, nullptr), S_OK);
        CMyComPtr<IInArchiveGetStream> provider;
        QCOMPARE(
            archive_->QueryInterface(IID_IInArchiveGetStream, reinterpret_cast<void **>(&provider)),
            S_OK);
        CMyComPtr<IArchiveGetRawProps> raw;
        QCOMPARE(archive_->QueryInterface(IID_IArchiveGetRawProps, reinterpret_cast<void **>(&raw)),
                 S_OK);
        CMyComPtr<ISequentialInStream> value, empty;
        const UInt32 id = fileIndex("data.bin:$XATTR.user.large");
        QVERIFY(id != UInt32(-1));
        QCOMPARE(provider->GetStream(id, &value), S_OK);
        QCOMPARE(provider->GetStream(fileIndex("data.bin:$XATTR.user.empty"), &empty), S_OK);
        UInt32 parent = 0, type = 0;
        QCOMPARE(raw->GetParent(id, &parent, &type), S_OK);
        QCOMPARE(parent, fileIndex("data.bin"));
        QCOMPARE(type, UInt32(NParentType::kAltStream));
        NWindows::NCOM::CPropVariant property;
        QCOMPARE(archive_->GetProperty(id, kpidIsAltStream, &property), S_OK);
        QCOMPARE(property.vt, VARTYPE(VT_BOOL));
        QVERIFY(property.boolVal != VARIANT_FALSE);
        QCOMPARE(archive_->Close(), S_OK);
        input.Release();
        provider.Release();
        raw.Release();
        archive_.Release();
        const auto expected = QByteArray::fromHex(
            record["files"].toObject()["data.bin:$XATTR.user.large"].toString().toLatin1());
        QByteArray bytes(expected.size() + 1, '!');
        UInt32 actual = 0;
        QCOMPARE(value->Read(bytes.data(), UInt32(bytes.size()), &actual), S_OK);
        QCOMPARE(bytes.left(int(actual)), expected);
        QCOMPARE(empty->Read(bytes.data(), UInt32(bytes.size()), &actual), S_OK);
        QCOMPARE(actual, UInt32(0));
        createArchive();
    }
    void xattrReadFailureCancellationAndReuse() {
        const auto record = bundle("xattrs");
        CMyComPtr2<IInStream, Input> input;
        input.SetFromCls(new Input(image(record)));
        input->failAt = UInt64(record["ea_data_offset"].toDouble());
        for (HRESULT result : {E_FAIL, E_ABORT}) {
            QCOMPARE(input->Seek(0, STREAM_SEEK_SET, nullptr), S_OK);
            input->atResult = result;
            QCOMPARE(archive_->Open(input, nullptr, nullptr), result);
            UInt32 count = 99;
            QCOMPARE(archive_->GetNumberOfItems(&count), S_OK);
            QCOMPARE(count, UInt32(0));
        }
        input->notifyAt = input->failAt;
        input->failAt = UINT64_MAX;
        CMyComPtr2<IArchiveOpenCallback, CancelXattrOpen> cancel;
        cancel.SetFromCls(new CancelXattrOpen);
        cancel->source = input.ClsPtr();
        QCOMPARE(input->Seek(0, STREAM_SEEK_SET, nullptr), S_OK);
        QCOMPARE(archive_->Open(input, nullptr, cancel), E_ABORT);
        input->notifyAt = UINT64_MAX;
        input->notified = false;
        QCOMPARE(input->Seek(0, STREAM_SEEK_SET, nullptr), S_OK);
        QCOMPARE(archive_->Open(input, nullptr, cancel), S_OK);
        QVERIFY(fileIndex("data.bin:$XATTR.user.large") != UInt32(-1));
    }
    void sparseStreamsRetainSourceAfterCloseAndRelease_data() {
        QTest::addColumn<QString>("layout");
        for (const char *layout : {"double-hole", "double-subtree", "triple", "triple-hole",
                                   "extent-tree", "modern-inline", "verity-salted"})
            QTest::newRow(layout) << QString::fromLatin1(layout);
    }
    void sparseStreamsRetainSourceAfterCloseAndRelease() {
        QFETCH(QString, layout);
        const auto record = bundle(layout);
        QVERIFY(!record.isEmpty());
        const auto data = image(record);
        CMyComPtr<IInStream> input = new Input(data);
        QCOMPARE(archive_->Open(input, nullptr, nullptr), S_OK);
        const auto index = fileIndex();
        QVERIFY(index != UInt32(-1));
        CMyComPtr<IInArchiveGetStream> provider;
        QCOMPARE(
            archive_->QueryInterface(IID_IInArchiveGetStream, reinterpret_cast<void **>(&provider)),
            S_OK);
        CMyComPtr<ISequentialInStream> sequential;
        QCOMPARE(provider->GetStream(index, &sequential), S_OK);
        CMyComPtr<IInStream> stream;
        QCOMPARE(sequential->QueryInterface(IID_IInStream, reinterpret_cast<void **>(&stream)),
                 S_OK);
        QCOMPARE(archive_->Close(), S_OK);
        auto changed = layout == "modern-inline"   ? image(bundle("modern-inline-replacement"))
                       : layout == "verity-salted" ? image(bundle("direct"))
                                                   : data;
        const auto start = changed.indexOf("head");
        if (start >= 0)
            changed[start] = 'Z';
        CMyComPtr<IInStream> second = new Input(changed);
        QCOMPARE(archive_->Open(second, nullptr, nullptr), S_OK);
        provider.Release();
        archive_.Release();
        sequential.Release();
        input.Release();
        for (const auto &entry : record["samples"].toArray()) {
            const auto sample = entry.toObject();
            const auto expected = QByteArray::fromHex(sample["bytes"].toString().toLatin1());
            QCOMPARE(stream->Seek(qint64(sample["offset"].toDouble()), STREAM_SEEK_SET, nullptr),
                     S_OK);
            QByteArray actual;
            while (actual.size() < expected.size()) {
                char bytes[37];
                UInt32 count = 0;
                QCOMPARE(stream->Read(bytes, UInt32(expected.size() - actual.size()), &count),
                         S_OK);
                QVERIFY(count > 0);
                actual.append(bytes, count);
            }
            QCOMPARE(actual, expected);
        }
        createArchive();
    }
    void inputSeekFailuresPropagateThroughExtract_data() {
        QTest::addColumn<QString>("layout");
        QTest::newRow("direct-data-seek") << QString("direct");
        QTest::newRow("indirect-metadata-seek") << QString("double-hole");
    }
    void inputSeekFailuresPropagateThroughExtract() {
        QFETCH(QString, layout);
        const auto record = bundle(layout);
        const auto data = image(record);
        QVERIFY(!data.isEmpty());
        auto *source = new Input(data);
        CMyComPtr<IInStream> input = source;
        QCOMPARE(archive_->Open(input, nullptr, nullptr), S_OK);
        const UInt32 index = fileIndex();
        QVERIFY(index != UInt32(-1));
        CMyComPtr2<IArchiveExtractCallback, Extract> callback;
        callback.Create_if_Empty();
        for (const HRESULT error : {E_ABORT, E_FAIL}) {
            source->seekResult = error;
            QCOMPARE(archive_->Extract(&index, 1, 0, callback), error);
        }
        source->seekResult = S_OK;
        callback->cancel = true;
        QCOMPARE(archive_->Extract(&index, 1, 0, callback), E_ABORT);
        callback->cancel = false;
        callback->output->fail = true;
        QCOMPARE(archive_->Extract(&index, 1, 0, callback), E_FAIL);
        callback->output->fail = false;
        callback->output->bytes.clear();
        QCOMPARE(archive_->Extract(&index, 1, 0, callback), S_OK);
        QCOMPARE(callback->result, Int32(NArchive::NExtract::NOperationResult::kOK));
        const auto expectedSize = qint64(record["file_size"].toDouble());
        QCOMPARE(callback->output->bytes.size(), expectedSize);
        QCOMPARE(callback->output->bytes.left(4), QByteArray("head"));
        QCOMPARE(callback->output->bytes.right(4), QByteArray("tail"));
    }
    void failedAndCancelledOpenCanBeReused() {
        const auto data = image(bundle("direct"));
        CMyComPtr<IInStream> input = new Input(data);
        CMyComPtr<IArchiveOpenCallback> cancel = new CancelOpen;
        QCOMPARE(archive_->Open(input, nullptr, cancel), E_ABORT);
        UInt32 count = 99;
        QCOMPARE(archive_->GetNumberOfItems(&count), S_OK);
        QCOMPARE(count, 0u);
        auto *failed = new Input(data);
        input = failed;
        failed->fail = true;
        QCOMPARE(archive_->Open(input, nullptr, nullptr), E_FAIL);
        failed->fail = false;
        QCOMPARE(archive_->Open(input, nullptr, nullptr), S_OK);
        QVERIFY(fileIndex() != UInt32(-1));
    }
    void partialTimestampExtensionsAndEpochZero() {
        CMyComPtr<IInStream> input = new Input(image(bundle("modern-time")));
        QCOMPARE(archive_->Open(input, nullptr, nullptr), S_OK);
        const auto index = fileIndex();
        QVERIFY(index != UInt32(-1));
        const auto time = [&](PROPID id, UInt64 expected) {
            NWindows::NCOM::CPropVariant value;
            QCOMPARE(archive_->GetProperty(index, id, &value), S_OK);
            QCOMPARE(value.vt, VARTYPE(VT_FILETIME));
            const UInt64 ticks =
                (UInt64(value.filetime.dwHighDateTime) << 32) | value.filetime.dwLowDateTime;
            QCOMPARE(ticks, expected);
        };
        const UInt64 unixEpoch = UInt64(11644473600) * 10000000;
        time(kpidMTime, unixEpoch + (UInt64(1) << 32) * 10000000 + 1234567);
        time(kpidATime, unixEpoch);
        NWindows::NCOM::CPropVariant created;
        QCOMPARE(archive_->GetProperty(index, kpidCTime, &created), S_OK);
        QCOMPARE(created.vt, VARTYPE(VT_EMPTY));
    }
    void verityMetadataAndReadFailuresPropagateAndCanBeRetried() {
        auto *source = new Input(image(bundle("verity-salted")));
        CMyComPtr<IInStream> input = source;
        QCOMPARE(archive_->Open(input, nullptr, nullptr), S_OK);
        CMyComPtr<IInArchiveGetStream> provider;
        QCOMPARE(
            archive_->QueryInterface(IID_IInArchiveGetStream, reinterpret_cast<void **>(&provider)),
            S_OK);
        const auto index = fileIndex();
        QVERIFY(index != UInt32(-1));
        for (HRESULT error : {E_ABORT, E_FAIL}) {
            source->readResult = error;
            CMyComPtr<ISequentialInStream> failed;
            QCOMPARE(provider->GetStream(index, &failed), error);
            QVERIFY(!failed);
        }
        source->readResult = S_OK;
        CMyComPtr<ISequentialInStream> stream;
        QCOMPARE(provider->GetStream(index, &stream), S_OK);
        CMyComPtr<IInStream> seekable;
        QCOMPARE(stream->QueryInterface(IID_IInStream, reinterpret_cast<void **>(&seekable)), S_OK);
        QCOMPARE(seekable->Seek(2 * 1024, STREAM_SEEK_SET, nullptr), S_OK);
        for (HRESULT error : {E_ABORT, E_FAIL}) {
            source->readResult = error;
            char data[37];
            memset(data, '!', sizeof(data));
            UInt32 count = 99;
            QCOMPARE(stream->Read(data, sizeof(data), &count), error);
            QCOMPARE(count, 0u);
            QCOMPARE(QByteArray(data, sizeof(data)), QByteArray(sizeof(data), '!'));
        }
        source->readResult = S_OK;
        char data[37];
        UInt32 count = 0;
        QCOMPARE(stream->Read(data, sizeof(data), &count), S_OK);
        QCOMPARE(count, UInt32(sizeof(data)));
        QCOMPARE(QByteArray(data, sizeof(data)), QByteArray(sizeof(data), 'A'));
    }
    void corruptedVerityBlockIsNotPublishedToCaller() {
        CMyComPtr<IInStream> input = new Input(image(bundle("verity-corrupt")));
        QCOMPARE(archive_->Open(input, nullptr, nullptr), S_OK);
        CMyComPtr<IInArchiveGetStream> provider;
        QCOMPARE(
            archive_->QueryInterface(IID_IInArchiveGetStream, reinterpret_cast<void **>(&provider)),
            S_OK);
        CMyComPtr<ISequentialInStream> stream;
        QCOMPARE(provider->GetStream(fileIndex(), &stream), S_OK);
        char data[37];
        memset(data, '!', sizeof(data));
        UInt32 count = 99;
        QCOMPARE(stream->Read(data, sizeof(data), &count), E_FAIL);
        QCOMPARE(count, 0u);
        QCOMPARE(QByteArray(data, sizeof(data)), QByteArray(sizeof(data), '!'));
    }
    void cleanupTestCase() { archive_.Release(); }
};
QTEST_GUILESS_MAIN(ExtStreamTests)
#include "ext_stream_tests.moc"
