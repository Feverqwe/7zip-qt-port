#include "7zip/Archive/IArchive.h"
#include "Common/MyCom.h"
#include "Windows/PropVariant.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QLibrary>
#include <QProcess>
#include <QtEndian>
#include <QtTest>
#include <map>

namespace {
class Input final : public IInStream, public CMyUnknownImp {
    Z7_COM_UNKNOWN_IMP_2(ISequentialInStream, IInStream)
    QByteArray bytes_;
    UInt64 position_ = 0;

  public:
    UInt64 fault = UINT64_MAX, end = UINT64_MAX;
    HRESULT failure = E_FAIL;
    bool faultSeen = false;
    explicit Input(QByteArray bytes) : bytes_(std::move(bytes)) {}
    Z7_COM7F_IMF(Read(void *data, UInt32 size, UInt32 *processed)) override {
        if (processed)
            *processed = 0;
        if (size && position_ < end && (position_ >= fault || fault - position_ < size)) {
            faultSeen = true;
            return failure;
        }
        const UInt32 done = UInt32(
            qMin<UInt64>(size, position_ < UInt64(bytes_.size()) ? bytes_.size() - position_ : 0));
        if (done)
            memcpy(data, bytes_.constData() + position_, done);
        position_ += done;
        if (processed)
            *processed = done;
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
    QByteArray &bytes_;
    HRESULT failure_;

  public:
    Output(QByteArray &bytes, HRESULT failure) : bytes_(bytes), failure_(failure) {}
    Z7_COM7F_IMF(Write(const void *data, UInt32 size, UInt32 *processed)) override {
        if (processed)
            *processed = 0;
        if (failure_ != S_OK)
            return failure_;
        bytes_.append(static_cast<const char *>(data), size);
        if (processed)
            *processed = size;
        return S_OK;
    }
};
class Extract final : public IArchiveExtractCallback, public CMyUnknownImp {
    Z7_COM_UNKNOWN_IMP_1(IArchiveExtractCallback)
    UInt32 current_ = 0;

