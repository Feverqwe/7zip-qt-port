#include "7zip/Common/StreamObjects.h"
#include "archive_reader_fixture.h"
#include "archivenativeopen.h"
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QtEndian>

namespace {
class CCancelOnRead final : public IInStream, public CMyUnknownImp {
    Z7_COM_UNKNOWN_IMP_2(IInStream, ISequentialInStream)
  public:
    CMyComPtr<IInStream> Input;
    bool Armed = false, Canceled = false;
    Z7_COM7F_IMF(Read(void *data, UInt32 size, UInt32 *processed)) override {
        if (Armed && size)
            Canceled = true;
        return Input->Read(data, size, processed);
    }
    Z7_COM7F_IMF(Seek(Int64 offset, UInt32 origin, UInt64 *position)) override {
        return Input->Seek(offset, origin, position);
    }
};
class CSnapshotOutput final : public ISequentialOutStream, public CMyUnknownImp {
    Z7_COM_UNKNOWN_IMP_1(ISequentialOutStream)
  public:
    QByteArray *Bytes;
    HRESULT Failure;
    Z7_COM7F_IMF(Write(const void *data, UInt32 size, UInt32 *processed)) override {
        if (processed)
            *processed = 0;
        if (Failure != S_OK)
            return Failure;
        Bytes->append(static_cast<const char *>(data), qsizetype(size));
        if (processed)
            *processed = size;
        return S_OK;
    }
};
class CSnapshotExtract final : public IArchiveExtractCallback, public CMyUnknownImp {
    Z7_COM_UNKNOWN_IMP_1(IArchiveExtractCallback)
  public:
    QMap<UInt32, QByteArray> Bytes;
    QMap<UInt32, Int32> Results;
    UInt32 Index = 0;
    UInt64 Total = 0, Completed = 0;
    HRESULT Failure = S_OK;
    const bool *Canceled = nullptr;
    Z7_COM7F_IMF(SetTotal(UInt64 total)) override {
        Total = total;
        return S_OK;
    }
    Z7_COM7F_IMF(SetCompleted(const UInt64 *value)) override {
        if (Canceled && *Canceled)
            return E_ABORT;
        if (value)
            Completed = *value;
        return S_OK;
    }
    Z7_COM7F_IMF(GetStream(UInt32 index, ISequentialOutStream **stream, Int32 mode)) override {
        Index = index;
        *stream = nullptr;
        if (mode == NArchive::NExtract::NAskMode::kExtract) {
            auto *output = new CSnapshotOutput;
            output->Bytes = &Bytes[index];
            output->Failure = Failure;
            CMyComPtr<ISequentialOutStream> owner = output;
            *stream = owner.Detach();
        }
        return S_OK;
    }
    Z7_COM7F_IMF(PrepareOperation(Int32)) override { return S_OK; }
    Z7_COM7F_IMF(SetOperationResult(Int32 result)) override {
        Results[Index] = result;
        return S_OK;
    }
};
} // namespace

