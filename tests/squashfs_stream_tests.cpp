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
#include <QProcess>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QtEndian>
#include <QtTest>
#include <algorithm>

namespace {
class Input final : public IInStream, public CMyUnknownImp {
    Z7_COM_UNKNOWN_IMP_2(ISequentialInStream, IInStream)
    QByteArray bytes_;
    UInt64 position_ = 0;

  public:
    HRESULT failure = S_OK;
    UInt64 failAt = UINT64_MAX, notifyAt = UINT64_MAX;
    HRESULT atFailure = E_FAIL;
    bool notified = false;
    explicit Input(QByteArray bytes) : bytes_(std::move(bytes)) {}
    Z7_COM7F_IMF(Read(void *data, UInt32 size, UInt32 *processed)) override {
        if (processed)
            *processed = 0;
        if (failure != S_OK)
            return failure;
        if (failAt >= position_ && failAt - position_ < size)
            return atFailure;
        if (notifyAt >= position_ && notifyAt - position_ < size)
            notified = true;
        size = UInt32(qMin<UInt64>(
            size, position_ < UInt64(bytes_.size()) ? UInt64(bytes_.size()) - position_ : 0));
        if (size)
            memcpy(data, bytes_.constData() + position_, size);
        position_ += size;
        if (processed)
            *processed = size;
        return S_OK;
    }
    Z7_COM7F_IMF(Seek(Int64 offset, UInt32 origin, UInt64 *position)) override {
        if (failure != S_OK)
            return failure;
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
class MetadataOpen final : public IArchiveOpenCallback, public CMyUnknownImp {
    Z7_COM_UNKNOWN_IMP_1(IArchiveOpenCallback)
  public:
    const Input *source = nullptr;
    HRESULT completionResult = E_ABORT;
    Z7_COM7F_IMF(SetTotal(const UInt64 *, const UInt64 *)) override { return S_OK; }
    Z7_COM7F_IMF(SetCompleted(const UInt64 *, const UInt64 *)) override {
        return source && source->notified ? completionResult : S_OK;
    }
};
class Extract final : public IArchiveExtractCallback, public CMyUnknownImp {
    Z7_COM_UNKNOWN_IMP_1(IArchiveExtractCallback)
  public:
    bool cancel = false;
    bool capture = false, cancelAfterOutput = false;
    UInt64 completed = 0;
    CMyComPtr2<ISequentialOutStream, Output> output;
    Extract() { output.Create_if_Empty(); }
    Int32 result = -1;
    Z7_COM7F_IMF(SetTotal(UInt64)) override { return S_OK; }
    Z7_COM7F_IMF(SetCompleted(const UInt64 *value)) override {
        if (value)
            completed = *value;
        return cancel || (cancelAfterOutput && !output->bytes.isEmpty()) ? E_ABORT : S_OK;
    }
    Z7_COM7F_IMF(GetStream(UInt32, ISequentialOutStream **stream, Int32)) override {
        *stream = nullptr;
        if (capture) {
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
} // namespace

class SquashfsStreamTests final : public QObject {
    Q_OBJECT
    QLibrary module_{QString::fromUtf8(SEVENZIP_SQUASHFS_CORE)};
    CMyComPtr<IInArchive> archive_;
    QJsonObject record_;
    QByteArray bytes_;

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
            if (name.vt != VT_BSTR || QString::fromWCharArray(name.bstrVal) != "SquashFS")
                continue;
            NWindows::NCOM::CPropVariant clsid;
            QCOMPARE(property(i, NArchive::NHandlerPropID::kClassID, &clsid), S_OK);
            GUID guid;
            memcpy(&guid, clsid.bstrVal, sizeof(guid));
            QCOMPARE(create(&guid, &IID_IInArchive, reinterpret_cast<void **>(&archive_)), S_OK);
            return;
        }
        QFAIL("Prepared engine has no SquashFS handler");
    }
    UInt32 index(const QString &name) {
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
    CMyComPtr<IInStream> stream(const QString &name) {
        CMyComPtr<IInArchiveGetStream> provider;
        if (archive_->QueryInterface(IID_IInArchiveGetStream,
                                     reinterpret_cast<void **>(&provider)) != S_OK)
            return {};
        CMyComPtr<ISequentialInStream> sequential;
        if (provider->GetStream(index(name), &sequential) != S_OK || !sequential)
            return {};
        CMyComPtr<IInStream> result;
        sequential->QueryInterface(IID_IInStream, reinterpret_cast<void **>(&result));
        return result;
    }
    void check(IInStream *stream, const QString &name, int offset, int size) {
        const auto expected =
            QByteArray::fromBase64(record_["files"].toObject()[name].toString().toLatin1())
                .mid(offset, size);
        QCOMPARE(stream->Seek(offset, STREAM_SEEK_SET, nullptr), S_OK);
        QByteArray actual;
        while (actual.size() < expected.size()) {
            char data[37];
            UInt32 count = 0;
            QCOMPARE(
                stream->Read(data,
                             UInt32(qMin(qsizetype(sizeof(data)), expected.size() - actual.size())),
                             &count),
                S_OK);
            QVERIFY(count > 0);
            actual.append(data, count);
        }
        QCOMPARE(actual, expected);
    }
    void loadFixture(const QString &codec, bool xattrs = false, const QString &fault = {}) {
        if (codec == "native") {
            const QDir fixtures(QFileInfo(QString::fromUtf8(SEVENZIP_SQUASHFS_FIXTURE))
                                    .dir()
                                    .filePath("fixtures/squashfs"));
            QFile manifest(fixtures.filePath("manifest.json"));
            QVERIFY(manifest.open(QIODevice::ReadOnly));
            record_ = QJsonDocument::fromJson(manifest.readAll())
                          .object()["images"]
                          .toObject()["xattr-values"]
                          .toObject();
            QFile file(fixtures.filePath("xattr-values.squashfs.zlib"));
            QVERIFY(file.open(QIODevice::ReadOnly));
            QByteArray packed(4, '\0');
            qToBigEndian(quint32(record_["size"].toInt()), packed.data());
            packed += file.readAll();
            bytes_ = qUncompress(packed);
            QVERIFY(!bytes_.isEmpty());
            return;
        }
        QProcess generator;
        QStringList arguments{QString::fromUtf8(SEVENZIP_SQUASHFS_FIXTURE)};
        arguments << (codec == "zlib" && !xattrs && fault.isEmpty() ? "--compressed"
                                                                    : "--codec=" + codec);
        if (xattrs) {
            arguments << "--xattrs";
            if (fault != "inode-count-large")
                arguments << "--export";
        }
        if (!fault.isEmpty())
            arguments << "--fault=" + fault;
        generator.start(QString::fromUtf8(SEVENZIP_SQUASHFS_PYTHON), arguments);
        QVERIFY(generator.waitForFinished(10000));
        QCOMPARE(generator.exitCode(), 0);
        record_ = QJsonDocument::fromJson(generator.readAllStandardOutput()).object();
        QVERIFY(!record_.isEmpty());
        QByteArray packed(4, '\0');
        qToBigEndian(quint32(record_["size"].toInt()), packed.data());
        packed += QByteArray::fromBase64(record_["image"].toString().toLatin1());
        bytes_ = qUncompress(packed);
        QVERIFY(!bytes_.isEmpty());
    }
    static void codecs() {
        QTest::addColumn<QString>("codec");
        for (const char *codec : {"zlib", "lz4", "lzo-rle"})
            QTest::newRow(codec) << QString::fromLatin1(codec);
    }
  private slots:
    void initTestCase() { createArchive(); }
    void inodePropertiesDescribeHardlinksAndSymlinks() {
        loadFixture("zlib", true);
        CMyComPtr<IInStream> source = new Input(bytes_);
        QCOMPARE(archive_->Open(source, nullptr, nullptr), S_OK);
        for (const QString &name : {QString("alpha"), QString("hardlink")}) {
            NWindows::NCOM::CPropVariant inode, links, streamId;
            QCOMPARE(archive_->GetProperty(index(name), kpidINode, &inode), S_OK);
            QCOMPARE(inode.vt, VARTYPE(VT_UI4));
            QCOMPARE(inode.ulVal, 1u);
            QCOMPARE(archive_->GetProperty(index(name), kpidLinks, &links), S_OK);
            QCOMPARE(links.ulVal, 2u);
            QCOMPARE(archive_->GetProperty(index(name), kpidStreamId, &streamId), S_OK);
            QCOMPARE(streamId.ulVal, inode.ulVal);
        }
        NWindows::NCOM::CPropVariant target;
        QCOMPARE(archive_->GetProperty(index("link"), kpidSymLink, &target), S_OK);
        QCOMPARE(target.vt, VARTYPE(VT_BSTR));
        QCOMPARE(QString::fromWCharArray(target.bstrVal), QString("alpha"));
        NWindows::NCOM::CPropVariant error;
        QCOMPARE(archive_->GetProperty(index("link"), kpidError, &error), S_OK);
        QCOMPARE(error.vt, VARTYPE(VT_EMPTY));
    }
    void unreadableExportPayloadDoesNotBlockFiles_data() {
        QTest::addColumn<HRESULT>("failure");
        QTest::newRow("export-read-error") << HRESULT(E_FAIL);
        QTest::newRow("export-read-aborted") << HRESULT(E_ABORT);
    }
    void partialDirectoriesKeepKnownEntries_data() {
        QTest::addColumn<QString>("fault");
        for (const QString &fault :
             {"directory-reference", "directory-unavailable", "directory-ambiguous",
              "directory-tail", "cycle", "name-slash", "count-wrap"})
            QTest::newRow(qPrintable(fault)) << fault;
    }
    void partialDirectoriesKeepKnownEntries() {
        QFETCH(QString, fault);
        loadFixture("zlib", true, fault);
        CMyComPtr<IInStream> source = new Input(bytes_);
        QCOMPARE(archive_->Open(source, nullptr, nullptr), S_OK);
        const UInt32 root = index("[ROOT]");
        QVERIFY(root != UInt32(-1));
        NWindows::NCOM::CPropVariant rootError;
        QCOMPARE(archive_->GetProperty(root, kpidError, &rootError), S_OK);
        QVERIFY(rootError.vt == VT_BSTR && rootError.bstrVal[0]);
        CMyComPtr2<IArchiveExtractCallback, Extract> diagnostic;
        diagnostic.SetFromCls(new Extract);
        QCOMPARE(archive_->Extract(&root, 1, 1, diagnostic), S_OK);
        QCOMPARE(diagnostic->result, Int32(NArchive::NExtract::NOperationResult::kDataError));
        if (fault == "name-slash" || fault == "count-wrap") {
            QCOMPARE(index("alpha"), UInt32(-1));
            QCOMPARE(index("hardlink"), UInt32(-1));
        } else {
            if (fault == "directory-reference") {
                const UInt32 recovered = index("alpha");
                auto data = stream("alpha");
                QVERIFY(data);
                check(data, "alpha", 0, 8202);
                NWindows::NCOM::CPropVariant error, identity;
                QCOMPARE(archive_->GetProperty(recovered, kpidError, &error), S_OK);
                QVERIFY(error.vt == VT_BSTR && error.bstrVal[0]);
                QCOMPARE(archive_->GetProperty(recovered, kpidStreamId, &identity), S_OK);
                QCOMPARE(identity.vt, VARTYPE(VT_EMPTY));
                diagnostic->capture = true;
                QCOMPARE(archive_->Extract(&recovered, 1, 0, diagnostic), S_OK);
                QCOMPARE(diagnostic->result,
                         Int32(NArchive::NExtract::NOperationResult::kDataError));
                QCOMPARE(diagnostic->output->bytes,
                         QByteArray::fromBase64(
                             record_["files"].toObject()["alpha"].toString().toLatin1()));
            }
            if (fault == "directory-unavailable" || fault == "directory-ambiguous") {
                const UInt32 missing = index("alpha");
                QVERIFY(missing != UInt32(-1));
                for (PROPID property :
                     {kpidSize, kpidPackSize, kpidMTime, kpidUserId, kpidINode, kpidPosixAttrib}) {
                    NWindows::NCOM::CPropVariant value;
                    QCOMPARE(archive_->GetProperty(missing, property, &value), S_OK);
                    QCOMPARE(value.vt, VARTYPE(VT_EMPTY));
                }
                CMyComPtr<IInArchiveGetStream> streams;
                QCOMPARE(archive_->QueryInterface(IID_IInArchiveGetStream,
                                                  reinterpret_cast<void **>(&streams)),
                         S_OK);
                CMyComPtr<ISequentialInStream> data;
                QCOMPARE(streams->GetStream(missing, &data), S_FALSE);
                QVERIFY(!data);
                const UInt32 items[] = {missing, index("hardlink")};
                diagnostic->capture = true;
                QCOMPARE(archive_->Extract(items, 2, 0, diagnostic), S_OK);
                QCOMPARE(diagnostic->result,
                         Int32(fault == "directory-ambiguous"
                                   ? NArchive::NExtract::NOperationResult::kDataError
                                   : NArchive::NExtract::NOperationResult::kOK));
                QCOMPARE(diagnostic->output->bytes,
                         QByteArray::fromBase64(
                             record_["files"].toObject()["hardlink"].toString().toLatin1()));
            }
            if (fault == "directory-ambiguous") {
                for (const QString &name : {QString("hardlink"), QString("данные 空")}) {
                    NWindows::NCOM::CPropVariant identity;
                    QCOMPARE(archive_->GetProperty(index(name), kpidStreamId, &identity), S_OK);
                    QCOMPARE(identity.vt, VARTYPE(VT_EMPTY));
                }
            }
            auto healthy = stream("hardlink");
            QVERIFY(healthy);
            check(healthy, "hardlink", 0, 8202);
            if (fault == "directory-tail")
                QCOMPARE(index("данные 空"), UInt32(-1));
            else {
                auto unicode = stream("данные 空");
                QVERIFY(unicode);
                check(unicode, "данные 空", 0, 16395);
            }
        }
        QCOMPARE(archive_->Close(), S_OK);
        loadFixture("zlib", true);
        CMyComPtr<IInStream> healthy = new Input(bytes_);
        QCOMPARE(archive_->Open(healthy, nullptr, nullptr), S_OK);
        QCOMPARE(index("[ROOT]"), UInt32(-1));
        QVERIFY(stream("alpha"));
    }
    void directoryCallbackFailureIsNotPartialSuccess() {
        loadFixture("zlib", false, "directory-tail");
        const auto directory = qFromLittleEndian<quint64>(bytes_.constData() + 72);
        CMyComPtr2<IInStream, Input> source;
        source.SetFromCls(new Input(bytes_));
        source->notifyAt = directory;
        CMyComPtr2<IArchiveOpenCallback, MetadataOpen> callback;
        callback.SetFromCls(new MetadataOpen);
        callback->source = source.ClsPtr();
        for (HRESULT result : {E_ABORT, E_OUTOFMEMORY, E_FAIL, S_FALSE}) {
            source->notified = false;
            callback->completionResult = result;
            QCOMPARE(source->Seek(0, STREAM_SEEK_SET, nullptr), S_OK);
            QCOMPARE(archive_->Open(source, nullptr, callback), result);
            UInt32 count = 99;
            QCOMPARE(archive_->GetNumberOfItems(&count), S_OK);
            QCOMPARE(count, 0u);
        }
    }
    void inodeFailuresDoNotHideHealthyRecords_data() {
        QTest::addColumn<QString>("fault");
        for (const QString &fault : {"inode-record", "inode-unused", "inode-count-short",
                                     "inode-count-large", "inode-length"})
            QTest::newRow(qPrintable(fault)) << fault;
    }
    void inodeFailuresDoNotHideHealthyRecords() {
        QFETCH(QString, fault);
        loadFixture("zlib", true, fault);
        CMyComPtr<IInStream> source = new Input(bytes_);
        QCOMPARE(archive_->Open(source, nullptr, nullptr), S_OK);
        auto data = stream("данные 空");
        QVERIFY(data);
        check(data, "данные 空", 0, 16395);
        NWindows::NCOM::CPropVariant identity;
        QCOMPARE(archive_->GetProperty(index("данные 空"), kpidINode, &identity), S_OK);
        QCOMPARE(identity.vt, VARTYPE(VT_EMPTY));
        if (fault == "inode-record") {
            const UInt32 missing = index("alpha");
            QVERIFY(missing != UInt32(-1));
            QVERIFY(!stream("alpha"));
            NWindows::NCOM::CPropVariant value;
            QCOMPARE(archive_->GetProperty(missing, kpidSize, &value), S_OK);
            QCOMPARE(value.vt, VARTYPE(VT_EMPTY));
            QCOMPARE(archive_->GetProperty(missing, kpidError, &value), S_OK);
            QCOMPARE(value.vt, VARTYPE(VT_BSTR));
            CMyComPtr2<IArchiveExtractCallback, Extract> output;
            output.SetFromCls(new Extract);
            output->capture = true;
            const UInt32 healthy = index("данные 空");
            QCOMPARE(archive_->Extract(&healthy, 1, 0, output), S_OK);
            QCOMPARE(output->result, Int32(NArchive::NExtract::NOperationResult::kOK));
            QCOMPARE(output->output->bytes,
                     QByteArray::fromBase64(
                         record_["files"].toObject()["данные 空"].toString().toLatin1()));
        } else if (fault != "inode-length") {
            auto first = stream("alpha");
            QVERIFY(first);
            check(first, "alpha", 0, 8202);
        }
        QCOMPARE(archive_->Close(), S_OK);
        loadFixture("zlib", true);
        CMyComPtr<IInStream> healthy = new Input(bytes_);
        QCOMPARE(archive_->Open(healthy, nullptr, nullptr), S_OK);
        QCOMPARE(archive_->GetProperty(index("alpha"), kpidINode, &identity), S_OK);
        QCOMPARE(identity.vt, VARTYPE(VT_UI4));
        QVERIFY(stream("alpha"));
    }
    void lateDirectoryInodeRestartsItsOwnMetadataRange() {
        loadFixture("zlib", true, "directory-block-inode");
        CMyComPtr<IInStream> source = new Input(bytes_);
        QCOMPARE(archive_->Open(source, nullptr, nullptr), S_OK);
        QVERIFY(!stream("alpha"));
        auto good = stream("good 空/payload");
        QVERIFY(good);
        check(good, "good 空/payload", 0, 333);
        QCOMPARE(index("broken/payload"), UInt32(-1));
        QCOMPARE(index("payload"), UInt32(-1));
        // Parent paths must remain stable when recovery appends another range.
        QVERIFY(index("broken") != UInt32(-1));
        QVERIFY(index("good 空") != UInt32(-1));
    }
    void unreadableRootRemainsAnOpeningFailure() {
        loadFixture("zlib", true, "inode-root");
        CMyComPtr<IInStream> source = new Input(bytes_);
        QCOMPARE(archive_->Open(source, nullptr, nullptr), S_FALSE);
        UInt32 count = 99;
        QCOMPARE(archive_->GetNumberOfItems(&count), S_OK);
        QCOMPARE(count, 0u);
    }
    void directoryBlocksAreIsolated_data() {
        QTest::addColumn<QString>("fault");
        for (const QString &fault :
             {"directory-block-clean", "directory-block-header", "directory-block-zlib",
              "directory-block-span", "directory-block-io"})
            QTest::newRow(qPrintable(fault)) << fault;
    }
    void directoryBlocksAreIsolated() {
        QFETCH(QString, fault);
        loadFixture("zlib", true, fault);
        const bool clean = fault == "directory-block-clean";
        const auto inode = qFromLittleEndian<quint64>(bytes_.constData() + 64);
        const auto directory = qFromLittleEndian<quint64>(bytes_.constData() + 72);
        const auto broken =
            directory + qFromLittleEndian<quint32>(bytes_.constData() + inode + 2 + 72 + 16);
        CMyComPtr2<IInStream, Input> source;
        source.SetFromCls(new Input(bytes_));
        if (fault == "directory-block-io")
            source->failAt = broken + 2;
        QCOMPARE(archive_->Open(source, nullptr, nullptr), S_OK);
        for (const QString &name : {QString("alpha"), QString("good 空/payload")}) {
            auto data = stream(name);
            QVERIFY(data);
            const auto expected =
                QByteArray::fromBase64(record_["files"].toObject()[name].toString().toLatin1());
            check(data, name, 0, expected.size());
        }
        QCOMPARE(index("broken/payload") != UInt32(-1), clean);
        QCOMPARE(index("payload"), UInt32(-1)); // Never join a directory across a metadata gap.
        NWindows::NCOM::CPropVariant error;
        QCOMPARE(archive_->GetProperty(index("broken"), kpidError, &error), S_OK);
        QCOMPARE(error.vt, VARTYPE(clean ? VT_EMPTY : VT_BSTR));
        if (fault == "directory-block-span") {
            const UInt32 root = index("[ROOT]");
            QVERIFY(root != UInt32(-1));
            QCOMPARE(archive_->GetProperty(root, kpidError, &error), S_OK);
            QCOMPARE(error.vt, VARTYPE(VT_BSTR));
        }
        QCOMPARE(archive_->Close(), S_OK);
        loadFixture("zlib", true, "directory-block-clean");
        CMyComPtr<IInStream> healthy = new Input(bytes_);
        QCOMPARE(archive_->Open(healthy, nullptr, nullptr), S_OK);
        QVERIFY(stream("broken/payload"));
    }
    void inodeBlocksAreIsolated_data() {
        QTest::addColumn<QString>("fault");
        for (const QString &fault :
             {"inode-block-clean", "inode-block-header", "inode-block-zlib", "inode-block-io"})
            QTest::newRow(qPrintable(fault)) << fault;
    }
    void inodeBlocksAreIsolated() {
        QFETCH(QString, fault);
        loadFixture("zlib", true, fault);
        const bool clean = fault == "inode-block-clean";
        const auto inode = qFromLittleEndian<quint64>(bytes_.constData() + 64);
        const auto broken =
            inode + 2 + (qFromLittleEndian<quint16>(bytes_.constData() + inode) & 0x7fff);
        CMyComPtr2<IInStream, Input> source;
        source.SetFromCls(new Input(bytes_));
        if (fault == "inode-block-io")
            source->failAt = broken + 2;
        QCOMPARE(archive_->Open(source, nullptr, nullptr), S_OK);
        auto good = stream("данные 空");
        QVERIFY(good);
        check(good, "данные 空", 0, 333);
        NWindows::NCOM::CPropVariant size;
        const UInt32 alpha = index("alpha");
        QVERIFY(alpha != UInt32(-1));
        QCOMPARE(archive_->GetProperty(alpha, kpidSize, &size), S_OK);
        QCOMPARE(size.vt, VARTYPE(clean ? VT_UI8 : VT_EMPTY));
        if (clean) {
            auto data = stream("alpha");
            QVERIFY(data);
            check(data, "alpha", 0, 237);
        } else {
            QVERIFY(!stream("alpha"));
            NWindows::NCOM::CPropVariant error;
            QCOMPARE(archive_->GetProperty(alpha, kpidError, &error), S_OK);
            QCOMPARE(error.vt, VARTYPE(VT_BSTR));
        }
        QCOMPARE(archive_->Close(), S_OK);
        loadFixture("zlib", true, "inode-block-clean");
        CMyComPtr<IInStream> healthy = new Input(bytes_);
        QCOMPARE(archive_->Open(healthy, nullptr, nullptr), S_OK);
        QVERIFY(stream("alpha"));
    }
    void unreadableRootMetadataCannotBeOpened() {
        loadFixture("zlib", true, "inode-block-root");
        CMyComPtr<IInStream> source = new Input(bytes_);
        QCOMPARE(archive_->Open(source, nullptr, nullptr), S_FALSE);
        UInt32 count = 99;
        QCOMPARE(archive_->GetNumberOfItems(&count), S_OK);
        QCOMPARE(count, 0u);
    }
    void metadataBlockFailuresRemainFatalWhenRequired_data() {
        QTest::addColumn<QString>("fault");
        QTest::newRow("directory") << QString("directory-block-io");
        QTest::newRow("inode") << QString("inode-block-io");
    }
    void metadataBlockFailuresRemainFatalWhenRequired() {
        QFETCH(QString, fault);
        loadFixture("zlib", true, fault);
        const auto inode = qFromLittleEndian<quint64>(bytes_.constData() + 64);
        const auto directory = qFromLittleEndian<quint64>(bytes_.constData() + 72);
        const auto broken =
            fault == "inode-block-io"
                ? inode + 2 + (qFromLittleEndian<quint16>(bytes_.constData() + inode) & 0x7fff)
                : directory + qFromLittleEndian<quint32>(bytes_.constData() + inode + 2 + 72 + 16);
        CMyComPtr2<IInStream, Input> source;
        source.SetFromCls(new Input(bytes_));
        source->failAt = broken + 2;
        for (HRESULT result : {E_ABORT, E_OUTOFMEMORY, E_NOTIMPL}) {
            source->atFailure = result;
            QCOMPARE(source->Seek(0, STREAM_SEEK_SET, nullptr), S_OK);
            QCOMPARE(archive_->Open(source, nullptr, nullptr), result);
            UInt32 count = 99;
            QCOMPARE(archive_->GetNumberOfItems(&count), S_OK);
            QCOMPARE(count, 0u);
        }
        source->failAt = UINT64_MAX;
        source->notifyAt = broken + 2;
        CMyComPtr2<IArchiveOpenCallback, MetadataOpen> callback;
        callback.SetFromCls(new MetadataOpen);
        callback->source = source.ClsPtr();
        for (HRESULT result : {E_ABORT, E_FAIL, S_FALSE}) {
            source->notified = false;
            callback->completionResult = result;
            QCOMPARE(source->Seek(0, STREAM_SEEK_SET, nullptr), S_OK);
            QCOMPARE(archive_->Open(source, nullptr, callback), result);
        }
    }
    void unavailableTablesKeepReadableData_data() {
        QTest::addColumn<QString>("fault");
        for (const QString &fault :
             {"uid-index", "gid-index", "id-metadata-zero", "fragment-metadata-zero",
              "fragment-index-invalid", "metadata-combined"})
            QTest::newRow(qPrintable(fault)) << fault;
    }
    void unavailableIndexesKeepIndependentReads_data() {
        QTest::addColumn<QString>("fault");
        for (const QString &fault :
             {"xattr-ids-zero", "xattr-index-outside", "xattr-table-start-outside",
              "id-table-index-outside", "id-table-start-outside", "id-metadata-gap",
              "id-count-zero", "export-index-outside", "export-table-start-outside",
              "fragment-table-index-outside", "fragment-table-start-outside",
              "fragment-count-large", "table-indexes-combined"})
            QTest::newRow(qPrintable(fault)) << fault;
    }
    void unavailableIndexesKeepIndependentReads() {
        QFETCH(QString, fault);
        loadFixture("zlib", true, fault);
        const bool fragments = fault.startsWith("fragment-") || fault == "table-indexes-combined";
        const bool ids = fault.startsWith("id-") || fault == "table-indexes-combined";
        const bool xattrs = fault.startsWith("xattr-") || fault == "table-indexes-combined";
        CMyComPtr<IInStream> source = new Input(bytes_);
        QCOMPARE(archive_->Open(source, nullptr, nullptr), S_OK);
        auto good = stream("данные 空");
        QVERIFY(good);
        const int length =
            QByteArray::fromBase64(record_["files"].toObject()["данные 空"].toString().toLatin1())
                .size();
        check(good, "данные 空", 0, length);
        auto first = stream("alpha");
        QVERIFY(first);
        check(first, "alpha", 0, fragments ? 8192 : 8202);
        if (fragments) {
            UInt32 count = 99;
            char byte;
            QCOMPARE(first->Read(&byte, 1, &count), S_FALSE);
            QCOMPARE(count, 0u);
        }
        NWindows::NCOM::CPropVariant uid;
        QCOMPARE(archive_->GetProperty(index("alpha"), kpidUserId, &uid), S_OK);
        QCOMPARE(uid.vt, VARTYPE(ids ? VT_EMPTY : VT_UI4));
        if (!ids)
            QCOMPARE(uid.ulVal, 1000u);
        auto attribute = stream("alpha:$XATTR.user.inline");
        QCOMPARE(bool(attribute), !xattrs);
        if (attribute) {
            char bytes[7];
            UInt32 count = 0;
            QCOMPARE(attribute->Read(bytes, sizeof(bytes), &count), S_OK);
            QCOMPARE(QByteArray(bytes, int(count)), QByteArray("v\0a\0lue", 7));
        }
        QCOMPARE(archive_->Close(), S_OK);
        loadFixture("zlib", true);
        CMyComPtr<IInStream> healthy = new Input(bytes_);
        QCOMPARE(archive_->Open(healthy, nullptr, nullptr), S_OK);
        QVERIFY(stream("alpha:$XATTR.user.inline"));
        auto complete = stream("alpha");
        QVERIFY(complete);
        check(complete, "alpha", 0, 8202);
        QCOMPARE(archive_->GetProperty(index("alpha"), kpidUserId, &uid), S_OK);
        QCOMPARE(uid.vt, VARTYPE(VT_UI4));
    }
    void indexIoFailuresPreserveFatalResults_data() {
        QTest::addColumn<int>("field");
        for (const int field : {48, 56, 80, 88})
            QTest::newRow(qPrintable(QString::number(field))) << field;
    }
    void indexIoFailuresPreserveFatalResults() {
        QFETCH(int, field);
        loadFixture("zlib", true);
        CMyComPtr2<IInStream, Input> source;
        source.SetFromCls(new Input(bytes_));
        source->failAt = qFromLittleEndian<quint64>(bytes_.constData() + field);
        for (HRESULT failure : {E_ABORT, E_OUTOFMEMORY, E_NOTIMPL}) {
            source->atFailure = failure;
            QCOMPARE(source->Seek(0, STREAM_SEEK_SET, nullptr), S_OK);
            QCOMPARE(archive_->Open(source, nullptr, nullptr), failure);
            UInt32 count = 99;
            QCOMPARE(archive_->GetNumberOfItems(&count), S_OK);
            QCOMPARE(count, 0u);
        }
        source->atFailure = E_FAIL;
        QCOMPARE(source->Seek(0, STREAM_SEEK_SET, nullptr), S_OK);
        QCOMPARE(archive_->Open(source, nullptr, nullptr), S_OK);
        auto good = stream("данные 空");
        QVERIFY(good);
        check(good, "данные 空", 0, 8192);
        source->failAt = UINT64_MAX;
        source->notifyAt = qFromLittleEndian<quint64>(bytes_.constData() + field);
        CMyComPtr2<IArchiveOpenCallback, MetadataOpen> callback;
        callback.SetFromCls(new MetadataOpen);
        callback->source = source.ClsPtr();
        for (HRESULT failure : {E_ABORT, E_FAIL, S_FALSE}) {
            source->notified = false;
            callback->completionResult = failure;
            QCOMPARE(source->Seek(0, STREAM_SEEK_SET, nullptr), S_OK);
            QCOMPARE(archive_->Open(source, nullptr, callback), failure);
        }
    }
    void unavailableTablesKeepReadableData() {
        QFETCH(QString, fault);
        loadFixture("zlib", true, fault);
        const bool fragment = fault.startsWith("fragment-") || fault == "metadata-combined";
        CMyComPtr<IInStream> source = new Input(bytes_);
        QCOMPARE(archive_->Open(source, nullptr, nullptr), S_OK);
        for (const QString &name : {QString("alpha"), QString("hardlink"), QString("данные 空")}) {
            const UInt32 item = index(name);
            const bool incomplete = fragment && name != "данные 空";
            const auto expected =
                QByteArray::fromBase64(record_["files"].toObject()[name].toString().toLatin1());
            auto data = stream(name);
            QVERIFY(data);
            check(data, name, 0, incomplete ? 8192 : expected.size());
            if (incomplete) {
                char byte;
                UInt32 read = 99;
                QCOMPARE(data->Read(&byte, 1, &read), S_FALSE);
                QCOMPARE(read, UInt32(0));
                UInt64 end = 0;
                QCOMPARE(data->Seek(0, STREAM_SEEK_END, &end), S_OK);
                QCOMPARE(end, UInt64(expected.size()));
            }
            NWindows::NCOM::CPropVariant uid, gid, packed;
            QCOMPARE(archive_->GetProperty(item, kpidUserId, &uid), S_OK);
            QCOMPARE(archive_->GetProperty(item, kpidGroupId, &gid), S_OK);
            const bool first = name != "данные 空";
            const bool noUid = fault == "id-metadata-zero" ||
                               (first && (fault == "uid-index" || fault == "metadata-combined"));
            const bool noGid = fault == "id-metadata-zero" || (first && fault == "gid-index");
            QCOMPARE(uid.vt, VARTYPE(noUid ? VT_EMPTY : VT_UI4));
            QCOMPARE(gid.vt, VARTYPE(noGid ? VT_EMPTY : VT_UI4));
            if (!noUid)
                QCOMPARE(uid.ulVal, 1000u);
            if (!noGid)
                QCOMPARE(gid.ulVal, 1000u);
            QCOMPARE(archive_->GetProperty(item, kpidPackSize, &packed), S_OK);
            QCOMPARE(packed.vt, VARTYPE(incomplete ? VT_EMPTY : VT_UI8));
            CMyComPtr2<IArchiveExtractCallback, Extract> output;
            output.SetFromCls(new Extract);
            output->capture = true;
            QCOMPARE(archive_->Extract(&item, 1, 0, output), S_OK);
            QCOMPARE(output->result,
                     Int32(incomplete ? NArchive::NExtract::NOperationResult::kDataError
                                      : NArchive::NExtract::NOperationResult::kOK));
            QCOMPARE(output->output->bytes, incomplete ? expected.left(8192) : expected);
        }
        if (fragment) {
            const UInt32 items[] = {index("alpha"), index("данные 空")};
            CMyComPtr2<IArchiveExtractCallback, Extract> output;
            output.SetFromCls(new Extract);
            output->capture = true;
            QCOMPARE(archive_->Extract(items, 2, 0, output), S_OK);
            QCOMPARE(output->result, Int32(NArchive::NExtract::NOperationResult::kOK));
            const auto files = record_["files"].toObject();
            const auto first = QByteArray::fromBase64(files["alpha"].toString().toLatin1());
            const auto second = QByteArray::fromBase64(files["данные 空"].toString().toLatin1());
            QCOMPARE(output->output->bytes, first.left(8192) + second);
        }
        auto retained = stream("alpha");
        QVERIFY(retained);
        QCOMPARE(archive_->Close(), S_OK);
        loadFixture("zlib", true);
        CMyComPtr<IInStream> healthy = new Input(bytes_);
        QCOMPARE(archive_->Open(healthy, nullptr, nullptr), S_OK);
        check(retained, "alpha", 0, 8192);
        if (fragment) {
            char byte;
            UInt32 read = 0;
            QCOMPARE(retained->Read(&byte, 1, &read), S_FALSE);
        }
        NWindows::NCOM::CPropVariant error, uid;
        QCOMPARE(archive_->GetProperty(index("alpha"), kpidError, &error), S_OK);
        QCOMPARE(error.vt, VARTYPE(VT_EMPTY));
        QCOMPARE(archive_->GetProperty(index("alpha"), kpidUserId, &uid), S_OK);
        QCOMPARE(uid.vt, VARTYPE(VT_UI4));
        QCOMPARE(uid.ulVal, 1000u);
    }
    void tableReadFailuresAndCancellation_data() {
        QTest::addColumn<bool>("fragment");
        QTest::newRow("ownership") << false;
        QTest::newRow("fragment") << true;
    }
    void fragmentsAreLoadedByReferencedIndex_data() {
        QTest::addColumn<QString>("fault");
        for (const QString &fault :
             {"fragment-remap", "fragment-high-index", "fragment-unused-block"})
            QTest::newRow(qPrintable(fault)) << fault;
    }
    void fragmentsAreLoadedByReferencedIndex() {
        QFETCH(QString, fault);
        loadFixture("zlib", true, fault);
        CMyComPtr2<IInStream, Input> source;
        source.SetFromCls(new Input(bytes_));
        if (fault == "fragment-unused-block") {
            const auto table = qFromLittleEndian<quint64>(bytes_.constData() + 80);
            source->failAt = qFromLittleEndian<quint64>(bytes_.constData() + table + 8);
            source->atFailure = E_ABORT;
        }
        QCOMPARE(archive_->Open(source, nullptr, nullptr), S_OK);
        auto first = stream("alpha");
        auto second = stream("данные 空");
        QVERIFY(first && second);
        check(first, "alpha", 8189, 13);
        check(second, "данные 空", 16380, 15);
        check(first, "alpha", 0, 8202);
        for (const QString &name : {QString("alpha"), QString("hardlink"), QString("данные 空")}) {
            NWindows::NCOM::CPropVariant error;
            QCOMPARE(archive_->GetProperty(index(name), kpidError, &error), S_OK);
            QCOMPARE(error.vt, VARTYPE(VT_EMPTY));
        }
        QCOMPARE(archive_->Close(), S_OK);
        loadFixture("zlib", true);
        CMyComPtr<IInStream> healthy = new Input(bytes_);
        QCOMPARE(archive_->Open(healthy, nullptr, nullptr), S_OK);
        check(first, "alpha", 8192, 10);
        check(second, "данные 空", 16384, 11);
        auto reopened = stream("alpha");
        QVERIFY(reopened);
        check(reopened, "alpha", 0, 8202);
    }
    void tableReadFailuresAndCancellation() {
        QFETCH(bool, fragment);
        loadFixture("zlib", true);
        const auto table = qFromLittleEndian<quint64>(bytes_.constData() + (fragment ? 80 : 48));
        const auto metadata = qFromLittleEndian<quint64>(bytes_.constData() + table);
        CMyComPtr2<IInStream, Input> source;
        source.SetFromCls(new Input(bytes_));
        source->failAt = metadata;
        for (HRESULT failure : {E_ABORT, E_OUTOFMEMORY}) {
            source->atFailure = failure;
            QCOMPARE(source->Seek(0, STREAM_SEEK_SET, nullptr), S_OK);
            QCOMPARE(archive_->Open(source, nullptr, nullptr), failure);
            UInt32 count = 99;
            QCOMPARE(archive_->GetNumberOfItems(&count), S_OK);
            QCOMPARE(count, 0u);
        }
        source->atFailure = E_FAIL;
        QCOMPARE(source->Seek(0, STREAM_SEEK_SET, nullptr), S_OK);
        QCOMPARE(archive_->Open(source, nullptr, nullptr), S_OK);
        QVERIFY(stream("alpha"));
        source->failAt = UINT64_MAX;
        source->notifyAt = metadata;
        CMyComPtr2<IArchiveOpenCallback, MetadataOpen> callback;
        callback.SetFromCls(new MetadataOpen);
        callback->source = source.ClsPtr();
        callback->completionResult = E_FAIL;
        QCOMPARE(source->Seek(0, STREAM_SEEK_SET, nullptr), S_OK);
        QCOMPARE(archive_->Open(source, nullptr, callback), E_FAIL);
    }
    void unreadableExportPayloadDoesNotBlockFiles() {
        QFETCH(HRESULT, failure);
        loadFixture("zlib", true);
        const auto table = qFromLittleEndian<quint64>(bytes_.constData() + 88);
        const auto metadata = qFromLittleEndian<quint64>(bytes_.constData() + table);
        CMyComPtr2<IInStream, Input> source;
        source.SetFromCls(new Input(bytes_));
        source->failAt = metadata;
        source->atFailure = failure;
        QCOMPARE(archive_->Open(source, nullptr, nullptr), S_OK);
        for (const QString &name : {QString("alpha"), QString("hardlink"), QString("данные 空")}) {
            auto data = stream(name);
            QVERIFY(data);
            const int size =
                QByteArray::fromBase64(record_["files"].toObject()[name].toString().toLatin1())
                    .size();
            check(data, name, 0, size);
        }
        auto value = stream("alpha:$XATTR.user.inline");
        QVERIFY(value);
        char data[7];
        UInt32 read = 0;
        QCOMPARE(value->Read(data, sizeof(data), &read), S_OK);
        QCOMPARE(QByteArray(data, int(read)), QByteArray("v\0a\0lue", 7));
    }
    void interleavedStreamsSurviveCloseReopenAndRelease_data() { codecs(); }
    void interleavedStreamsSurviveCloseReopenAndRelease() {
        QFETCH(QString, codec);
        loadFixture(codec);
        CMyComPtr<IInStream> input = new Input(bytes_);
        QCOMPARE(archive_->Open(input, nullptr, nullptr), S_OK);
        auto first = stream("alpha");
        auto second = stream(QStringLiteral("данные 空"));
        auto link = stream("link");
        QVERIFY(first && second && link);
        check(first, "alpha", 0, 53);
        check(second, QStringLiteral("данные 空"), 0, 53);
        check(first, "alpha", 8192, 10);
        check(second, QStringLiteral("данные 空"), 16384, 11);
        QCOMPARE(archive_->Close(), S_OK);
        CMyComPtr<IInStream> replacement = new Input(bytes_);
        QCOMPARE(archive_->Open(replacement, nullptr, nullptr), S_OK);
        archive_.Release();
        input.Release();
        check(link, "link", 0, 5);
        check(second, QStringLiteral("данные 空"), 8192, 8192);
        check(first, "alpha", 8175, 27);
        check(second, QStringLiteral("данные 空"), 0, 8200);
        createArchive();
    }
    void sourceErrorsAndCancellationPropagateWithoutPoisoningRetry_data() { codecs(); }
    void sourceErrorsAndCancellationPropagateWithoutPoisoningRetry() {
        QFETCH(QString, codec);
        loadFixture(codec);
        auto *source = new Input(bytes_);
        CMyComPtr<IInStream> input = source;
        QCOMPARE(archive_->Open(input, nullptr, nullptr), S_OK);
        const UInt32 item = index("alpha");
        QVERIFY(item != UInt32(-1));
        CMyComPtr<IArchiveExtractCallback> callback = new Extract;
        for (HRESULT error : {E_ABORT, E_FAIL}) {
            source->failure = error;
            QCOMPARE(archive_->Extract(&item, 1, 1, callback), error);
        }
        source->failure = S_OK;
        QCOMPARE(archive_->Extract(&item, 1, 1, callback), S_OK);
        QCOMPARE(static_cast<Extract *>(callback.Interface())->result,
                 Int32(NArchive::NExtract::NOperationResult::kOK));
        static_cast<Extract *>(callback.Interface())->cancel = true;
        QCOMPARE(archive_->Extract(&item, 1, 1, callback), E_ABORT);
    }
    void backendDiscoversModuleAndReadsCompressedFiles_data() {
        codecs();
        QTest::newRow("native-metadata") << QString("native");
    }
    void backendDiscoversModuleAndReadsCompressedFiles() {
        QFETCH(QString, codec);
        loadFixture(codec);
        QTemporaryDir runtime;
        QVERIFY(runtime.isValid());
#ifdef Q_OS_WIN
        const QString engineName = "7z.exe", coreName = "7z.dll";
#else
        const QString engineName = "7z", coreName = "7z.so";
#endif
        const QDir prepared(QString::fromUtf8(SEVENZIP_SQUASHFS_RUNTIME));
        const QString engine = runtime.filePath(engineName), core = runtime.filePath(coreName);
        QVERIFY(QFile::copy(prepared.filePath(engineName), engine));
        QVERIFY(QFile::copy(prepared.filePath(coreName), core));
        QVERIFY(QDir(runtime.path()).mkdir("Formats"));
        const QString module = QString::fromUtf8(SEVENZIP_SQUASHFS_CORE);
        QVERIFY(QFile::copy(module, runtime.filePath("Formats/" + QFileInfo(module).fileName())));
        const QString image = runtime.filePath(QStringLiteral("образ 空.squashfs"));
        QFile source(image);
        QVERIFY(source.open(QIODevice::WriteOnly));
        QCOMPARE(source.write(bytes_), bytes_.size());
        source.close();
        ArchiveBackend backend(nullptr, engine);
        QSignalSpy ready(&backend, &ArchiveBackend::codecCatalogReady);
        QVERIFY(backend.discoverCodecCatalog(core));
        QTRY_COMPARE_WITH_TIMEOUT(ready.size(), 1, 15000);
        QVERIFY2(ready[0][0].toBool(), qPrintable(ready[0][1].toString()));
        QVERIFY(!backend.codecCatalog()->canUpdate("SquashFS"));
        QList<ArchiveEntry> entries;
        connect(&backend, &ArchiveBackend::entriesReady, this,
                [&](const QList<ArchiveEntry> &result) { entries = result; });
        QSignalSpy finished(&backend, &ArchiveBackend::finished);
        backend.list(image);
        QTRY_COMPARE_WITH_TIMEOUT(finished.size(), 1, 10000);
        QVERIFY2(finished[0][0].toBool(), qPrintable(finished[0][1].toString()));
        if (codec == "native") {
            for (const QString &entry :
                 {QString("data.bin:$XATTR.user.large"), QString("nested:$XATTR.user.directory"),
                  QString("[ROOT]_:$XATTR.user.root")}) {
                QVERIFY(
                    std::any_of(entries.cbegin(), entries.cend(), [&](const ArchiveEntry &value) {
                        return value.path == entry && !value.directory &&
                               value.properties.value("Alternate Stream") == "+";
                    }));
                finished.clear();
                const QString saved = runtime.filePath("selected.bin");
                QFile::remove(saved);
                backend.extractSourceEntry(image, entry, saved, "SquashFS", {}, true);
                QTRY_COMPARE_WITH_TIMEOUT(finished.size(), 1, 10000);
                QVERIFY2(finished[0][0].toBool(), qPrintable(finished[0][1].toString()));
                QFile output(saved);
                QVERIFY(output.open(QIODevice::ReadOnly));
                const int colon = entry.indexOf(':');
                const QString host = entry.left(colon) == "[ROOT]_" ? QString() : entry.left(colon);
                const QString name = entry.mid(colon + QString(":$XATTR.").size());
                QCOMPARE(output.readAll(), QByteArray::fromHex(record_["attributes"]
                                                                   .toObject()[host]
                                                                   .toObject()[name]
                                                                   .toString()
                                                                   .toLatin1()));
            }
            QVERIFY(source.open(QIODevice::ReadOnly));
            QCOMPARE(source.readAll(), bytes_);
            return;
        }
        const QString name = QStringLiteral("данные 空");
        QVERIFY(std::any_of(entries.cbegin(), entries.cend(),
                            [&](const ArchiveEntry &entry) { return entry.path == name; }));
        finished.clear();
        backend.test(image);
        QTRY_COMPARE_WITH_TIMEOUT(finished.size(), 1, 10000);
        QVERIFY2(finished[0][0].toBool(), qPrintable(finished[0][1].toString()));
        finished.clear();
        const QString output = runtime.filePath("output");
        backend.extract(image, output);
        QTRY_COMPARE_WITH_TIMEOUT(finished.size(), 1, 10000);
        QVERIFY(!finished[0][0].toBool());
        QVERIFY(!QFileInfo::exists(output));
        finished.clear();
        backend.extract(image, output, {name});
        QTRY_COMPARE_WITH_TIMEOUT(finished.size(), 1, 10000);
        QVERIFY2(finished[0][0].toBool(), qPrintable(finished[0][1].toString()));
        QFile result(QDir(output).filePath(name));
        QVERIFY(result.open(QIODevice::ReadOnly));
        QCOMPARE(result.readAll(),
                 QByteArray::fromBase64(record_["files"].toObject()[name].toString().toLatin1()));
        QVERIFY(source.open(QIODevice::ReadOnly));
        QCOMPARE(source.readAll(), bytes_);
    }
    void cleanupTestCase() { archive_.Release(); }
    void metadataStreamOwnershipAndParent() {
        loadFixture("native");
        CMyComPtr<IInStream> source = new Input(bytes_);
        QCOMPARE(archive_->Open(source, nullptr, nullptr), S_OK);
        const UInt32 host = index("data.bin"), attribute = index("data.bin:$XATTR.user.large");
        QVERIFY(host != UInt32(-1) && attribute != UInt32(-1));
        CMyComPtr<IArchiveGetRawProps> raw;
        QCOMPARE(archive_->QueryInterface(IID_IArchiveGetRawProps, reinterpret_cast<void **>(&raw)),
                 S_OK);
        UInt32 parent = 0, type = 0;
        QCOMPARE(raw->GetParent(attribute, &parent, &type), S_OK);
        QCOMPARE(parent, host);
        QCOMPARE(type, UInt32(NParentType::kAltStream));
        auto value = stream("data.bin:$XATTR.user.large");
        auto empty = stream("data.bin:$XATTR.user.empty");
        QVERIFY(value && empty);
        const auto expected = QByteArray::fromHex(record_["attributes"]
                                                      .toObject()["data.bin"]
                                                      .toObject()["user.large"]
                                                      .toString()
                                                      .toLatin1());
        QCOMPARE(archive_->Close(), S_OK);
        loadFixture("zlib");
        CMyComPtr<IInStream> second = new Input(bytes_);
        QCOMPARE(archive_->Open(second, nullptr, nullptr), S_OK);
        raw.Release();
        source.Release();
        archive_.Release();
        QCOMPARE(value->Seek(8180, STREAM_SEEK_SET, nullptr), S_OK);
        char data[37];
        UInt32 count = 0;
        QCOMPARE(value->Read(data, sizeof(data), &count), S_OK);
        QCOMPARE(QByteArray(data, int(count)), expected.mid(8180, sizeof(data)));
        QCOMPARE(empty->Read(data, sizeof(data), &count), S_OK);
        QCOMPARE(count, UInt32(0));
        createArchive();
    }
    void metadataReadFailureCancellationAndReuse() {
        loadFixture("zlib", true);
        const auto table = qFromLittleEndian<quint64>(bytes_.constData() + 56);
        const auto xattrs = qFromLittleEndian<quint64>(bytes_.constData() + table);
        CMyComPtr2<IInStream, Input> source;
        source.SetFromCls(new Input(bytes_));
        source->failAt = xattrs;
        for (HRESULT result : {E_ABORT, E_OUTOFMEMORY}) {
            QCOMPARE(source->Seek(0, STREAM_SEEK_SET, nullptr), S_OK);
            source->atFailure = result;
            QCOMPARE(archive_->Open(source, nullptr, nullptr), result);
            UInt32 count = 99;
            QCOMPARE(archive_->GetNumberOfItems(&count), S_OK);
            QCOMPARE(count, UInt32(0));
        }
        source->notifyAt = source->failAt;
        source->failAt = UINT64_MAX;
        CMyComPtr2<IArchiveOpenCallback, MetadataOpen> callback;
        callback.SetFromCls(new MetadataOpen);
        callback->source = source.ClsPtr();
        QCOMPARE(source->Seek(0, STREAM_SEEK_SET, nullptr), S_OK);
        QCOMPARE(archive_->Open(source, nullptr, callback), E_ABORT);
        source->notified = false;
        source->notifyAt = UINT64_MAX;
        QCOMPARE(source->Seek(0, STREAM_SEEK_SET, nullptr), S_OK);
        QCOMPARE(archive_->Open(source, nullptr, callback), S_OK);
        const UInt32 attribute = index("alpha:$XATTR.user.duplicate");
        QVERIFY(attribute != UInt32(-1));
        CMyComPtr2<IArchiveExtractCallback, Extract> output;
        output.SetFromCls(new Extract);
        output->capture = true;
        QCOMPARE(archive_->Extract(&attribute, 1, 0, output), S_OK);
        QByteArray expected;
        for (int i = 0; i < 36; ++i)
            for (int byte = 0; byte < 251; ++byte)
                expected += char(byte);
        QCOMPARE(output->output->bytes, expected);
        QCOMPARE(output->completed, UInt64(expected.size()));
        output->output->bytes.clear();
        output->cancelAfterOutput = true;
        QCOMPARE(archive_->Extract(&attribute, 1, 0, output), E_ABORT);
        output->cancelAfterOutput = false;
        output->output->bytes.clear();
        output->output->fail = true;
        QCOMPARE(archive_->Extract(&attribute, 1, 0, output), E_FAIL);
        output->output->fail = false;
        QCOMPARE(archive_->Extract(&attribute, 1, 0, output), S_OK);
        QCOMPARE(output->output->bytes, expected);
    }
    void unavailableAttributesPreserveFileData_data() {
        QTest::addColumn<bool>("outOfLine");
        QTest::addColumn<bool>("corruptReference");
        QTest::newRow("attribute-list-unreadable") << false << false;
        QTest::newRow("large-value-unreadable") << true << false;
        QTest::newRow("large-reference-invalid") << true << true;
    }
    void unavailableAttributesPreserveFileData() {
        QFETCH(bool, outOfLine);
        QFETCH(bool, corruptReference);
        loadFixture("zlib", true);
        const auto table = qFromLittleEndian<quint64>(bytes_.constData() + 56);
        const auto xattrs = qFromLittleEndian<quint64>(bytes_.constData() + table);
        if (corruptReference) {
            const int name = bytes_.indexOf("large", int(xattrs));
            QVERIFY(name >= 0);
            qToLittleEndian(UINT64_MAX, bytes_.data() + name + 9);
        }
        CMyComPtr2<IInStream, Input> source;
        source.SetFromCls(new Input(bytes_));
        source->failAt = corruptReference ? UINT64_MAX : xattrs + (outOfLine ? 8194 : 0);
        QCOMPARE(archive_->Open(source, nullptr, nullptr), S_OK);
        NWindows::NCOM::CPropVariant readOnly;
        QCOMPARE(archive_->GetArchiveProperty(kpidReadOnly, &readOnly), S_OK);
        QCOMPARE(readOnly.vt, VARTYPE(VT_BOOL));
        QCOMPARE(readOnly.boolVal, VARIANT_TRUE);
        bool limitations = false;
        PROPID limitationsId = 0;
        UInt32 properties = 0;
        QCOMPARE(archive_->GetNumberOfArchiveProperties(&properties), S_OK);
        for (UInt32 i = 0; i < properties; ++i) {
            CMyComBSTR name;
            PROPID id;
            VARTYPE type;
            QCOMPARE(archive_->GetArchivePropertyInfo(i, &name, &id, &type), S_OK);
            if (name && QString::fromWCharArray(name) == "Read-only limitations") {
                NWindows::NCOM::CPropVariant value;
                QCOMPARE(archive_->GetArchiveProperty(id, &value), S_OK);
                QVERIFY(value.vt == VT_BSTR && value.bstrVal[0]);
                limitations = true;
                limitationsId = id;
            }
        }
        QVERIFY(limitations);
        for (const QString &name : {QString("alpha"), QString("hardlink"), QString("данные 空")}) {
            auto data = stream(name);
            QVERIFY(data);
            const int size =
                QByteArray::fromBase64(record_["files"].toObject()[name].toString().toLatin1())
                    .size();
            check(data, name, 0, size);
            NWindows::NCOM::CPropVariant itemError;
            QCOMPARE(archive_->GetProperty(index(name), kpidError, &itemError), S_OK);
            QCOMPARE(itemError.vt, VARTYPE(name == "данные 空" ? VT_EMPTY : VT_BSTR));
        }
        QCOMPARE(index("alpha:$XATTR.trusted.large"), UInt32(-1));
        QCOMPARE(index("alpha:$XATTR.user.inline") != UInt32(-1), outOfLine);
        QCOMPARE(index("alpha:$XATTR.user.duplicate") != UInt32(-1), corruptReference);
        if (corruptReference) {
            auto data = stream("alpha:$XATTR.user.duplicate");
            QVERIFY(data);
            QByteArray expected;
            for (int i = 0; i < 36; ++i)
                for (int byte = 0; byte < 251; ++byte)
                    expected += char(byte);
            QByteArray value(expected.size(), '!');
            UInt32 read = 0;
            QCOMPARE(data->Read(value.data(), UInt32(value.size()), &read), S_OK);
            QCOMPARE(read, UInt32(value.size()));
            QCOMPARE(value, expected);
        }
        if (outOfLine) {
            auto data = stream("alpha:$XATTR.user.inline");
            QVERIFY(data);
            char value[7];
            UInt32 read = 0;
            QCOMPARE(data->Read(value, sizeof(value), &read), S_OK);
            QCOMPARE(QByteArray(value, int(read)), QByteArray("v\0a\0lue", 7));
        }
        QCOMPARE(archive_->Close(), S_OK);
        loadFixture("zlib", true);
        CMyComPtr<IInStream> healthy = new Input(bytes_);
        QCOMPARE(archive_->Open(healthy, nullptr, nullptr), S_OK);
        NWindows::NCOM::CPropVariant error;
        QCOMPARE(archive_->GetArchiveProperty(limitationsId, &error), S_OK);
        QCOMPARE(error.vt, VARTYPE(VT_EMPTY));
        QVERIFY(index("alpha:$XATTR.trusted.large") != UInt32(-1));
    }
};
QTEST_GUILESS_MAIN(SquashfsStreamTests)
#include "squashfs_stream_tests.moc"
