#include "7zip/Common/FileStreams.h"
#include "7zip/Common/StreamObjects.h"
#include "CPP/7zip/UI/Agent/Agent.h"
#include "CPP/7zip/UI/Common/ArchiveOpenCallback.h"
#include "Common/StringConvert.h"
#include "archive_copy_callback.h"
#include "archivebackend.h"
#include "archivelayercontext.h"
#include "archivereadstream.h"
#include "qtarchivefileresource.h"
#include "qtarchivefoldermanager.h"
#include "qtarchiveitemopen.h"
#include <QCryptographicHash>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QProcess>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QtEndian>
#include <QtTest>
#include <future>

namespace {
class COpenMeter final : public IOpenCallbackUI {
  public:
    std::optional<UInt64> Total, Completed;
    HRESULT Open_CheckBreak() override { return S_OK; }
    HRESULT Open_SetTotal(const UInt64 *, const UInt64 *bytes) override {
        if (bytes)
            Total = *bytes;
        return S_OK;
    }
    HRESULT Open_SetCompleted(const UInt64 *, const UInt64 *bytes) override {
        if (bytes)
            Completed = *bytes;
        return S_OK;
    }
    HRESULT Open_Finished() override { return S_OK; }
    HRESULT Open_CryptoGetTextPassword(BSTR *) override { return E_ABORT; }
};
class CBudgetInput final : public IInStream,
                           public IArchiveLayerSource,
                           public IArchiveReadStream,
                           public CMyUnknownImp {
    Z7_COM_UNKNOWN_IMP_4(IInStream, ISequentialInStream, IArchiveLayerSource, IArchiveReadStream)
    CMyComPtr<IInStream> source_;
    std::optional<UInt64> depth_;
    bool *released_;

  public:
    UInt64 Remaining = 2 << 20;
    HRESULT Failure = S_OK;
    bool *CancelAtRead = nullptr;
    bool ObservedRequest = false;
    bool ObservedWorker = false;
    QThread *OwnerThread = QThread::currentThread();
    explicit CBudgetInput(IInStream *source, bool *released = nullptr)
        : source_(source), released_(released) {
        CMyComPtr<IArchiveLayerSource> context;
        if (source->QueryInterface(IID_IArchiveLayerSource, (void **)&context) == S_OK && context) {
            UInt64 depth;
            if (context->GetContainingContainerDepth(&depth) == S_OK)
                depth_ = depth;
        }
    }
    ~CBudgetInput() {
        if (released_)
            *released_ = true;
    }
    Z7_COM7F_IMF(GetContainingContainerDepth(UInt64 *depth)) override {
        if (!depth)
            return E_INVALIDARG;
        *depth = depth_.value_or(0);
        return depth_ ? S_OK : S_FALSE;
    }
    Z7_COM7F_IMF(Read(void *data, UInt32 size, UInt32 *processed)) override {
        return ReadWithRequest(data, size, processed, nullptr);
    }
    Z7_COM7F_IMF(ReadWithRequest(void *data, UInt32 size, UInt32 *processed, IProgress *request))
    override {
        if (processed)
            *processed = 0;
        if (request) {
            ObservedRequest = true;
            ObservedWorker = QThread::currentThread() != OwnerThread;
            if (CancelAtRead)
                *CancelAtRead = true;
            RINOK(request->SetCompleted(nullptr))
        }
        if (Failure != S_OK)
            return Failure;
        if (size && !Remaining)
            return E_ABORT;
        UInt32 actual = 0;
        CMyComPtr<IArchiveReadStream> requested;
        source_->QueryInterface(IID_IArchiveReadStream, (void **)&requested);
        const auto amount = UInt32(qMin<UInt64>(size, Remaining));
        const HRESULT result = requested
                                   ? requested->ReadWithRequest(data, amount, &actual, request)
                                   : source_->Read(data, amount, &actual);
        Remaining -= actual;
        if (processed)
            *processed = actual;
        return result;
    }
    Z7_COM7F_IMF(Seek(Int64 offset, UInt32 origin, UInt64 *position)) override {
        return SeekWithRequest(offset, origin, position, nullptr);
    }
    Z7_COM7F_IMF(SeekWithRequest(Int64 offset, UInt32 origin, UInt64 *position, IProgress *request))
    override {
        CMyComPtr<IArchiveReadStream> requested;
        source_->QueryInterface(IID_IArchiveReadStream, (void **)&requested);
        return requested ? requested->SeekWithRequest(offset, origin, position, request)
                         : source_->Seek(offset, origin, position);
    }
};

HRESULT Run(const std::function<HRESULT()> &action) {
    auto worker = std::async(std::launch::async, action);
    while (worker.wait_for(std::chrono::milliseconds(0)) != std::future_status::ready)
        QTest::qWait(1);
    return worker.get();
}

HRESULT ItemStream(IFolderFolder *folder, UInt32 index, IInStream **stream) {
    return Run([&]() -> HRESULT {
        CMyComPtr<IInArchiveGetStream> reader;
        RINOK(folder->QueryInterface(IID_IInArchiveGetStream, (void **)&reader))
        CMyComPtr<ISequentialInStream> sequential;
        RINOK(reader->GetStream(index, &sequential))
        if (!sequential)
            return E_NOTIMPL;
        return sequential.QueryInterface(IID_IInStream, stream);
    });
}

HRESULT CloseReader(IFolderFolder *folder) {
    auto *agent = dynamic_cast<CAgentFolder *>(folder);
    auto *resource =
        agent ? dynamic_cast<QtArchiveFileResource *>(agent->GetEngineResource()) : nullptr;
    return resource ? resource->CloseNativeOpen() : E_FAIL;
}
} // namespace