  public:
    std::map<UInt32, QByteArray> bytes;
    std::map<UInt32, Int32> results;
    HRESULT outputFailure = S_OK;
    Z7_COM7F_IMF(SetTotal(UInt64)) override { return S_OK; }
    Z7_COM7F_IMF(SetCompleted(const UInt64 *)) override { return S_OK; }
    Z7_COM7F_IMF(GetStream(UInt32 index, ISequentialOutStream **stream, Int32 mode)) override {
        current_ = index;
        *stream = nullptr;
        if (mode == NArchive::NExtract::NAskMode::kExtract) {
            CMyComPtr<ISequentialOutStream> output = new Output(bytes[index], outputFailure);
            *stream = output.Detach();
        }
        return S_OK;
    }
    Z7_COM7F_IMF(PrepareOperation(Int32)) override { return S_OK; }
    Z7_COM7F_IMF(SetOperationResult(Int32 result)) override {
        results[current_] = result;
        return S_OK;
    }
};
class Progress final : public IArchiveOpenCallback, public CMyUnknownImp {
    Z7_COM_UNKNOWN_IMP_1(IArchiveOpenCallback)
  public:
    Input *source = nullptr;
    HRESULT failure = E_ABORT;
    Z7_COM7F_IMF(SetTotal(const UInt64 *, const UInt64 *)) override { return S_OK; }
    Z7_COM7F_IMF(SetCompleted(const UInt64 *, const UInt64 *)) override {
        return source->faultSeen ? failure : S_OK;
    }
};
} // namespace

class ExtRecoveryTests final : public QObject {
    Q_OBJECT
    QLibrary module_{QString::fromUtf8(SEVENZIP_EXT_CORE)};
    CMyComPtr<IInArchive> archive_;
    QByteArray image_;
    QJsonObject fixture_;
    void fixture(const QString &kind) {
        QProcess generator;
        generator.start(QString::fromUtf8(SEVENZIP_EXT_PYTHON),
                        {QString::fromUtf8(SEVENZIP_EXT_RECOVERY_FIXTURE), kind});
        QVERIFY(generator.waitForFinished(10000));
        QVERIFY2(generator.exitCode() == 0, generator.readAllStandardError().constData());
        fixture_ = QJsonDocument::fromJson(generator.readAllStandardOutput()).object();
        QByteArray packed(4, '\0');
        qToBigEndian(quint32(fixture_["size"].toInt()), packed.data());
        packed += QByteArray::fromBase64(fixture_["image"].toString().toLatin1());
        image_ = qUncompress(packed);
        QVERIFY(!image_.isEmpty());
    }
    Input *source() const {
        auto *input = new Input(image_);
        if (fixture_["fault_end"].toInteger()) {
            input->fault = UInt64(fixture_["fault_offset"].toInteger());
            input->end = UInt64(fixture_["fault_end"].toInteger());
        }
        return input;
    }
    UInt32 index(UInt32 inode) const {
        UInt32 count = 0;
        archive_->GetNumberOfItems(&count);
        for (UInt32 i = 0; i < count; ++i) {
            NWindows::NCOM::CPropVariant value;
            archive_->GetProperty(i, kpidINode, &value);
            if (value.vt == VT_UI4 && value.ulVal == inode)
                return i;
        }
        return UINT32_MAX;
    }
    QString limitations() const {
        UInt32 count = 0;
        archive_->GetNumberOfArchiveProperties(&count);
        for (UInt32 i = 0; i < count; ++i) {
            CMyComBSTR name;
            PROPID id;
            VARTYPE type;
            archive_->GetArchivePropertyInfo(i, &name, &id, &type);
            if (name && QString::fromWCharArray(name) == "Read-only limitations") {
                NWindows::NCOM::CPropVariant value;
                archive_->GetArchiveProperty(id, &value);
                if (value.vt == VT_BSTR)
                    return QString::fromWCharArray(value.bstrVal);
            }
        }
        return {};
    }
    QByteArray expected(UInt32 inode) const {
        return QByteArray::fromHex(
            fixture_["files"].toObject()[QString::number(inode)].toString().toLatin1());
    }
  private slots:
    void initTestCase() {
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
    void readableSubset_data() {
        QTest::addColumn<QString>("kind");
        for (const QString &kind : {"control",
                                    "short-tail",
                                    "allocation-blocks",
                                    "allocation-inodes",
                                    "allocation-inodes-overflow",
                                    "block-bitmap",
                                    "inode-bitmap",
                                    "block-bitmap-io",
                                    "descriptor-sibling-io",
                                    "descriptor-root-io",
                                    "descriptor-table-range",
                                    "bitmap-address",
                                    "bitmap-overlap",
                                    "root-directory-io",
                                    "root-inode",
                                    "descriptor-checksum",
                                    "root-block-uninit",
                                    "root-inode-uninit",
                                    "inode-bitmap-checksum",
                                    "root-directory-checksum"})
            QTest::newRow(qPrintable(kind)) << kind;
    }
    void readableSubset() {
        QFETCH(QString, kind);
        fixture(kind);
        CMyComPtr<IInStream> input = source();
        QCOMPARE(archive_->Open(input, nullptr, nullptr), S_OK);
        QCOMPARE(limitations().isEmpty(), kind == "control");
        const UInt32 inode =
            kind == "descriptor-root-io" || kind == "descriptor-table-range" ? 28 : 12;
        const UInt32 selected = index(inode);
        QVERIFY(selected != UINT32_MAX);
        CMyComPtr2<IArchiveExtractCallback, Extract> callback;
        callback.Create_if_Empty();
        const HRESULT result = archive_->Extract(&selected, 1, 0, callback);
        QVERIFY(result == S_OK || result == S_FALSE);
        QCOMPARE(callback->results[selected], Int32(NArchive::NExtract::NOperationResult::kOK));
        QCOMPARE(callback->bytes[selected], expected(inode));
        if (kind.startsWith("descriptor-") || kind.startsWith("bitmap-") ||
            kind.contains("bitmap") || kind.endsWith("-uninit") || kind.startsWith("allocation-")) {
            NWindows::NCOM::CPropVariant free;
            QCOMPARE(archive_->GetArchiveProperty(kpidFreeSpace, &free), S_OK);
            QCOMPARE(free.vt, VARTYPE(VT_EMPTY));
        }
        fixture("control");
        input = source();
        QCOMPARE(archive_->Open(input, nullptr, nullptr), S_OK);
        QVERIFY(limitations().isEmpty());
    }
    void dataFailuresRetainOtherFiles_data() {
        QTest::addColumn<QString>("kind");
        QTest::newRow("short-file") << QString("short-file");
        QTest::newRow("unsafe-file-mapping") << QString("file-mapping");
        QTest::newRow("overlapping-metadata-remains-protected") << QString("overlap-file-mapping");
    }
    void dataFailuresRetainOtherFiles() {
        QFETCH(QString, kind);
        fixture(kind);
        CMyComPtr<IInStream> input = source();
        QCOMPARE(archive_->Open(input, nullptr, nullptr), S_OK);
        const UInt32 bad = index(kind == "short-file" ? 28 : 12);
        const UInt32 good = index(kind == "short-file" ? 12 : 28);
        QVERIFY(bad != UINT32_MAX && good != UINT32_MAX);
        const UInt32 selected[] = {bad, good};
        CMyComPtr2<IArchiveExtractCallback, Extract> callback;
        callback.Create_if_Empty();
        QCOMPARE(archive_->Extract(selected, 2, 0, callback), S_OK);
        QCOMPARE(callback->results[bad], Int32(NArchive::NExtract::NOperationResult::kDataError));
        QCOMPARE(callback->results[good], Int32(NArchive::NExtract::NOperationResult::kOK));
        QCOMPARE(callback->bytes[bad], kind == "short-file" ? expected(28).left(7) : QByteArray());
        QCOMPARE(callback->bytes[good], expected(kind == "short-file" ? 12 : 28));
        callback->outputFailure = E_FAIL;
        QCOMPARE(archive_->Extract(&good, 1, 0, callback), E_FAIL);
    }
    void fullCopyReportsIncompleteRoot() {
        fixture("root-directory-io");
        CMyComPtr<IInStream> input = source();
        QCOMPARE(archive_->Open(input, nullptr, nullptr), S_OK);
        for (bool explicitIndices : {false, true}) {
            UInt32 count = 0;
            QCOMPARE(archive_->GetNumberOfItems(&count), S_OK);
            QVector<UInt32> indices;
            for (UInt32 i = 0; i < count; ++i)
                indices.append(i);
            CMyComPtr2<IArchiveExtractCallback, Extract> callback;
            callback.Create_if_Empty();
            QCOMPARE(archive_->Extract(explicitIndices ? indices.constData() : nullptr,
                                       explicitIndices ? count : UInt32(-1), 0, callback),
                     S_FALSE);
            QCOMPARE(callback->bytes[index(12)], expected(12));
            QCOMPARE(callback->bytes[index(28)], expected(28));
        }
    }
    void unavailableRootCannotSucceedAsEmpty() {
        fixture("descriptor-all-io");
        CMyComPtr<IInStream> input = source();
        QCOMPARE(archive_->Open(input, nullptr, nullptr), S_FALSE);
        UInt32 count = 99;
        QCOMPARE(archive_->GetNumberOfItems(&count), S_OK);
        QCOMPARE(count, UInt32(0));
        NWindows::NCOM::CPropVariant error;
        QCOMPARE(archive_->GetArchiveProperty(kpidError, &error), S_OK);
        QCOMPARE(error.vt, VARTYPE(VT_BSTR));
        QVERIFY(::SysStringLen(error.bstrVal));
        fixture("control");
        input = source();
        QCOMPARE(archive_->Open(input, nullptr, nullptr), S_OK);
        QVERIFY(limitations().isEmpty());
    }
    void fatalFailuresCannotAuthorizeRecovery() {
        fixture("block-bitmap-io");
        for (HRESULT failure : {E_ABORT, E_OUTOFMEMORY}) {
            Input *stream = source();
            stream->failure = failure;
            CMyComPtr<IInStream> input = stream;
            QCOMPARE(archive_->Open(input, nullptr, nullptr), failure);
        }
        for (HRESULT failure : {E_ABORT, E_FAIL, E_OUTOFMEMORY}) {
            Input *stream = source();
            CMyComPtr<IInStream> input = stream;
            CMyComPtr2<IArchiveOpenCallback, Progress> callback;
            callback.Create_if_Empty();
            callback->source = stream;
            callback->failure = failure;
            QCOMPARE(archive_->Open(input, nullptr, callback), failure);
        }
    }
    void cleanupTestCase() { archive_.Release(); }
};
QTEST_GUILESS_MAIN(ExtRecoveryTests)
#include "ext_recovery_tests.moc"
