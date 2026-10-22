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
    UInt64 failUntil = 0;
    HRESULT atResult = E_FAIL;
    UInt32 maxRead = UINT32_MAX;
    UInt64 seekFailAt = UINT64_MAX;
    UInt64 seekFailUntil = 0;
    bool partialFailure = false;
    unsigned faults = 0;
    bool notified = false;
    explicit Input(QByteArray bytes) : bytes_(std::move(bytes)) {}
    Z7_COM7F_IMF(Read(void *data, UInt32 size, UInt32 *processed)) override {
        if (processed)
            *processed = 0;
        size = qMin(size, maxRead);
        HRESULT result = S_OK;
        if (size && failAt != UINT64_MAX && position_ < qMax(failUntil, failAt + 1) &&
            (position_ >= failAt || failAt - position_ < size)) {
            ++faults;
            if (!partialFailure || position_ >= failAt)
                return atResult;
            size = UInt32(failAt - position_);
            result = atResult;
        }
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
        return result;
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
        if (UInt64(base + offset) == seekFailAt ||
            (UInt64(base + offset) >= seekFailAt && UInt64(base + offset) < seekFailUntil)) {
            ++faults;
            return atResult;
        }
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
    HRESULT failure = E_FAIL;
    Z7_COM7F_IMF(Write(const void *data, UInt32 size, UInt32 *processed)) override {
        if (processed)
            *processed = 0;
        if (fail)
            return failure;
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
    bool separateFiles = false;
    QVector<Int32> results;
    QVector<QByteArray> files;
    HRESULT progressFailureAfterItem = S_OK;
    Extract() { output.Create_if_Empty(); }
    Z7_COM7F_IMF(SetTotal(UInt64)) override { return S_OK; }
    Z7_COM7F_IMF(SetCompleted(const UInt64 *)) override {
        return cancel ? E_ABORT : results.isEmpty() ? S_OK : progressFailureAfterItem;
    }
    Z7_COM7F_IMF(GetStream(UInt32, ISequentialOutStream **stream, Int32 mode)) override {
        *stream = nullptr;
        if (separateFiles)
            output->bytes.clear();
        if (mode == NArchive::NExtract::NAskMode::kExtract) {
            *stream = output.Interface();
            (*stream)->AddRef();
        }
        return S_OK;
    }
    Z7_COM7F_IMF(PrepareOperation(Int32)) override { return S_OK; }
    Z7_COM7F_IMF(SetOperationResult(Int32 value)) override {
        result = value;
        results.append(value);
        files.append(output->bytes);
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
    HRESULT result = E_ABORT;
    Z7_COM7F_IMF(SetTotal(const UInt64 *, const UInt64 *)) override { return S_OK; }
    Z7_COM7F_IMF(SetCompleted(const UInt64 *, const UInt64 *)) override {
        return source && source->notified ? result : S_OK;
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
    QString readLimitations() {
        UInt32 count = 0;
        if (archive_->GetNumberOfArchiveProperties(&count) != S_OK)
            return {};
        for (UInt32 i = 0; i < count; ++i) {
            BSTR name = nullptr;
            PROPID id = 0;
            VARTYPE type = VT_EMPTY;
            if (archive_->GetArchivePropertyInfo(i, &name, &id, &type) != S_OK)
                return {};
            const QString label = name ? QString::fromWCharArray(name) : QString();
            SysFreeString(name);
            if (label == "Read-only limitations") {
                NWindows::NCOM::CPropVariant value;
                if (archive_->GetArchiveProperty(id, &value) == S_OK && value.vt == VT_BSTR)
                    return QString::fromWCharArray(value.bstrVal);
            }
        }
        return {};
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
        for (const char *key : {"ea_data_offset", "xattr_block_offset"}) {
            const UInt64 offset = UInt64(record[key].toDouble());
            QVERIFY(offset > 0);
            for (bool seek : {false, true}) {
                input->failAt = seek ? UINT64_MAX : offset;
                input->seekFailAt = seek ? offset : UINT64_MAX;
                for (HRESULT result : {E_FAIL, E_ABORT, E_OUTOFMEMORY}) {
                    input->faults = 0;
                    QCOMPARE(input->Seek(0, STREAM_SEEK_SET, nullptr), S_OK);
                    input->atResult = result;
                    QCOMPARE(archive_->Open(input, nullptr, nullptr),
                             result == E_FAIL ? S_OK : result);
                    UInt32 count = 99;
                    QCOMPARE(archive_->GetNumberOfItems(&count), S_OK);
                    if (result == E_FAIL) {
                        QVERIFY(input->faults > 0);
                        QVERIFY(!readLimitations().isEmpty());
                        QCOMPARE(fileIndex(QString(key) == "ea_data_offset"
                                               ? "data.bin:$XATTR.user.large"
                                               : "data.bin:$XATTR.system.posix_acl_access"),
                                 UInt32(-1));
                        QVERIFY(fileIndex("data.bin:$XATTR.user.empty") != UInt32(-1));
                        const UInt32 index = fileIndex("data.bin");
                        QVERIFY(index != UInt32(-1));
                        NWindows::NCOM::CPropVariant diagnostic;
                        QCOMPARE(archive_->GetProperty(index, kpidError, &diagnostic), S_OK);
                        QCOMPARE(diagnostic.vt, VARTYPE(VT_BSTR));
                        CMyComPtr2<IArchiveExtractCallback, Extract> extract;
                        extract.SetFromCls(new Extract);
                        QCOMPARE(archive_->Extract(&index, 1, false, extract), S_OK);
                        QCOMPARE(extract->files.size(), 1);
                        QByteArray expected;
                        for (int b = 0; b < 100; ++b)
                            expected += char(b);
                        QCOMPARE(extract->files[0], expected);
                        QCOMPARE(extract->results,
                                 QList<Int32>{NArchive::NExtract::NOperationResult::kOK});
                    } else
                        QCOMPARE(count, UInt32(0));
                }
            }
        }
        input->notifyAt = UInt64(record["ea_data_offset"].toDouble());
        input->failAt = UINT64_MAX;
        input->seekFailAt = UINT64_MAX;
        CMyComPtr2<IArchiveOpenCallback, CancelXattrOpen> cancel;
        cancel.SetFromCls(new CancelXattrOpen);
        cancel->source = input.ClsPtr();
        for (HRESULT result : {E_ABORT, E_FAIL, S_FALSE, E_OUTOFMEMORY}) {
            cancel->result = result;
            input->notified = false;
            QCOMPARE(input->Seek(0, STREAM_SEEK_SET, nullptr), S_OK);
            QCOMPARE(archive_->Open(input, nullptr, cancel), result);
            QVERIFY(input->notified);
        }
        input->notifyAt = UINT64_MAX;
        input->notified = false;
        QCOMPARE(input->Seek(0, STREAM_SEEK_SET, nullptr), S_OK);
        QCOMPARE(archive_->Open(input, nullptr, cancel), S_OK);
        QVERIFY(readLimitations().isEmpty());
        QVERIFY(fileIndex("data.bin:$XATTR.user.large") != UInt32(-1));
    }
    void wideIndirectStreamsReadRangesWithoutDenseAllocation_data() {
        QTest::addColumn<QString>("layout");
        QTest::newRow("32-bit last logical block") << QString("indirect-wide");
        QTest::newRow("oversize does not wrap to the first block")
            << QString("indirect-wide-oversize");
    }
    void wideIndirectStreamsReadRangesWithoutDenseAllocation() {
        QFETCH(QString, layout);
        const auto record = bundle(layout);
        CMyComPtr<IInStream> input = new Input(image(record));
        QCOMPARE(archive_->Open(input, nullptr, nullptr), S_OK);
        const UInt32 index = fileIndex();
        QVERIFY(index != UInt32(-1));
        QCOMPARE(!readLimitations().isEmpty(), layout.endsWith("oversize"));
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
        CMyComPtr<IInStream> other = new Input(image(bundle("direct")));
        QCOMPARE(archive_->Open(other, nullptr, nullptr), S_OK);
        provider.Release();
        archive_.Release();
        input.Release();
        const UInt64 blockSize = UInt64(1) << 16;
        for (const auto &sample : QVector<QPair<UInt64, QByteArray>>{
                 {0, QByteArray("head")},
                 {(UInt64(1) << 31) * blockSize, QByteArray(37, '\0')},
                 {UInt64(UINT32_MAX) * blockSize, QByteArray("last indirect block")},
                 {0, QByteArray("head")}}) {
            QCOMPARE(stream->Seek(Int64(sample.first), STREAM_SEEK_SET, nullptr), S_OK);
            QByteArray bytes(sample.second.size(), '\0');
            UInt32 done = 0;
            QCOMPARE(stream->Read(bytes.data(), UInt32(bytes.size()), &done), S_OK);
            QCOMPARE(done, UInt32(bytes.size()));
            QCOMPARE(bytes, sample.second);
        }
        if (layout.endsWith("oversize")) {
            QCOMPARE(stream->Seek(Int64((UInt64(1) << 32) * blockSize), STREAM_SEEK_SET, nullptr),
                     S_OK);
            char byte = 0;
            UInt32 done = 99;
            QCOMPARE(stream->Read(&byte, 1, &done), S_FALSE);
            QCOMPARE(done, UInt32(0));
        }
        createArchive();
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
    void inputSeekFailuresAreLocalDuringExtract_data() {
        QTest::addColumn<QString>("layout");
        QTest::newRow("direct-data-seek") << QString("direct");
        QTest::newRow("indirect-metadata-seek") << QString("double-hole");
    }
    void mappedReadsRetryFailedSeeks_data() {
        QTest::addColumn<QString>("layout");
        for (const char *layout : {"direct", "single", "extent-tree"})
            QTest::newRow(layout) << QString::fromLatin1(layout);
    }
    void mappedReadsRetryFailedSeeks() {
        QFETCH(QString, layout);
        const auto record = bundle(layout);
        auto *source = new Input(image(record));
        CMyComPtr<IInStream> input = source;
        QCOMPARE(archive_->Open(input, nullptr, nullptr), S_OK);
        CMyComPtr<IInArchiveGetStream> provider;
        QCOMPARE(
            archive_->QueryInterface(IID_IInArchiveGetStream, reinterpret_cast<void **>(&provider)),
            S_OK);
        CMyComPtr<ISequentialInStream> stream;
        QCOMPARE(provider->GetStream(fileIndex(), &stream), S_OK);
        CMyComPtr<IInStream> seekable;
        QCOMPARE(stream->QueryInterface(IID_IInStream, reinterpret_cast<void **>(&seekable)), S_OK);
        for (HRESULT error : {E_FAIL, E_ABORT, E_OUTOFMEMORY}) {
            QCOMPARE(seekable->Seek(record["file_size"].toInt() - 4, STREAM_SEEK_SET, nullptr),
                     S_OK);
            source->seekResult = error;
            char bytes[4] = {};
            UInt32 done = 99;
            QCOMPARE(stream->Read(bytes, sizeof(bytes), &done), error);
            QCOMPARE(done, UInt32(0));
            source->seekResult = S_OK;
            QCOMPARE(stream->Read(bytes, sizeof(bytes), &done), S_OK);
            QCOMPARE(done, UInt32(4));
            QCOMPARE(QByteArray(bytes, 4), QByteArray("tail"));
        }
    }
    void inputSeekFailuresAreLocalDuringExtract() {
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
        for (const HRESULT error : {E_ABORT, E_OUTOFMEMORY, E_FAIL}) {
            source->seekResult = error;
            QCOMPARE(archive_->Extract(&index, 1, 0, callback), error == E_FAIL ? S_OK : error);
            if (error == E_FAIL)
                QCOMPARE(callback->result, Int32(NArchive::NExtract::NOperationResult::kDataError));
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
    void fileSourceFailuresKeepHealthySiblings_data() {
        QTest::addColumn<QString>("layout");
        for (const char *layout :
             {"direct", "single", "double-hole", "double-subtree", "extent-tree",
              "extent-two-branches", "extent-two-branches-full-prefix", "verity", "verity-tree",
              "symlink"})
            QTest::newRow(layout) << QString::fromLatin1(layout);
    }
    void fileSourceFailuresKeepHealthySiblings() {
        QFETCH(QString, layout);
        const auto record = bundle("extract-" + layout);
        const QByteArray bytes = image(record);
        QVERIFY(!bytes.isEmpty());
        const QByteArray expected = QByteArray::fromHex(record["payload"].toString().toLatin1());
        const QByteArray healthy = QByteArray::fromHex(record["healthy"].toString().toLatin1());
        QVERIFY(!expected.isEmpty());
        QVERIFY(!healthy.isEmpty());
        auto *source = new Input(bytes);
        CMyComPtr<IInStream> input = source;
        QCOMPARE(archive_->Open(input, nullptr, nullptr), S_OK);
        const UInt32 indices[] = {fileIndex(), fileIndex("healthy space.bin")};
        QVERIFY(indices[0] != UInt32(-1));
        QVERIFY(indices[1] != UInt32(-1));
        CMyComPtr<IInArchiveGetStream> provider;
        QCOMPARE(
            archive_->QueryInterface(IID_IInArchiveGetStream, reinterpret_cast<void **>(&provider)),
            S_OK);
        for (bool metadata : {false, true}) {
            const UInt64 target =
                UInt64(record[metadata ? "failure_metadata" : "failure_data"].toDouble());
            QVERIFY(target > 0);
            for (bool seek : {false, true}) {
                const UInt64 seekTarget =
                    metadata ? UInt64(record["failure_metadata_seek"].toDouble()) : target;
                for (HRESULT error : {E_FAIL, S_FALSE, S_OK, E_ABORT, E_OUTOFMEMORY}) {
                    if (seek && error == S_OK)
                        continue;
                    for (bool testOnly : {false, true}) {
                        source->maxRead = 37;
                        source->failAt = seek ? UINT64_MAX : target + (metadata ? 0 : 48);
                        source->seekFailAt = seek ? seekTarget : UINT64_MAX;
                        source->atResult = error;
                        CMyComPtr2<IArchiveExtractCallback, Extract> callback;
                        callback.Create_if_Empty();
                        callback->separateFiles = true;
                        const bool fatal = error == E_ABORT || error == E_OUTOFMEMORY;
                        const unsigned faults = source->faults;
                        QCOMPARE(archive_->Extract(indices, 2, testOnly, callback),
                                 fatal ? error : S_OK);
                        QVERIFY2(source->faults > faults,
                                 qPrintable(QString("Unreached fault: metadata=%1 seek=%2 test=%3 "
                                                    "error=%4 address=%5")
                                                .arg(metadata)
                                                .arg(seek)
                                                .arg(testOnly)
                                                .arg(quint32(error), 0, 16)
                                                .arg(seek ? seekTarget : source->failAt)));
                        if (fatal) {
                            QVERIFY(callback->results.isEmpty());
                        } else {
                            QCOMPARE(
                                callback->results,
                                QVector<Int32>({NArchive::NExtract::NOperationResult::kDataError,
                                                NArchive::NExtract::NOperationResult::kOK}));
                            QCOMPARE(callback->files[1], testOnly ? QByteArray() : healthy);
                            if (!testOnly) {
                                QVERIFY(expected.startsWith(callback->files[0]));
                                QVERIFY(callback->files[0].size() <= expected.size());
                                if (metadata)
                                    QCOMPARE(callback->files[0],
                                             expected.left(record["prefix_size"].toInt()));
                                else if (layout == "symlink")
                                    QVERIFY(callback->files[0].isEmpty());
                            }
                        }
                    }
                }
            }
        }
        source->maxRead = UINT32_MAX;
        source->failAt = source->seekFailAt = UINT64_MAX;
        CMyComPtr2<IArchiveExtractCallback, Extract> retry;
        retry.Create_if_Empty();
        retry->separateFiles = true;
        QCOMPARE(archive_->Extract(indices, 2, 0, retry), S_OK);
        QCOMPARE(retry->results, QVector<Int32>({NArchive::NExtract::NOperationResult::kOK,
                                                 NArchive::NExtract::NOperationResult::kOK}));
        QCOMPARE(retry->files, QVector<QByteArray>({expected, healthy}));
        source->seekResult = E_FAIL;
        CMyComPtr<ISequentialInStream> failed;
        HRESULT failure = provider->GetStream(indices[0], &failed);
        if (failure == S_OK) {
            QVERIFY(failed);
            char byte = 0;
            UInt32 done = 99;
            failure = failed->Read(&byte, 1, &done);
            QCOMPARE(done, UInt32(0));
        }
        QCOMPARE(failure, E_FAIL);
        source->seekResult = S_OK;
    }
    void incompleteExtentMappingsDoNotInventHoles_data() {
        QTest::addColumn<QString>("layout");
        QTest::newRow("first-child-invalid") << QString("extent-tree");
        QTest::newRow("later-child-invalid") << QString("extent-two-branches");
        QTest::newRow("verity-metadata-invalid") << QString("verity-tree");
    }
    void incompleteExtentMappingsDoNotInventHoles() {
        QFETCH(QString, layout);
        const auto record = bundle("extract-" + layout);
        auto damaged = image(record);
        const auto offset = qsizetype(record["failure_metadata"].toDouble());
        QVERIFY(offset > 0 && offset < damaged.size());
        damaged[offset] ^= 1;
        CMyComPtr<IInStream> input = new Input(damaged);
        QCOMPARE(archive_->Open(input, nullptr, nullptr), S_OK);
        const UInt32 indices[] = {fileIndex(), fileIndex("healthy space.bin")};
        QVERIFY(indices[0] != UInt32(-1) && indices[1] != UInt32(-1));
        const QByteArray expected = QByteArray::fromHex(record["payload"].toString().toLatin1());
        const QByteArray healthy = QByteArray::fromHex(record["healthy"].toString().toLatin1());
        for (bool testOnly : {false, true}) {
            CMyComPtr2<IArchiveExtractCallback, Extract> callback;
            callback.Create_if_Empty();
            callback->separateFiles = true;
            QCOMPARE(archive_->Extract(indices, 2, testOnly, callback), S_OK);
            QCOMPARE(callback->results,
                     QVector<Int32>({NArchive::NExtract::NOperationResult::kDataError,
                                     NArchive::NExtract::NOperationResult::kOK}));
            QCOMPARE(callback->files[0],
                     testOnly ? QByteArray() : expected.left(record["prefix_size"].toInt()));
            QCOMPARE(callback->files[1], testOnly ? QByteArray() : healthy);
        }
        CMyComPtr<IInArchiveGetStream> provider;
        QCOMPARE(
            archive_->QueryInterface(IID_IInArchiveGetStream, reinterpret_cast<void **>(&provider)),
            S_OK);
        CMyComPtr<ISequentialInStream> stream;
        QCOMPARE(provider->GetStream(indices[0], &stream), S_FALSE);
        QVERIFY(!stream);
    }
    void extractionPrefixesPreserveConsumerFailures_data() {
        QTest::addColumn<QString>("layout");
        QTest::addColumn<bool>("metadata");
        QTest::newRow("direct") << QString("direct") << false;
        QTest::newRow("extent") << QString("extent-tree") << false;
        QTest::newRow("indirect-metadata") << QString("single") << true;
        QTest::newRow("extent-metadata") << QString("extent-two-branches") << true;
    }
    void extractionPrefixesPreserveConsumerFailures() {
        QFETCH(QString, layout);
        QFETCH(bool, metadata);
        const auto record = bundle("extract-" + layout);
        auto *source = new Input(image(record));
        CMyComPtr<IInStream> input = source;
        QCOMPARE(archive_->Open(input, nullptr, nullptr), S_OK);
        const UInt32 indices[] = {fileIndex(), fileIndex("healthy space.bin")};
        QVERIFY(indices[0] != UInt32(-1));
        QVERIFY(indices[1] != UInt32(-1));
        source->maxRead = 37;
        source->partialFailure = true;
        source->failAt = UInt64(record[metadata ? "failure_metadata" : "failure_data"].toDouble()) +
                         (metadata ? 0 : 48);
        const QByteArray expected = QByteArray::fromHex(record["payload"].toString().toLatin1());
        CMyComPtr2<IArchiveExtractCallback, Extract> prefix;
        prefix.Create_if_Empty();
        prefix->separateFiles = true;
        QCOMPARE(archive_->Extract(indices, 2, 0, prefix), S_OK);
        const qsizetype prefixSize =
            metadata ? record["prefix_size"].toInt() : (layout == "direct" ? 0 : 2048) + 48;
        QCOMPARE(prefix->files[0], expected.left(prefixSize));
        QCOMPARE(prefix->results, QVector<Int32>({NArchive::NExtract::NOperationResult::kDataError,
                                                  NArchive::NExtract::NOperationResult::kOK}));
        for (HRESULT error : {E_FAIL, S_FALSE, E_ABORT}) {
            CMyComPtr2<IArchiveExtractCallback, Extract> output;
            output.Create_if_Empty();
            output->output->fail = true;
            output->output->failure = error;
            QCOMPARE(archive_->Extract(indices, 2, 0, output), error);
            QVERIFY(output->results.isEmpty());
            CMyComPtr2<IArchiveExtractCallback, Extract> progress;
            progress.Create_if_Empty();
            progress->progressFailureAfterItem = error;
            QCOMPARE(archive_->Extract(indices, 2, 0, progress), error);
            QCOMPARE(progress->results,
                     QVector<Int32>({NArchive::NExtract::NOperationResult::kDataError}));
        }
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
    void symlinkReadsAreBoundedAndOwned() {
        const auto record = bundle("readlink-block-tail");
        CMyComPtr2<IInStream, Input> input;
        input.SetFromCls(new Input(image(record)));
        input->failAt = UInt64(record["second_data_offset"].toDouble());
        QCOMPARE(archive_->Open(input, nullptr, nullptr), S_OK);
        CMyComPtr<IInArchiveGetStream> provider;
        QCOMPARE(
            archive_->QueryInterface(IID_IInArchiveGetStream, reinterpret_cast<void **>(&provider)),
            S_OK);
        CMyComPtr<ISequentialInStream> target;
        QCOMPARE(provider->GetStream(fileIndex(), &target), S_OK);
        QCOMPARE(input->faults, 0u);
        QCOMPARE(archive_->Close(), S_OK);
        provider.Release();
        archive_.Release();
        input.SetFromCls(nullptr);
        QByteArray actual(record["file_size"].toInt(), '\0');
        UInt32 read = 0;
        QCOMPARE(target->Read(actual.data(), UInt32(actual.size()), &read), S_OK);
        QCOMPARE(read, UInt32(actual.size()));
        QCOMPARE(actual, QByteArray(actual.size(), 't'));
        createArchive();
    }
    void symlinkFailuresAreLocalAndRetryable_data() {
        QTest::addColumn<bool>("seek");
        QTest::addColumn<bool>("partial");
        QTest::addColumn<int>("failure");
        for (bool seek : {false, true})
            for (bool partial : {false, true}) {
                if (seek && partial)
                    continue;
                for (HRESULT failure : {E_FAIL, S_FALSE, E_ABORT, E_OUTOFMEMORY})
                    QTest::newRow(
                        qPrintable(QString("%1-%2-%3").arg(seek).arg(partial).arg(failure)))
                        << seek << partial << int(failure);
            }
    }
    void symlinkFailuresAreLocalAndRetryable() {
        QFETCH(bool, seek);
        QFETCH(bool, partial);
        QFETCH(int, failure);
        const auto record = bundle("readlink-isolated-block-tail");
        CMyComPtr2<IInStream, Input> input;
        input.SetFromCls(new Input(image(record)));
        const UInt64 offset = UInt64(record["first_data_offset"].toDouble());
        QVERIFY(offset > 0);
        input->failAt = seek ? UINT64_MAX : offset + (partial ? 123 : 0);
        input->seekFailAt = seek ? offset : UINT64_MAX;
        input->partialFailure = partial;
        input->atResult = HRESULT(failure);
        const bool fatal = HRESULT(failure) == E_ABORT || HRESULT(failure) == E_OUTOFMEMORY;
        QCOMPARE(archive_->Open(input, nullptr, nullptr), fatal ? HRESULT(failure) : S_OK);
        QVERIFY(input->faults > 0);
        if (fatal) {
            UInt32 count = 99;
            QCOMPARE(archive_->GetNumberOfItems(&count), S_OK);
            QCOMPARE(count, UInt32(0));
            return;
        }
        QVERIFY(!readLimitations().isEmpty());
        const UInt32 link = fileIndex(), healthy = fileIndex("healthy.bin"),
                     later = fileIndex("later-link");
        QVERIFY(link != UInt32(-1) && healthy != UInt32(-1) && later != UInt32(-1));
        NWindows::NCOM::CPropVariant value;
        QCOMPARE(archive_->GetProperty(link, kpidSymLink, &value), S_OK);
        QCOMPARE(value.vt, VARTYPE(VT_EMPTY));
        QCOMPARE(archive_->GetProperty(link, kpidSize, &value), S_OK);
        QCOMPARE(value.vt, VARTYPE(VT_EMPTY));
        QCOMPARE(archive_->GetProperty(link, kpidError, &value), S_OK);
        QCOMPARE(value.vt, VARTYPE(VT_BSTR));
        CMyComPtr<IInArchiveGetStream> provider;
        QCOMPARE(
            archive_->QueryInterface(IID_IInArchiveGetStream, reinterpret_cast<void **>(&provider)),
            S_OK);
        CMyComPtr<ISequentialInStream> target;
        QCOMPARE(provider->GetStream(link, &target), HRESULT(failure));
        QVERIFY(!target);
        const UInt32 indices[] = {link, healthy, later};
        CMyComPtr2<IArchiveExtractCallback, Extract> extract;
        extract.SetFromCls(new Extract);
        extract->separateFiles = true;
        QCOMPARE(archive_->Extract(indices, 3, false, extract), S_OK);
        QCOMPARE(extract->results, (QVector<Int32>{NArchive::NExtract::NOperationResult::kDataError,
                                                   NArchive::NExtract::NOperationResult::kOK,
                                                   NArchive::NExtract::NOperationResult::kOK}));
        QCOMPARE(extract->files,
                 (QVector<QByteArray>{QByteArray(),
                                      QByteArray::fromHex(record["healthy"].toString().toLatin1()),
                                      QByteArray("healthy.bin")}));
        input->failAt = input->seekFailAt = UINT64_MAX;
        extract.SetFromCls(new Extract);
        QCOMPARE(archive_->Extract(&link, 1, false, extract), S_OK);
        QCOMPARE(extract->result, Int32(NArchive::NExtract::NOperationResult::kOK));
        QCOMPARE(extract->output->bytes, QByteArray(record["file_size"].toInt(), 't'));
        QCOMPARE(input->Seek(0, STREAM_SEEK_SET, nullptr), S_OK);
        QCOMPARE(archive_->Open(input, nullptr, nullptr), S_OK);
        QVERIFY(readLimitations().isEmpty());
    }
    void symlinkCallbacksRemainFatal() {
        const auto record = bundle("readlink-isolated-block-tail");
        CMyComPtr2<IInStream, Input> input;
        input.SetFromCls(new Input(image(record)));
        input->notifyAt = UInt64(record["first_data_offset"].toDouble());
        CMyComPtr2<IArchiveOpenCallback, CancelXattrOpen> callback;
        callback.SetFromCls(new CancelXattrOpen);
        callback->source = input.ClsPtr();
        for (HRESULT failure : {E_ABORT, E_FAIL, S_FALSE, E_OUTOFMEMORY}) {
            callback->result = failure;
            input->notified = false;
            QCOMPARE(input->Seek(0, STREAM_SEEK_SET, nullptr), S_OK);
            QCOMPARE(archive_->Open(input, nullptr, callback), failure);
            QVERIFY(input->notified);
            UInt32 count = 99;
            QCOMPARE(archive_->GetNumberOfItems(&count), S_OK);
            QCOMPARE(count, UInt32(0));
        }
    }
    void inodeTableReadFailuresAreLocal_data() { symlinkFailuresAreLocalAndRetryable_data(); }
    void inodeTableReadFailuresAreLocal() {
        QFETCH(bool, seek);
        QFETCH(bool, partial);
        QFETCH(int, failure);
        const auto record = bundle("inode-inventory-control");
        CMyComPtr2<IInStream, Input> input;
        input.SetFromCls(new Input(image(record)));
        const UInt64 page = UInt64(record["inode_page_offset"].toDouble());
        QVERIFY(page > UInt64(record["root_inode_page_offset"].toDouble()));
        input->failAt = seek ? UINT64_MAX : page + (partial ? 123 : 0);
        input->seekFailAt = seek ? page : UINT64_MAX;
        input->failUntil = input->seekFailUntil = page + UInt64(record["inode_page_size"].toInt());
        input->partialFailure = partial;
        input->atResult = HRESULT(failure);
        const bool fatal = HRESULT(failure) == E_ABORT || HRESULT(failure) == E_OUTOFMEMORY;
        QCOMPARE(archive_->Open(input, nullptr, nullptr), fatal ? HRESULT(failure) : S_OK);
        QVERIFY(input->faults > 0);
        if (fatal) {
            UInt32 count = 99;
            QCOMPARE(archive_->GetNumberOfItems(&count), S_OK);
            QCOMPARE(count, UInt32(0));
            return;
        }
        QVERIFY(!readLimitations().isEmpty());
        QCOMPARE(fileIndex(), UInt32(-1));
        const UInt32 indices[] = {fileIndex("healthy.bin"), fileIndex("later-link")};
        QVERIFY(indices[0] != UInt32(-1) && indices[1] != UInt32(-1));
        CMyComPtr2<IArchiveExtractCallback, Extract> extract;
        extract.SetFromCls(new Extract);
        extract->separateFiles = true;
        QCOMPARE(archive_->Extract(indices, 2, false, extract), S_OK);
        QCOMPARE(extract->results, (QVector<Int32>{NArchive::NExtract::NOperationResult::kOK,
                                                   NArchive::NExtract::NOperationResult::kOK}));
        QCOMPARE(extract->files,
                 (QVector<QByteArray>{QByteArray::fromHex(record["healthy"].toString().toLatin1()),
                                      QByteArray("healthy.bin")}));
        input->failAt = input->seekFailAt = UINT64_MAX;
        QCOMPARE(input->Seek(0, STREAM_SEEK_SET, nullptr), S_OK);
        QCOMPARE(archive_->Open(input, nullptr, nullptr), S_OK);
        QVERIFY(readLimitations().isEmpty());
        QVERIFY(fileIndex() != UInt32(-1));
    }
    void mandatoryRootInodeAndInventoryCallbacksRemainFatal() {
        const auto record = bundle("inode-inventory-control");
        CMyComPtr2<IInStream, Input> input;
        input.SetFromCls(new Input(image(record)));
        input->failAt = UInt64(record["root_inode_page_offset"].toDouble());
        input->failUntil = input->failAt + UInt64(record["inode_page_size"].toInt());
        QVERIFY(input->failAt > 0);
        for (HRESULT result : {E_FAIL, E_ABORT, E_OUTOFMEMORY}) {
            input->atResult = result;
            QCOMPARE(input->Seek(0, STREAM_SEEK_SET, nullptr), S_OK);
            QCOMPARE(archive_->Open(input, nullptr, nullptr), result == E_FAIL ? S_FALSE : result);
            UInt32 count = 99;
            QCOMPARE(archive_->GetNumberOfItems(&count), S_OK);
            QCOMPARE(count, UInt32(0));
        }
        input->failAt = UINT64_MAX;
        input->notifyAt = UInt64(record["inode_page_offset"].toDouble());
        CMyComPtr2<IArchiveOpenCallback, CancelXattrOpen> callback;
        callback.SetFromCls(new CancelXattrOpen);
        callback->source = input.ClsPtr();
        for (HRESULT result : {E_ABORT, E_FAIL, S_FALSE, E_OUTOFMEMORY}) {
            callback->result = result;
            input->notified = false;
            QCOMPARE(input->Seek(0, STREAM_SEEK_SET, nullptr), S_OK);
            QCOMPARE(archive_->Open(input, nullptr, callback), result);
            QVERIFY(input->notified);
            UInt32 count = 99;
            QCOMPARE(archive_->GetNumberOfItems(&count), S_OK);
            QCOMPARE(count, UInt32(0));
        }
    }
    void inodeRetriesKeepCompleteRecordsFromFailedBlocks_data() {
        QTest::addColumn<bool>("root");
        QTest::newRow("root survives a different unreadable record") << true;
        QTest::newRow("file survives an unreadable following record") << false;
    }
    void inodeRetriesKeepCompleteRecordsFromFailedBlocks() {
        QFETCH(bool, root);
        const auto record = bundle("inode-inventory-control");
        const auto bytes = image(record);
        const UInt64 inodeSize = qFromLittleEndian<quint16>(bytes.constData() + 1024 + 88);
        CMyComPtr2<IInStream, Input> input;
        input.SetFromCls(new Input(bytes));
        const UInt64 pageSize = UInt64(record["inode_page_size"].toInt());
        const UInt64 page = root ? UInt64(record["root_inode_page_offset"].toDouble())
                                 : UInt64(record["inode_page_offset"].toDouble()) + pageSize;
        // Fail a different record in the root's block, or the record after
        // healthy.bin in the final block. Whole-block I/O fails; a complete
        // read/checksum of the root or healthy file's inode remains possible.
        input->failAt = root ? page : page + inodeSize;
        input->failUntil = root ? page + inodeSize : page + pageSize;
        input->partialFailure = true;
        QCOMPARE(archive_->Open(input, nullptr, nullptr), S_OK);
        QVERIFY(input->faults > 0);
        QVERIFY(!readLimitations().isEmpty());
        QCOMPARE(fileIndex("later-link") == UInt32(-1), !root);
        QVERIFY(fileIndex() != UInt32(-1));
        const UInt32 index = fileIndex("healthy.bin");
        QVERIFY(index != UInt32(-1));
        CMyComPtr2<IArchiveExtractCallback, Extract> extract;
        extract.SetFromCls(new Extract);
        QCOMPARE(archive_->Extract(&index, 1, false, extract), S_OK);
        QCOMPARE(extract->result, Int32(NArchive::NExtract::NOperationResult::kOK));
        QCOMPARE(extract->output->bytes,
                 QByteArray::fromHex(record["healthy"].toString().toLatin1()));
    }
    void htreeIndexReadsPreserveErrorsCancellationAndReuse() {
        for (const char *layout : {"htree-depth2", "htree-next-index"}) {
            const auto record = bundle(layout);
            CMyComPtr2<IInStream, Input> input;
            input.SetFromCls(new Input(image(record)));
            const UInt64 index = UInt64(record["next_index_offset"].toDouble());
            input->failAt = index;
            UInt32 count = 99;
            for (HRESULT error : {E_FAIL, E_ABORT, E_OUTOFMEMORY}) {
                input->atResult = error;
                QCOMPARE(input->Seek(0, STREAM_SEEK_SET, nullptr), S_OK);
                QCOMPARE(archive_->Open(input, nullptr, nullptr), error == E_FAIL ? S_OK : error);
                QCOMPARE(archive_->GetNumberOfItems(&count), S_OK);
                if (error == E_FAIL) {
                    QVERIFY(!readLimitations().isEmpty());
                    QVERIFY(fileIndex() != UInt32(-1));
                } else
                    QCOMPARE(count, UInt32(0));
            }
            input->failAt = UINT64_MAX;
            input->notifyAt = index;
            CMyComPtr2<IArchiveOpenCallback, CancelXattrOpen> cancel;
            cancel.SetFromCls(new CancelXattrOpen);
            cancel->source = input.ClsPtr();
            QCOMPARE(input->Seek(0, STREAM_SEEK_SET, nullptr), S_OK);
            QCOMPARE(archive_->Open(input, nullptr, cancel), E_ABORT);
            QCOMPARE(archive_->GetNumberOfItems(&count), S_OK);
            QCOMPARE(count, UInt32(0));
            auto damaged = image(record);
            damaged[qsizetype(index) + 1020] ^= 1;
            CMyComPtr<IInStream> corrupt = new Input(damaged);
            QCOMPARE(archive_->Open(corrupt, nullptr, nullptr), S_OK);
            QCOMPARE(archive_->GetNumberOfItems(&count), S_OK);
            QVERIFY(!readLimitations().isEmpty());
            QVERIFY(fileIndex() != UInt32(-1));
            input->notifyAt = UINT64_MAX;
            input->notified = false;
            QCOMPARE(input->Seek(0, STREAM_SEEK_SET, nullptr), S_OK);
            QCOMPARE(archive_->Open(input, nullptr, cancel), S_OK);
            QVERIFY(readLimitations().isEmpty());
            QVERIFY(fileIndex() != UInt32(-1));
        }
    }
    void blockBitmapReadErrorsPreserveFailureAndReuse() {
        const auto record = bundle("direct");
        CMyComPtr2<IInStream, Input> input;
        input.SetFromCls(new Input(image(record)));
        input->failAt = UInt64(record["block_bitmap_offset"].toDouble());
        UInt32 count = 99;
        for (HRESULT error : {E_FAIL, E_ABORT}) {
            input->atResult = error;
            QCOMPARE(input->Seek(0, STREAM_SEEK_SET, nullptr), S_OK);
            QCOMPARE(archive_->Open(input, nullptr, nullptr), error);
            QCOMPARE(archive_->GetNumberOfItems(&count), S_OK);
            QCOMPARE(count, UInt32(0));
        }
        input->failAt = UINT64_MAX;
        QCOMPARE(input->Seek(0, STREAM_SEEK_SET, nullptr), S_OK);
        QCOMPARE(archive_->Open(input, nullptr, nullptr), S_OK);
        QVERIFY(fileIndex() != UInt32(-1));
    }
    void directoryBlocksPreserveErrorsCancellationAndReuse() {
        const auto record = bundle("directory-stream");
        CMyComPtr2<IInStream, Input> input;
        input.SetFromCls(new Input(image(record)));
        input->failAt = UInt64(record["directory_tail_offset"].toDouble());
        UInt32 count = 99;
        for (HRESULT error : {E_FAIL, E_ABORT, E_OUTOFMEMORY}) {
            input->atResult = error;
            QCOMPARE(input->Seek(0, STREAM_SEEK_SET, nullptr), S_OK);
            QCOMPARE(archive_->Open(input, nullptr, nullptr), error == E_FAIL ? S_OK : error);
            QCOMPARE(archive_->GetNumberOfItems(&count), S_OK);
            if (error == E_FAIL) {
                QVERIFY(!readLimitations().isEmpty());
                QVERIFY(fileIndex() != UInt32(-1));
            } else
                QCOMPARE(count, UInt32(0));
        }
        input->failAt = UINT64_MAX;
        input->notifyAt = UInt64(record["directory_head_offset"].toDouble());
        CMyComPtr2<IArchiveOpenCallback, CancelXattrOpen> cancel;
        cancel.SetFromCls(new CancelXattrOpen);
        cancel->source = input.ClsPtr();
        QCOMPARE(input->Seek(0, STREAM_SEEK_SET, nullptr), S_OK);
        QCOMPARE(archive_->Open(input, nullptr, cancel), E_ABORT);
        QCOMPARE(archive_->GetNumberOfItems(&count), S_OK);
        QCOMPARE(count, UInt32(0));
        CMyComPtr<IInStream> corrupt = new Input(image(bundle("directory-corrupt")));
        QCOMPARE(archive_->Open(corrupt, nullptr, nullptr), S_OK);
        QCOMPARE(archive_->GetNumberOfItems(&count), S_OK);
        QVERIFY(!readLimitations().isEmpty());
        QVERIFY(fileIndex() != UInt32(-1));
        input->notifyAt = UINT64_MAX;
        input->notified = false;
        QCOMPARE(input->Seek(0, STREAM_SEEK_SET, nullptr), S_OK);
        QCOMPARE(archive_->Open(input, nullptr, cancel), S_OK);
        QVERIFY(readLimitations().isEmpty());
        QVERIFY(fileIndex() != UInt32(-1));
        QVERIFY(fileIndex("after-hole.bin") != UInt32(-1));
    }
    void directoryFaultsKeepOtherBlocksAndFatalCallbacks_data() {
        QTest::addColumn<bool>("head");
        QTest::addColumn<QString>("layout");
        QTest::addColumn<bool>("root");
        QTest::newRow("first-directory-block") << true << QString("directory-stream") << false;
        QTest::newRow("last-directory-block") << false << QString("directory-stream") << false;
        QTest::newRow("first-child-block") << true << QString("directory-namespace") << false;
        QTest::newRow("last-child-block") << false << QString("directory-namespace") << false;
        QTest::newRow("unknown-parent-name") << true << QString("directory-namespace") << true;
    }
    void directoryFaultsKeepOtherBlocksAndFatalCallbacks() {
        QFETCH(bool, head);
        QFETCH(QString, layout);
        QFETCH(bool, root);
        const auto record = bundle(layout);
        CMyComPtr2<IInStream, Input> input;
        input.SetFromCls(new Input(image(record)));
        QCOMPARE(archive_->Open(input, nullptr, nullptr), S_OK);
        UInt32 selected = fileIndex();
        QVERIFY(selected != UInt32(-1));
        CMyComPtr2<IArchiveExtractCallback, Extract> baseline;
        baseline.Create_if_Empty();
        QCOMPARE(archive_->Extract(&selected, 1, 0, baseline), S_OK);
        const QByteArray expected = baseline->output->bytes;
        QCOMPARE(expected.size(), record["file_size"].toInt());
        for (const auto &value : record["samples"].toArray()) {
            const auto sample = value.toObject();
            const auto bytes = QByteArray::fromHex(sample["bytes"].toString().toLatin1());
            QCOMPARE(expected.mid(sample["offset"].toInt(), bytes.size()), bytes);
        }
        const UInt64 target = UInt64(record[root   ? "root_directory_offset"
                                            : head ? "directory_head_offset"
                                                   : "directory_tail_offset"]
                                         .toDouble());
        for (bool seek : {false, true}) {
            for (HRESULT error : {E_FAIL, S_FALSE, S_OK, E_ABORT, E_OUTOFMEMORY}) {
                if (seek && error == S_OK)
                    continue;
                input->failAt = seek ? UINT64_MAX : target;
                input->seekFailAt = seek ? target : UINT64_MAX;
                input->atResult = error;
                QCOMPARE(input->Seek(0, STREAM_SEEK_SET, nullptr), S_OK);
                const unsigned faults = input->faults;
                const bool fatal = error == E_ABORT || error == E_OUTOFMEMORY;
                QCOMPARE(archive_->Open(input, nullptr, nullptr), fatal ? error : S_OK);
                QVERIFY(input->faults > faults);
                if (fatal)
                    continue;
                QVERIFY(!readLimitations().isEmpty());
                UInt32 incompleteDirectory = UInt32(-1);
                if (layout == "directory-namespace") {
                    if (!root)
                        selected = fileIndex(head ? "DIRECTORY/after-error.bin"
                                                  : "DIRECTORY/before-error.bin");
                    else {
                        selected = UInt32(-1);
                        UInt32 count = 0;
                        QCOMPARE(archive_->GetNumberOfItems(&count), S_OK);
                        for (UInt32 i = 0; i < count; ++i) {
                            NWindows::NCOM::CPropVariant value;
                            QCOMPARE(archive_->GetProperty(i, kpidPath, &value), S_OK);
                            if (value.vt == VT_BSTR &&
                                QDir::fromNativeSeparators(QString::fromWCharArray(value.bstrVal))
                                    .endsWith("/after-error.bin"))
                                selected = i;
                        }
                    }
                    UInt32 parent = UInt32(-1), type = 0;
                    CMyComPtr<IArchiveGetRawProps> tree;
                    QCOMPARE(archive_->QueryInterface(IID_IArchiveGetRawProps,
                                                      reinterpret_cast<void **>(&tree)),
                             S_OK);
                    QCOMPARE(tree->GetParent(selected, &parent, &type), S_OK);
                    QVERIFY(parent != UInt32(-1));
                    incompleteDirectory = parent;
                    NWindows::NCOM::CPropVariant error;
                    QCOMPARE(archive_->GetProperty(parent, kpidError, &error), S_OK);
                    QCOMPARE(error.vt, VARTYPE(VT_BSTR));
                    QVERIFY(SysStringLen(error.bstrVal) > 0);
                } else
                    selected = head ? fileIndex("after-hole.bin") : fileIndex();
                QVERIFY(selected != UInt32(-1));
                CMyComPtr2<IArchiveExtractCallback, Extract> copy;
                copy.Create_if_Empty();
                QCOMPARE(archive_->Extract(&selected, 1, 0, copy), S_OK);
                QCOMPARE(copy->result, Int32(NArchive::NExtract::NOperationResult::kOK));
                QCOMPARE(copy->output->bytes, expected);
                if (incompleteDirectory != UInt32(-1)) {
                    const UInt32 indices[] = {incompleteDirectory, selected};
                    CMyComPtr2<IArchiveExtractCallback, Extract> sequence;
                    sequence.Create_if_Empty();
                    sequence->separateFiles = true;
                    QCOMPARE(archive_->Extract(indices, 2, 0, sequence), S_OK);
                    QCOMPARE(sequence->results,
                             QVector<Int32>({NArchive::NExtract::NOperationResult::kDataError,
                                             NArchive::NExtract::NOperationResult::kOK}));
                    QCOMPARE(sequence->files[1], expected);
                }
            }
        }
        input->failAt = input->seekFailAt = UINT64_MAX;
        input->notifyAt = target;
        CMyComPtr2<IArchiveOpenCallback, CancelXattrOpen> callback;
        callback.SetFromCls(new CancelXattrOpen);
        callback->source = input.ClsPtr();
        for (HRESULT error : {E_FAIL, S_FALSE, E_ABORT, E_OUTOFMEMORY}) {
            input->notified = false;
            callback->result = error;
            QCOMPARE(input->Seek(0, STREAM_SEEK_SET, nullptr), S_OK);
            QCOMPARE(archive_->Open(input, nullptr, callback), error);
        }
        input->notifyAt = UINT64_MAX;
        input->notified = false;
        QCOMPARE(input->Seek(0, STREAM_SEEK_SET, nullptr), S_OK);
        QCOMPARE(archive_->Open(input, nullptr, nullptr), S_OK);
        QVERIFY(readLimitations().isEmpty());
        if (layout == "directory-namespace") {
            const UInt32 directory = fileIndex("DIRECTORY");
            QVERIFY(directory != UInt32(-1));
            NWindows::NCOM::CPropVariant error;
            QCOMPARE(archive_->GetProperty(directory, kpidError, &error), S_OK);
            QCOMPARE(error.vt, VARTYPE(VT_EMPTY));
        }
    }
    void unreadableDirectoryTargetDoesNotHideFollowingRecords() {
        const auto record = bundle("directory-invalid-target");
        CMyComPtr<IInStream> input = new Input(image(record));
        QCOMPARE(archive_->Open(input, nullptr, nullptr), S_OK);
        QVERIFY(!readLimitations().isEmpty());
        QCOMPARE(fileIndex("DIRECTORY/unreadable.bin"), UInt32(-1));
        const UInt32 indices[] = {fileIndex("DIRECTORY/before-error.bin"),
                                  fileIndex("DIRECTORY/after-bad-inode.bin")};
        QVERIFY(indices[0] != UInt32(-1) && indices[1] != UInt32(-1));
        QByteArray expected(record["file_size"].toInt(), '\0');
        expected.replace(0, 4, "head");
        expected.replace(expected.size() - 4, 4, "tail");
        CMyComPtr2<IArchiveExtractCallback, Extract> callback;
        callback.Create_if_Empty();
        callback->separateFiles = true;
        QCOMPARE(archive_->Extract(indices, 2, 0, callback), S_OK);
        QCOMPARE(callback->results, QVector<Int32>({NArchive::NExtract::NOperationResult::kOK,
                                                    NArchive::NExtract::NOperationResult::kOK}));
        QCOMPARE(callback->files, QVector<QByteArray>({expected, expected}));
    }
    void indirectJournalOwnership_data() {
        QTest::addColumn<QString>("layout");
        for (const char *mapping : {"single", "double-hole", "triple"})
            QTest::newRow(mapping) << QStringLiteral("journal-") + QString::fromLatin1(mapping);
    }
    void indirectJournalOwnership() {
        QFETCH(QString, layout);
        const auto record = bundle(layout);
        CMyComPtr<IInStream> input = new Input(image(record));
        QCOMPARE(archive_->Open(input, nullptr, nullptr), S_OK);
        CMyComPtr<IInArchiveGetStream> getter;
        QCOMPARE(
            archive_->QueryInterface(IID_IInArchiveGetStream, reinterpret_cast<void **>(&getter)),
            S_OK);
        const UInt32 index = fileIndex("[SYS]/Journal");
        QVERIFY(index != UInt32(-1));
        CMyComPtr<ISequentialInStream> sequential;
        QCOMPARE(getter->GetStream(index, &sequential), S_OK);
        CMyComPtr<IInStream> stream;
        QCOMPARE(sequential->QueryInterface(IID_IInStream, reinterpret_cast<void **>(&stream)),
                 S_OK);
        for (const auto &value : record["journal_samples"].toArray()) {
            const auto sample = value.toObject();
            const auto expected = QByteArray::fromHex(sample["bytes"].toString().toLatin1());
            QCOMPARE(stream->Seek(sample["offset"].toInteger(), STREAM_SEEK_SET, nullptr), S_OK);
            QByteArray actual(expected.size(), '\0');
            UInt32 total = 0;
            while (total < UInt32(actual.size())) {
                UInt32 done = 0;
                QCOMPARE(stream->Read(actual.data() + total, UInt32(actual.size()) - total, &done),
                         S_OK);
                QVERIFY(done > 0);
                total += done;
            }
            QCOMPARE(actual, expected);
        }
        QVERIFY(readLimitations().isEmpty());
    }
    void indirectJournalFailuresAndSourceErrors() {
        UInt32 count = 99;
        for (const char *fault : {"alias", "data-bound", "index-bound", "overlap"}) {
            const auto record =
                bundle(QStringLiteral("journal-single:") + QString::fromLatin1(fault));
            CMyComPtr2<IInStream, Input> input;
            const auto bytes = image(record);
            QVERIFY(!bytes.isEmpty());
            input.SetFromCls(new Input(bytes));
            input->failAt = UInt64(bytes.size());
            QCOMPARE(archive_->Open(input, nullptr, nullptr), S_FALSE);
            QCOMPARE(input->faults, 0U);
            QCOMPARE(archive_->GetNumberOfItems(&count), S_OK);
            QCOMPARE(count, UInt32(0));
        }
        const auto record = bundle("journal-single");
        CMyComPtr2<IInStream, Input> input;
        input.SetFromCls(new Input(image(record)));
        const auto metadata = UInt64(record["journal_metadata"].toInteger());
        QVERIFY(metadata > 0);
        for (bool seek : {false, true}) {
            for (HRESULT error : {S_FALSE, E_FAIL, E_ABORT, E_OUTOFMEMORY}) {
                input->failAt = seek ? UINT64_MAX : metadata;
                input->seekFailAt = seek ? metadata : UINT64_MAX;
                input->atResult = error;
                input->faults = 0;
                QCOMPARE(input->Seek(0, STREAM_SEEK_SET, nullptr), S_OK);
                QCOMPARE(archive_->Open(input, nullptr, nullptr), error);
                QVERIFY(input->faults > 0);
                QCOMPARE(archive_->GetNumberOfItems(&count), S_OK);
                QCOMPARE(count, UInt32(0));
            }
        }
        input->failAt = input->seekFailAt = UINT64_MAX;
        input->notifyAt = metadata;
        input->notified = false;
        CMyComPtr2<IArchiveOpenCallback, CancelXattrOpen> cancel;
        cancel.SetFromCls(new CancelXattrOpen);
        cancel->source = input.ClsPtr();
        QCOMPARE(input->Seek(0, STREAM_SEEK_SET, nullptr), S_OK);
        QCOMPARE(archive_->Open(input, nullptr, cancel), E_ABORT);
        input->notifyAt = UINT64_MAX;
        input->notified = false;
        QCOMPARE(input->Seek(0, STREAM_SEEK_SET, nullptr), S_OK);
        QCOMPARE(archive_->Open(input, nullptr, nullptr), S_OK);
        QVERIFY(fileIndex("[SYS]/Journal") != UInt32(-1));
    }
    void systemZoneFailureAndJournalCancellationCanBeReused() {
        UInt32 count = 99;
        for (const char *layout : {"block-journal", "block-overlap"}) {
            CMyComPtr<IInStream> corrupt = new Input(image(bundle(layout)));
            QCOMPARE(archive_->Open(corrupt, nullptr, nullptr), S_FALSE);
            QCOMPARE(archive_->GetNumberOfItems(&count), S_OK);
            QCOMPARE(count, UInt32(0));
        }
        CMyComPtr2<IInStream, Input> input;
        input.SetFromCls(new Input(image(bundle("block-journal-owner"))));
        // Cancellation after inode-table I/O also covers reserved-inode setup.
        input->notifyAt = 5 * 1024;
        CMyComPtr2<IArchiveOpenCallback, CancelXattrOpen> cancel;
        cancel.SetFromCls(new CancelXattrOpen);
        cancel->source = input.ClsPtr();
        QCOMPARE(archive_->Open(input, nullptr, cancel), E_ABORT);
        QCOMPARE(archive_->GetNumberOfItems(&count), S_OK);
        QCOMPARE(count, UInt32(0));
        input->notifyAt = UINT64_MAX;
        input->notified = false;
        QCOMPARE(input->Seek(0, STREAM_SEEK_SET, nullptr), S_OK);
        QCOMPARE(archive_->Open(input, nullptr, cancel), S_OK);
        QVERIFY(fileIndex("[SYS]/Journal") != UInt32(-1));
    }
    void partialTimestampExtensionsAndEpochZero_data() {
        QTest::addColumn<QString>("layout");
        QTest::addColumn<quint64>("accessEpochs");
        QTest::newRow("partial extension") << QStringLiteral("modern-time") << quint64(0);
        QTest::newRow("zero extension initialized by Linux")
            << QStringLiteral("modern-time-zero") << quint64(1);
    }
    void partialTimestampExtensionsAndEpochZero() {
        QFETCH(QString, layout);
        QFETCH(quint64, accessEpochs);
        CMyComPtr<IInStream> input = new Input(image(bundle(layout)));
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
        time(kpidATime, unixEpoch + accessEpochs * (UInt64(1) << 32) * 10000000);
        NWindows::NCOM::CPropVariant created;
        QCOMPARE(archive_->GetProperty(index, kpidCTime, &created), S_OK);
        if (accessEpochs) {
            time(kpidCTime, unixEpoch + (UInt64(2) << 32) * 10000000 + 2345678);
        } else {
            QCOMPARE(created.vt, VARTYPE(VT_EMPTY));
        }
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