class ArchiveQcowSnapshotsTests final : public ArchiveReaderFixture {
    Q_OBJECT
    QJsonArray sets_;
    QByteArray Inflate(const QString &name) {
        QProcess process;
        process.start(engine_, {"x", "-so", "--", QStringLiteral(LAYER_FIXTURE_DIR "/") + name});
        if (!process.waitForFinished(10000) || process.exitCode())
            return {};
        return process.readAllStandardOutput();
    }
    QByteArray Image(const QJsonObject &set) {
        return Inflate(set["files"].toObject()["image"].toString());
    }
    QByteArray Expected(const QJsonObject &set, unsigned index) {
        return Inflate(set["files"].toObject()[QString::number(index)].toString());
    }
    bool PutBacking(const QString &path, const QJsonObject &set) {
        const auto backing = set["files"].toObject()["backing"].toString();
        return backing.isEmpty() ||
               Put(QFileInfo(path).dir().filePath("source.raw"), Inflate(backing));
    }
    bool BackingUnchanged(const QString &path, const QJsonObject &set) {
        const auto backing = set["files"].toObject()["backing"].toString();
        return backing.isEmpty() ||
               Bytes(QFileInfo(path).dir().filePath("source.raw")) == Inflate(backing);
    }
    static HRESULT ReadAll(IInStream *stream, QByteArray &bytes) {
        qsizetype done = 0;
        while (done < bytes.size()) {
            UInt32 count = 0;
            RINOK(stream->Read(bytes.data() + done,
                               UInt32(qMin(qsizetype(8191), bytes.size() - done)), &count))
            if (!count)
                return E_FAIL;
            done += count;
        }
        return S_OK;
    }
  private slots:
    void initTestCase() {
        InitializeRuntime();
        QFile file(QStringLiteral(LAYER_FIXTURE_DIR "/manifest.json"));
        QVERIFY(file.open(QIODevice::ReadOnly));
        sets_ = QJsonDocument::fromJson(file.readAll()).object()["snapshot_sets"].toArray();
        QVERIFY(!sets_.isEmpty());
    }
    void independentStreams_data() {
        QTest::addColumn<QJsonObject>("set");
        for (const auto &set : sets_)
            QTest::newRow(qPrintable(set.toObject()["name"].toString())) << set.toObject();
    }
    void independentStreams() {
        QFETCH(QJsonObject, set);
        const auto image = Image(set);
        QVERIFY(!image.isEmpty());
        QTemporaryDir dir;
        const auto path = dir.filePath(QStringLiteral("снимки 空.qcow2"));
        QVERIFY(Put(path, image));
        QVERIFY(PutBacking(path, set));
        std::shared_ptr<ArchiveNativeOpen> reader;
        QCOMPARE(Worker([&] { return backend_->openNativeArchive(path, "QCOW", nullptr, reader); }),
                 S_OK);
        UInt32 count = 0;
        QCOMPARE(reader->arc().Archive->GetNumberOfItems(&count), S_OK);
        QCOMPARE(count, UInt32(3));
        NWindows::NCOM::CPropVariant main;
        QCOMPARE(reader->arc().Archive->GetArchiveProperty(kpidMainSubfile, &main), S_OK);
        QCOMPARE(main.vt, VARTYPE(VT_EMPTY));
        CMyComPtr<IInArchiveGetStream> factory;
        QCOMPARE(reader->arc().Archive.QueryInterface(IID_IInArchiveGetStream, &factory), S_OK);
        QVector<QByteArray> expected;
        CMyComPtr<IInStream> streams[4];
        for (unsigned i = 0; i < 4; ++i) {
            const unsigned index = i % 3;
            expected.append(Expected(set, index));
            QVERIFY(!expected.back().isEmpty());
            NWindows::NCOM::CPropVariant size, name;
            QCOMPARE(reader->arc().Archive->GetProperty(index, kpidSize, &size), S_OK);
            QCOMPARE(size.vt, VARTYPE(VT_UI8));
            QCOMPARE(size.uhVal.QuadPart, UInt64(expected.back().size()));
            QCOMPARE(reader->arc().Archive->GetProperty(index, kpidPath, &name), S_OK);
            QCOMPARE(name.vt, VARTYPE(VT_BSTR));
            QCOMPARE(QString::fromWCharArray(name.bstrVal),
                     index ? QString("snapshot-%1.img").arg(index) : QString("current.img"));
            if (index) {
                NWindows::NCOM::CPropVariant comment, time;
                QCOMPARE(reader->arc().Archive->GetProperty(index, kpidComment, &comment), S_OK);
                QCOMPARE(comment.vt, VARTYPE(VT_BSTR));
                QCOMPARE(QString::fromWCharArray(comment.bstrVal),
                         index == 1 ? QString::fromUtf8("../../снимок 空") : QString("second"));
                QCOMPARE(reader->arc().Archive->GetProperty(index, kpidMTime, &time), S_OK);
                QCOMPARE(time.vt, VARTYPE(VT_FILETIME));
                const UInt64 ticks =
                    (UInt64(time.filetime.dwHighDateTime) << 32) | time.filetime.dwLowDateTime;
                QCOMPARE(
                    QDateTime(QDate(1601, 1, 1), QTime(0, 0), QTimeZone::UTC)
                        .addMSecs(qint64(ticks / 10000)),
                    QDateTime::fromSecsSinceEpoch(1700000000 + (index - 1) * 3600, QTimeZone::UTC)
                        .addMSecs(123));
            }
            CMyComPtr<ISequentialInStream> sequential;
            QCOMPARE(Worker([&] { return factory->GetStream(index, &sequential); }), S_OK);
            QVERIFY(sequential);
            QCOMPARE(sequential.QueryInterface(IID_IInStream, &streams[i]), S_OK);
        }
        CMyComPtr<IInStream> direct;
        QCOMPARE(reader->arc().Archive.QueryInterface(IID_IInStream, &direct), S_OK);
        // Two opens of current + both differently sized snapshots coexist,
        // including the original image handler's directly exposed stream.
        for (UInt64 pos : {UInt64(4093), UInt64(0), UInt64(32767), UInt64(65535)}) {
            for (unsigned i : {0u, 2u, 1u, 3u}) {
                QCOMPARE(streams[i]->Seek(Int64(pos), STREAM_SEEK_SET, nullptr), S_OK);
                char data[513];
                UInt32 actual = 0;
                QCOMPARE(streams[i]->Read(data, sizeof(data), &actual), S_OK);
                QVERIFY(actual > 0);
                QCOMPARE(QByteArray(data, actual), expected[int(i)].mid(qsizetype(pos), actual));
            }
            QCOMPARE(direct->Seek(Int64(pos), STREAM_SEEK_SET, nullptr), S_OK);
            char data[513];
            UInt32 actual = 0;
            QCOMPARE(direct->Read(data, sizeof(data), &actual), S_OK);
            QVERIFY(actual > 0);
            QCOMPARE(QByteArray(data, actual), expected[0].mid(qsizetype(pos), actual));
        }
        // The archive may close before retained streams are consumed.
        factory.Release();
        direct.Release();
        QCOMPARE(reader->arc().Archive->Close(), S_OK);
        reader.reset();
        for (unsigned i = 0; i < 4; ++i) {
            QCOMPARE(streams[i]->Seek(0, STREAM_SEEK_SET, nullptr), S_OK);
            QByteArray actual(expected[int(i)].size(), Qt::Uninitialized);
            QCOMPARE(Worker([&] { return ReadAll(streams[i], actual); }), S_OK);
            QCOMPARE(actual, expected[int(i)]);
        }
        QCOMPARE(Bytes(path), image);
        QVERIFY(BackingUnchanged(path, set));
    }
    void extractSnapshots_data() {
        QTest::addColumn<QJsonObject>("set");
        QTest::addColumn<int>("result");
        for (const auto &set : sets_)
            for (int result : {int(S_OK), int(E_ABORT), int(E_FAIL)})
                QTest::newRow(
                    qPrintable(set.toObject()["name"].toString() + QString::number(result)))
                    << set.toObject() << result;
    }
    void extractSnapshots() {
        QFETCH(QJsonObject, set);
        QFETCH(int, result);
        const auto image = Image(set);
        QVERIFY(!image.isEmpty());
        QTemporaryDir dir;
        const auto path = dir.filePath(QStringLiteral("извлечь снимки.qcow2"));
        QVERIFY(Put(path, image));
        QVERIFY(PutBacking(path, set));
        std::shared_ptr<ArchiveNativeOpen> reader;
        QCOMPARE(Worker([&] { return backend_->openNativeArchive(path, "QCOW", nullptr, reader); }),
                 S_OK);
        auto *callback = new CSnapshotExtract;
        CMyComPtr<IArchiveExtractCallback> owner = callback;
        callback->Failure = HRESULT(result);
        // Deliberately use a non-ascending selection to check index forwarding.
        const UInt32 selected[] = {2, 0, 1};
        QCOMPARE(
            Worker([&] { return reader->arc().Archive->Extract(selected, 3, false, callback); }),
            HRESULT(result));
        if (result == int(S_OK)) {
            UInt64 total = 0;
            for (unsigned i = 0; i < 3; ++i) {
                const auto expected = Expected(set, i);
                QCOMPARE(callback->Bytes[i], expected);
                QCOMPARE(callback->Results.value(i, -1),
                         Int32(NArchive::NExtract::NOperationResult::kOK));
                total += UInt64(expected.size());
            }
            QCOMPARE(callback->Total, total);
            QCOMPARE(callback->Completed, total);
            callback->Results.clear();
            QCOMPARE(Worker([&] {
                         return reader->arc().Archive->Extract(nullptr, UInt32(-1), true, callback);
                     }),
                     S_OK);
            QCOMPARE(callback->Results.size(), 3);
        } else
            QVERIFY(callback->Results.isEmpty());
        QCOMPARE(Bytes(path), image);
        QVERIFY(BackingUnchanged(path, set));
    }
    void fileThroughSelectedSnapshotPartitionAndFilesystem() {
        QJsonObject set;
        for (const auto &entry : sets_)
            if (entry.toObject()["name"] == "filesystem")
                set = entry.toObject();
        QVERIFY(!set.isEmpty());
        const auto image = Image(set);
        QVERIFY(!image.isEmpty());
        QTemporaryDir dir;
        const auto path = dir.filePath(QStringLiteral("снимок файловой системы.qcow2"));
        QVERIFY(Put(path, image));
        std::shared_ptr<ArchiveNativeOpen> reader;
        QCOMPARE(Worker([&] { return backend_->openNativeArchive(path, "QCOW", nullptr, reader); }),
                 S_OK);
        const auto getStream = [](IUnknown *object, UInt32 index, CMyComPtr<IInStream> &input) {
            return Worker([&]() -> HRESULT {
                CMyComPtr<IInArchiveGetStream> factory;
                RINOK(object->QueryInterface(IID_IInArchiveGetStream, (void **)&factory))
                CMyComPtr<ISequentialInStream> stream;
                RINOK(factory->GetStream(index, &stream))
                if (!stream)
                    return E_FAIL;
                return stream.QueryInterface(IID_IInStream, &input);
            });
        };
        CMyComPtr<IInStream> disk, partition, file;
        QCOMPARE(getStream(reader->arc().Archive, 1, disk), S_OK);
        CMyComPtr<IFolderFolder> mbr, ext, selected;
        QCOMPARE(Worker([&] {
                     return manager_->OpenFolderFile(disk, L"snapshot.img", L"MBR", &mbr, nullptr);
                 }),
                 S_OK);
        QCOMPARE(getStream(mbr, 0, partition), S_OK);
        QCOMPARE(Worker([&] {
                     return manager_->OpenFolderFile(partition, L"partition.ext", L"Ext", &ext,
                                                     nullptr);
                 }),
                 S_OK);
        QCOMPARE(ext->BindToFolder(L"папка 空", &selected), S_OK);
        QVERIFY(selected);
        QCOMPARE(selected->LoadItems(), S_OK);
        QCOMPARE(getStream(selected, 0, file), S_OK);
        const QByteArray expected("device ext4 payload\n");
        QByteArray payload(expected.size(), Qt::Uninitialized);
        QCOMPARE(Worker([&] { return ReadAll(file, payload); }), S_OK);
        QCOMPARE(payload, expected);
        QCOMPARE(Bytes(path), image);
    }
    void cancellationWhileLoadingSnapshot() {
        const auto set = sets_[1].toObject();
        const auto image = Image(set);
        QVERIFY(!image.isEmpty());
        QTemporaryDir dir;
        const auto path = dir.filePath("cancel-snapshot.qcow2");
        QVERIFY(Put(path, image));
        auto *memory = new CBufferInStream;
        CMyComPtr<IInStream> input = memory;
        memory->Buf.CopyFrom(reinterpret_cast<const Byte *>(image.constData()),
                             size_t(image.size()));
        memory->Init();
        auto *source = new CCancelOnRead;
        CMyComPtr<IInStream> sourceOwner = source;
        source->Input = input;
        COpenOptions options;
        CObjectVector<COpenType> types;
        CIntVector excluded;
        options.types = &types;
        options.excludedFormats = &excluded;
        options.props = nullptr;
        options.stream = source;
        std::shared_ptr<ArchiveNativeOpen> reader;
        QCOMPARE(Worker([&] {
                     return backend_->openNativeArchive(path, "QCOW", nullptr, reader, &options);
                 }),
                 S_OK);
        auto *callback = new CSnapshotExtract;
        CMyComPtr<IArchiveExtractCallback> owner = callback;
        // Request cancellation during a source read, independently of the
        // number/order of public progress callbacks used while loading tables.
        callback->Canceled = &source->Canceled;
        source->Armed = true;
        const UInt32 index = 1;
        QCOMPARE(Worker([&] { return reader->arc().Archive->Extract(&index, 1, false, callback); }),
                 E_ABORT);
        QVERIFY(source->Canceled);
        QVERIFY(callback->Bytes.isEmpty());
        QVERIFY(callback->Results.isEmpty());
        QCOMPARE(Bytes(path), image);
    }
    void damagedSnapshotDoesNotHideOtherImages() {
        const auto set = sets_[0].toObject();
        auto image = Image(set);
        const auto table = qFromBigEndian<quint64>(image.constData() + 64);
        QVERIFY(table + 40 <= quint64(image.size()));
        // Damage only snapshot 1's L1 pointer, preserving active/snapshot 2 tables.
        qToBigEndian((quint64(image.size()) + 4095) / 4096 * 4096 + 4096, image.data() + table);
        QTemporaryDir dir;
        const auto path = dir.filePath("damaged-snapshot.qcow2");
        QVERIFY(Put(path, image));
        std::shared_ptr<ArchiveNativeOpen> reader;
        QCOMPARE(Worker([&] { return backend_->openNativeArchive(path, "QCOW", nullptr, reader); }),
                 S_OK);
        CMyComPtr<IInArchiveGetStream> factory;
        QCOMPARE(reader->arc().Archive.QueryInterface(IID_IInArchiveGetStream, &factory), S_OK);
        CMyComPtr<ISequentialInStream> damaged;
        QCOMPARE(Worker([&] { return factory->GetStream(1, &damaged); }), S_FALSE);
        QVERIFY(!damaged);
        auto *callback = new CSnapshotExtract;
        CMyComPtr<IArchiveExtractCallback> owner = callback;
        QCOMPARE(Worker([&] {
                     return reader->arc().Archive->Extract(nullptr, UInt32(-1), false, callback);
                 }),
                 S_OK);
        QCOMPARE(callback->Results.value(1, -1),
                 Int32(NArchive::NExtract::NOperationResult::kDataError));
        QVERIFY(callback->Bytes[1].isEmpty());
        for (unsigned index : {0u, 2u}) {
            QCOMPARE(callback->Bytes[index], Expected(set, index));
            QCOMPARE(callback->Results.value(index, -1),
                     Int32(NArchive::NExtract::NOperationResult::kOK));
        }
        QCOMPARE(Bytes(path), image);
    }
    void malformedSnapshotTable_data() {
        QTest::addColumn<int>("mutation");
        QTest::newRow("outside-source") << 0;
        QTest::newRow("excessive-count") << 1;
        QTest::newRow("extra-data-overrun") << 2;
    }
    void malformedSnapshotTable() {
        QFETCH(int, mutation);
        auto image = Image(sets_[0].toObject());
        QVERIFY(image.size() > 104);
        const auto table = qFromBigEndian<quint64>(image.constData() + 64);
        QVERIFY(table + 40 <= quint64(image.size()));
        if (mutation == 0)
            qToBigEndian(quint64(image.size()) + 4096, image.data() + 64);
        else if (mutation == 1)
            qToBigEndian(quint32(65537), image.data() + 60);
        else
            qToBigEndian(quint32(0xffffffff), image.data() + table + 36);
        QTemporaryDir dir;
        const auto path = dir.filePath("damaged.qcow2");
        QVERIFY(Put(path, image));
        std::shared_ptr<ArchiveNativeOpen> reader;
        QVERIFY(Worker([&] {
                    return backend_->openNativeArchive(path, "QCOW", nullptr, reader);
                }) != S_OK);
        QCOMPARE(Bytes(path), image);
    }
};

QTEST_MAIN(ArchiveQcowSnapshotsTests)
#include "archive_qcow_snapshots_tests.moc"