class ArchiveLayerChainTests final : public QObject {
    Q_OBJECT
    QString engine_, module_;
    QJsonObject manifest_;
    QByteArray fixture(const QString &name) {
        QProcess decoder;
        decoder.start(engine_, {"x", "-so", "--", QStringLiteral(LAYER_FIXTURE_DIR) + '/' + name});
        if (!decoder.waitForFinished(10000) || decoder.exitCode())
            return {};
        return decoder.readAllStandardOutput();
    }
  private slots:
    void initTestCase() {
#ifndef SEVENZIP_CODEC_MODULE
        QSKIP("Official module unavailable; container chain unverified");
#else
        module_ = QStringLiteral(SEVENZIP_CODEC_MODULE);
        engine_ = QFileInfo(module_).dir().filePath(
#ifdef Q_OS_WIN
            "7z.exe"
#else
            "7z"
#endif
        );
        QFile manifest(QStringLiteral(LAYER_FIXTURE_DIR "/manifest.json"));
        QVERIFY(manifest.open(QIODevice::ReadOnly));
        manifest_ = QJsonDocument::fromJson(manifest.readAll()).object();
#endif
    }
    void selectedFileThroughQcowPartitionAndFilesystem_data() {
        QTest::addColumn<QString>("image");
        QTest::addColumn<QString>("mode");
        for (const auto &image : manifest_["images"].toArray())
            for (const QString &mode :
                 {"success", "cancel", "source-failure", "read-cancel", "automatic", "tar"})
                QTest::newRow(qPrintable(image.toObject()["file"].toString() + '-' + mode))
                    << image.toObject()["file"].toString() << mode;
    }
    void streamFactoryReadsParentWithCurrentRequest() {
        QProcess generator;
        generator.start(
            QString::fromUtf8(SEVENZIP_EXT_PYTHON),
            {QString::fromUtf8(SEVENZIP_EXT_FIXTURE), "extract-extent-tree", "--bundle"});
        QVERIFY(generator.waitForFinished(10000));
        QCOMPARE(generator.exitCode(), 0);
        const auto record = QJsonDocument::fromJson(generator.readAllStandardOutput()).object();
        QByteArray packed(4, '\0');
        qToBigEndian(quint32(record["size"].toInt()), packed.data());
        packed += QByteArray::fromBase64(record["image"].toString().toLatin1());
        const QByteArray bytes = qUncompress(packed);
        QVERIFY(!bytes.isEmpty());
        auto *buffer = new CBufferInStream;
        CMyComPtr<IInStream> memory = buffer;
        buffer->Buf.CopyFrom(reinterpret_cast<const Byte *>(bytes.constData()), bytes.size());
        buffer->Init();
        auto *fault = new CBudgetInput(memory);
        CMyComPtr<IInStream> source = fault;
        ArchiveBackend backend(nullptr, engine_);
        QSignalSpy ready(&backend, &ArchiveBackend::codecCatalogReady);
        QVERIFY(backend.discoverCodecCatalog(module_));
        QTRY_COMPARE(ready.size(), 1);
        QVERIFY2(ready[0][0].toBool(), qPrintable(ready[0][1].toString()));
        CMyComPtr<IFolderManager> manager = new QtArchiveFolderManager(backend);
        CMyComPtr<IFolderFolder> folder;
        QCOMPARE(Run([&] {
                     return manager->OpenFolderFile(source, L"virtual.ext", L"Ext", &folder,
                                                    nullptr);
                 }),
                 S_OK);
        QCOMPARE(folder->LoadItems(), S_OK);
        UInt32 count, index = UInt32(-1);
        QCOMPARE(folder->GetNumberOfItems(&count), S_OK);
        for (UInt32 i = 0; i < count; ++i) {
            NWindows::NCOM::CPropVariant name;
            QCOMPARE(folder->GetProperty(i, kpidName, &name), S_OK);
            if (name.vt == VT_BSTR &&
                QString::fromWCharArray(name.bstrVal) == record["name"].toString())
                index = i;
        }
        QVERIFY(index != UInt32(-1));
        CMyComPtr<IArchiveGetStreamWithRequest> factory;
        QCOMPARE(folder.QueryInterface(IID_IArchiveGetStreamWithRequest, &factory), S_OK);
        auto *control = new CArchiveCopyCallback;
        CMyComPtr<IProgress> request = static_cast<IFolderOperationsExtractCallback *>(control);
        fault->ObservedRequest = false;
        fault->CancelAtRead = &control->Cancel;
        CMyComPtr<ISequentialInStream> stream;
        QCOMPARE(Run([&] { return factory->GetStreamWithRequest(index, &stream, request); }),
                 E_ABORT);
        QVERIFY(fault->ObservedRequest && control->Cancel);
        QVERIFY(!stream);
        // The interactive caller must also acquire mappings in its joined
        // worker, not on the UI thread or after a stale opening callback expires.
        fault->CancelAtRead = nullptr;
        fault->ObservedRequest = fault->ObservedWorker = false;
        fault->Failure = E_ABORT;
        bool passwordDefined = false;
        UString password;
        QCOMPARE(
            GetArchiveItemStream(folder, index, &stream, nullptr, {}, passwordDefined, password),
            E_ABORT);
        QVERIFY(fault->ObservedRequest && fault->ObservedWorker);
        QVERIFY(!stream);
        fault->Failure = S_OK;
        QCOMPARE(
            GetArchiveItemStream(folder, index, &stream, nullptr, {}, passwordDefined, password),
            S_OK);
        QVERIFY(stream);
        const auto payload = QByteArray::fromHex(record["payload"].toString().toLatin1());
        QVERIFY(!payload.isEmpty());
        QByteArray actual(37, '\0');
        UInt32 processed = 0;
        QCOMPARE(Run([&] { return stream->Read(actual.data(), actual.size(), &processed); }), S_OK);
        QCOMPARE(actual.left(processed), payload.left(processed));
        QVERIFY(processed > 0);
    }
    void selectedFileThroughQcowPartitionAndFilesystem() {
        QFETCH(QString, image);
        QFETCH(QString, mode);
        const QByteArray bytes = fixture(image);
        QVERIFY(!bytes.isEmpty());
        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        const QString path = directory.filePath(QStringLiteral("диск 空 with spaces.qcow2"));
        QFile original(path);
        QVERIFY(original.open(QIODevice::WriteOnly));
        QCOMPARE(original.write(bytes), qint64(bytes.size()));
        original.close();
        ArchiveBackend backend(nullptr, engine_);
        QSignalSpy ready(&backend, &ArchiveBackend::codecCatalogReady);
        QVERIFY(backend.discoverCodecCatalog(module_));
        QTRY_COMPARE_WITH_TIMEOUT(ready.size(), 1, 15000);
        QVERIFY2(ready[0][0].toBool(), qPrintable(ready[0][1].toString()));
        CMyComPtr<IFolderManager> manager = new QtArchiveFolderManager(backend);
        auto *file = new CInFileStream;
        CMyComPtr<IInStream> native = file;
        QVERIFY(file->Open(us2fs(path.toStdWString().c_str())));
        bool released = false;
        CMyComPtr<IFolderFolder> root, table, filesystem, selected, compressed;
        const auto open = [&](IInStream *input, const wchar_t *name, const wchar_t *format,
                              CMyComPtr<IFolderFolder> &folder, IProgress *progress = nullptr) {
            return Run(
                [&] { return manager->OpenFolderFile(input, name, format, &folder, progress); });
        };
        COpenMeter control, nested;
        auto *opening = new COpenCallbackImp;
        CMyComPtr<IProgress> progress = opening;
        opening->Callback = &nested;
        if (mode == "tar") {
            opening->Callback = &control;
            CMyComPtr<IFolderFolder> reference;
            QCOMPARE(open(nullptr, path.toStdWString().c_str(), L"QCOW", reference, progress),
                     S_OK);
            reference.Release();
            opening->Callback = &nested;
            QProcess producer;
            producer.setWorkingDirectory(directory.path());
            producer.start(engine_, {"a", "-ttar", "outer.tar", QFileInfo(path).fileName()});
            QVERIFY(producer.waitForFinished(30000));
            QCOMPARE(producer.exitCode(), 0);
            const auto zipPath = directory.filePath("outer.tar").toStdWString();
            QCOMPARE(open(nullptr, zipPath.c_str(), L"tar", compressed), S_OK);
            native.Release();
            QCOMPARE(ItemStream(compressed, 0, &native), S_OK);
        }
        auto *fault = new CBudgetInput(native, &released);
        CMyComPtr<IInStream> rootInput = fault;
        native.Release();
        CMyComPtr<IInStream> disk, diskInput, partition, partitionInput;
        if (mode == "automatic") {
            QCOMPARE(open(rootInput, path.toStdWString().c_str(), L"*:r", filesystem), S_OK);
        } else {
            QCOMPARE(open(rootInput, path.toStdWString().c_str(), L"QCOW", root, progress), S_OK);
            if (mode == "tar") {
                // Same QCOW metadata workload, independent of parent transport:
                // parent stream must not replace its progress with TAR counters.
                QCOMPARE(nested.Total, control.Total);
                QCOMPARE(nested.Completed, control.Completed);
            }
            auto *agent = dynamic_cast<CAgentFolder *>(root.Interface());
            QVERIFY(agent);
            NWindows::NCOM::CPropVariant method;
            QCOMPARE(agent->GetArchiveProperties()->GetArchiveProperty(kpidMethod, &method), S_OK);
            if (image.startsWith("deflate") || image.startsWith("zstd")) {
                QCOMPARE(method.vt, VARTYPE(VT_BSTR));
                QCOMPARE(QString::fromWCharArray(method.bstrVal), image.startsWith("zstd")
                                                                      ? QStringLiteral("ZSTD")
                                                                      : QStringLiteral("Deflate"));
            } else
                QCOMPARE(method.vt, VARTYPE(VT_EMPTY));
            QCOMPARE(ItemStream(root, 0, &disk), S_OK);
            CMyComPtr<IArchiveReadStream> requested;
            QCOMPARE(disk.QueryInterface(IID_IArchiveReadStream, &requested), S_OK);
            auto *probe = new CArchiveCopyCallback;
            CMyComPtr<IProgress> request = static_cast<IFolderOperationsExtractCallback *>(probe);
            UInt64 tail = 999;
            probe->Cancel = true;
            QCOMPARE(Run([&] {
                         return requested->SeekWithRequest(-512, STREAM_SEEK_END, &tail, request);
                     }),
                     E_ABORT);
            QCOMPARE(tail, UInt64(999));
            probe->Cancel = false;
            QCOMPARE(Run([&] {
                         return requested->SeekWithRequest(-512, STREAM_SEEK_END, &tail, request);
                     }),
                     S_OK);
            QCOMPARE(tail + 512, UInt64(manifest_["virtual_size"].toDouble()));
            char zeroes[512];
            UInt32 read = 123;
            probe->Cancel = true;
            const auto remaining = fault->Remaining;
            QCOMPARE(Run([&] {
                         return requested->ReadWithRequest(zeroes, sizeof(zeroes), &read, request);
                     }),
                     E_ABORT);
            QCOMPARE(read, UInt32(0));
            QCOMPARE(fault->Remaining, remaining);
            // Sparse zero-fill performs no parent read: cancellation must be
            // checked at this layer, and leave the logical cursor unchanged.
            UInt64 position = 0;
            QCOMPARE(disk->Seek(0, STREAM_SEEK_CUR, &position), S_OK);
            QCOMPARE(position, tail);
            CMyComPtr<IInStream> peer;
            QCOMPARE(ItemStream(root, 0, &peer), S_OK);
            char signature[512];
            QCOMPARE(Run([&] { return peer->Read(signature, sizeof(signature), &read); }), S_OK);
            QCOMPARE(read, UInt32(sizeof(signature)));
            QCOMPARE(QByteArray(signature + 510, 2), QByteArray::fromHex("55aa"));
            peer.Release();
            probe->Cancel = false;
            const auto beforeSparse = fault->Remaining;
            QCOMPARE(Run([&] {
                         return requested->ReadWithRequest(zeroes, sizeof(zeroes), &read, request);
                     }),
                     S_OK);
            QCOMPARE(fault->Remaining, beforeSparse);
            QCOMPARE(QByteArray(zeroes, int(read)), QByteArray(512, '\0'));
            requested.Release();
            diskInput = new CBudgetInput(disk);
            QCOMPARE(open(diskInput, L"virtual disk.mbr", L"MBR", table), S_OK);
            QCOMPARE(ItemStream(table, 0, &partition), S_OK);
            partitionInput = new CBudgetInput(partition);
            QCOMPARE(open(partitionInput, L"selected partition.ext", L"Ext", filesystem), S_OK);
        }
        QCOMPARE(filesystem->BindToFolder(L"папка 空", &selected), S_OK);
        QVERIFY(selected);
        QCOMPARE(selected->LoadItems(), S_OK);
        UInt32 count = 0;
        QCOMPARE(selected->GetNumberOfItems(&count), S_OK);
        QCOMPARE(count, UInt32(1));
        CMyComPtr<IInStream> leaf;
        QCOMPARE(ItemStream(selected, 0, &leaf), S_OK);
        CMyComPtr<IArchiveLayerSource> context;
        QCOMPARE(leaf.QueryInterface(IID_IArchiveLayerSource, &context), S_OK);
        UInt64 depth;
        QCOMPARE(context->GetContainingContainerDepth(&depth), S_OK);
        QCOMPARE(depth, UInt64(mode == "tar" ? 4 : 3));
        context.Release();
        if (root)
            QCOMPARE(CloseReader(root), S_OK);
        if (table)
            QCOMPARE(CloseReader(table), S_OK);
        root.Release();
        table.Release();
        rootInput.Release();
        // Put QCOW's optional compressed-cluster cache on the MBR before
        // injecting a source failure, rather than faulting already cached data.
        char header[512];
        UInt32 actual = 0;
        if (disk) {
            QCOMPARE(disk->Seek(0, STREAM_SEEK_SET, nullptr), S_OK);
            QCOMPARE(disk->Read(header, sizeof(header), &actual), S_OK);
        }
        disk.Release();
        diskInput.Release();
        partition.Release();
        partitionInput.Release();
        QVERIFY(!released);
        CMyComPtr<IFolderOperations> operations;
        QCOMPARE(selected.QueryInterface(IID_IFolderOperations, &operations), S_OK);
        auto *observer = new CArchiveCopyCallback;
        CMyComPtr<IFolderOperationsExtractCallback> callback = observer;
        observer->Cancel = mode == "cancel";
        fault->ObservedRequest = false;
        fault->CancelAtRead = mode == "read-cancel" ? &observer->Cancel : nullptr;
        fault->Failure = mode == "source-failure" ? E_FAIL : S_OK;
        const QString output = directory.filePath("selected output");
        QVERIFY(QDir().mkpath(output));
        const auto prefix = (output + '/').toStdWString();
        const UInt32 index = 0;
        const auto copy = [&] {
            return Run(
                [&] { return operations->CopyTo(0, &index, 1, 0, 0, prefix.c_str(), callback); });
        };
        const HRESULT result = copy();
        if (mode == "cancel" || mode == "read-cancel")
            QCOMPARE(result, E_ABORT);
        else if (mode == "source-failure")
            QVERIFY(result != S_OK || observer->Errors);
        if (mode == "read-cancel")
            QVERIFY(fault->ObservedRequest);
        if (mode == "cancel" || mode == "source-failure" || mode == "read-cancel") {
            fault->Failure = S_OK;
            fault->CancelAtRead = nullptr;
            observer = new CArchiveCopyCallback;
            callback = observer;
            QCOMPARE(copy(), S_OK);
        } else
            QCOMPARE(result, S_OK);
        QCOMPARE(observer->Errors, 0u);
        const QString name = manifest_["path"].toString().section('/', -1);
        QFile copied(QDir(output).filePath(name));
        QVERIFY(copied.open(QIODevice::ReadOnly));
        QCOMPARE(copied.readAll(), manifest_["payload"].toString().toUtf8());
        QCOMPARE(QDir(output).entryList(QDir::Files).size(), 1);
        QVERIFY(original.open(QIODevice::ReadOnly));
        QCOMPARE(original.readAll(), bytes);
        operations.Release();
        selected.Release();
        filesystem.Release();
        leaf.Release();
        QVERIFY(released);
    }
};
QTEST_MAIN(ArchiveLayerChainTests)
#include "archive_layer_chain_tests.moc"
