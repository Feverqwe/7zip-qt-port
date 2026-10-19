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
    explicit Input(QByteArray bytes) : bytes_(std::move(bytes)) {}
    Z7_COM7F_IMF(Read(void *data, UInt32 size, UInt32 *processed)) override {
        if (processed)
            *processed = 0;
        if (failure != S_OK)
            return failure;
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
class Extract final : public IArchiveExtractCallback, public CMyUnknownImp {
    Z7_COM_UNKNOWN_IMP_1(IArchiveExtractCallback)
  public:
    bool cancel = false;
    Int32 result = -1;
    Z7_COM7F_IMF(SetTotal(UInt64)) override { return S_OK; }
    Z7_COM7F_IMF(SetCompleted(const UInt64 *)) override { return cancel ? E_ABORT : S_OK; }
    Z7_COM7F_IMF(GetStream(UInt32, ISequentialOutStream **stream, Int32)) override {
        *stream = nullptr;
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
                QString::fromWCharArray(path.bstrVal) == name)
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
    void loadFixture(const QString &codec, bool xattrs = false) {
        QProcess generator;
        QStringList arguments{QString::fromUtf8(SEVENZIP_SQUASHFS_FIXTURE)};
        arguments << (codec == "zlib" && !xattrs ? "--compressed" : "--codec=" + codec);
        if (xattrs)
            arguments << "--xattrs" << "--export";
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
    void backendDiscoversModuleAndReadsCompressedFiles_data() { codecs(); }
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
};
QTEST_GUILESS_MAIN(SquashfsStreamTests)
#include "squashfs_stream_tests.moc"
