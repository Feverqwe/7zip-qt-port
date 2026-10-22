#include "7zip/Archive/IArchive.h"
#include "Common/MyCom.h"
#include "Windows/PropVariant.h"
#include "archivebackend.h"
#include "archivecodeccatalog.h"
#include "fat_test_support.h"
#include <QDir>
#include <QFile>
#include <QLibrary>
#include <QProcess>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QtEndian>
#include <QtTest>

using namespace FatTestSupport;

class FatModuleTests final : public QObject {
    Q_OBJECT
    QLibrary module_{QString::fromUtf8(SEVENZIP_FAT_MODULE)};
    CMyComPtr<IInArchive> archive_;
    QByteArray bytes_;
    QByteArray fixture(const QString &kind) {
        QProcess process;
        process.start(QString::fromUtf8(SEVENZIP_FAT_PYTHON),
                      {QString::fromUtf8(SEVENZIP_FAT_FIXTURE), "--readonly=" + kind});
        if (!process.waitForFinished(10000) || process.exitCode()) {
            QTest::qFail(process.readAllStandardError().constData(), __FILE__, __LINE__);
            return {};
        }
        return process.readAllStandardOutput();
    }
    void checkLimitations(bool dirty, bool shortSource) {
        UInt32 count = 0;
        QCOMPARE(archive_->GetNumberOfArchiveProperties(&count), S_OK);
        bool found = false;
        for (UInt32 i = 0; i < count; ++i) {
            CMyComBSTR name;
            PROPID id;
            VARTYPE type;
            QCOMPARE(archive_->GetArchivePropertyInfo(i, &name, &id, &type), S_OK);
            if (!name || QString::fromWCharArray(name) != "Read-only limitations")
                continue;
            found = true;
            NWindows::NCOM::CPropVariant value;
            QCOMPARE(archive_->GetArchiveProperty(id, &value), S_OK);
            QCOMPARE(value.vt, VARTYPE(dirty || shortSource ? VT_BSTR : VT_EMPTY));
            const QString text =
                value.vt == VT_BSTR ? QString::fromWCharArray(value.bstrVal) : QString();
            QCOMPARE(text.contains("not properly unmounted"), dirty);
            QCOMPARE(text.contains("shorter than"), shortSource);
        }
        QVERIFY(found);
    }
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
    void volumeIdentityDoesNotDependOnBootstrap_data() {
        QTest::addColumn<int>("bits");
        QTest::addColumn<QByteArray>("bootstrap");
        QTest::addColumn<int>("signature");
        QTest::addColumn<quint32>("identity");
        for (int bits : {12, 16, 32}) {
            for (const QByteArray &bootstrap :
                 {QByteArray::fromHex("eb5890"), QByteArray::fromHex("000000"),
                  QByteArray::fromHex("eb0090"), QByteArray::fromHex("e90000")})
                for (int signature : {0, 0x28, 0x29})
                    QTest::newRow(qPrintable(QString::number(bits) + '-' + bootstrap.toHex() + '-' +
                                             QString::number(signature)))
                        << bits << bootstrap << signature << quint32(0xfedcba98);
            QTest::newRow(qPrintable("zero-" + QString::number(bits)))
                << bits << QByteArray(3, '\0') << 0 << quint32(0);
        }
    }
    void volumeIdentityDoesNotDependOnBootstrap() {
        QFETCH(int, bits);
        QFETCH(QByteArray, bootstrap);
        QFETCH(int, signature);
        QFETCH(quint32, identity);
        auto bytes = fixture("clean" + QString::number(bits));
        QVERIFY(!bytes.isEmpty());
        bytes.replace(0, 3, bootstrap);
        const int extended = bits == 32 ? 66 : 38;
        bytes[extended] = char(signature);
        qToLittleEndian(identity, bytes.data() + extended + 1);
        QCOMPARE(open(bytes), S_OK);
        NWindows::NCOM::CPropVariant actual;
        QCOMPARE(archive_->GetArchiveProperty(kpidId, &actual), S_OK);
        QCOMPARE(actual.vt, VARTYPE(VT_UI4));
        QCOMPARE(actual.ulVal, identity);
    }
    void fat16CapacityIsAppliedBeforeMaximumClusterCheck() {
        for (const QString &kind : {"capped16", "maximum16", "overflow16"}) {
            QProcess generator;
            generator.start(QString::fromUtf8(SEVENZIP_FAT_PYTHON),
                            {QString::fromUtf8(SEVENZIP_FAT_FIXTURE), "--capacity=" + kind});
            QVERIFY(generator.waitForFinished(10000));
            QCOMPARE(generator.exitCode(), 0);
            const auto bytes = generator.readAllStandardOutput();
            QVERIFY(!bytes.isEmpty());
            const bool valid = kind != "overflow16";
            QCOMPARE(open(bytes), valid ? S_OK : S_FALSE);
            if (valid) {
                NWindows::NCOM::CPropVariant filesystem;
                QCOMPARE(archive_->GetArchiveProperty(kpidFileSystem, &filesystem), S_OK);
                QCOMPARE(filesystem.vt, VARTYPE(VT_BSTR));
                QCOMPARE(QString::fromWCharArray(filesystem.bstrVal), QString("FAT16"));
                UInt32 count = 0;
                QCOMPARE(archive_->GetNumberOfItems(&count), S_OK);
                QVERIFY(count >= 1);
                UInt32 id = UINT32_MAX;
                for (UInt32 i = 0; i < count; ++i) {
                    NWindows::NCOM::CPropVariant path;
                    QCOMPARE(archive_->GetProperty(i, kpidPath, &path), S_OK);
                    if (path.vt == VT_BSTR && QString::fromWCharArray(path.bstrVal) ==
                                                  QStringLiteral("данные 空 fragmented.bin"))
                        id = i;
                }
                QVERIFY(id != UINT32_MAX);
                CMyComPtr2<IArchiveExtractCallback, Extract> output;
                output.Create_if_Empty();
                QCOMPARE(archive_->Extract(&id, 1, 0, output), S_OK);
                QCOMPARE(output->result, Int32(NArchive::NExtract::NOperationResult::kOK));
                QByteArray expected;
                for (int i = 0; i < 1024; ++i)
                    expected.append(char(i & 255));
                expected += "fragmented FAT tail\n";
                QCOMPARE(output->output->bytes, expected);
            }
        }
    }
    void initTestCase() {
        QProcess generator;
        generator.start(QString::fromUtf8(SEVENZIP_FAT_PYTHON),
                        {QString::fromUtf8(SEVENZIP_FAT_FIXTURE)});
        QVERIFY(generator.waitForFinished(10000));
        QCOMPARE(generator.exitCode(), 0);
        bytes_ = generator.readAllStandardOutput();
        QVERIFY2(!bytes_.isEmpty(), generator.readAllStandardError().constData());
        createArchive();
    }
    void failuresCancellationAndReuse() {
        CMyComPtr<IArchiveOpenCallback> cancelled = new CancelOpen;
        QCOMPARE(open(bytes_, cancelled), E_ABORT);
        UInt32 count = 99;
        QCOMPARE(archive_->GetNumberOfItems(&count), S_OK);
        QCOMPARE(count, 0u);
        auto *input = new Input(bytes_);
        CMyComPtr<IInStream> source = input;
        input->fail = true;
        QCOMPARE(archive_->Open(source, nullptr, nullptr), E_FAIL);
        input->fail = false;
        QCOMPARE(archive_->Open(source, nullptr, nullptr), S_OK);
        const UInt32 index = 0;
        CMyComPtr2<IArchiveExtractCallback, Extract> callback;
        callback.Create_if_Empty();
        callback->cancel = true;
        QCOMPARE(archive_->Extract(&index, 1, 0, callback), E_ABORT);
        callback->cancel = false;
        callback->output->fail = true;
        QCOMPARE(archive_->Extract(&index, 1, 0, callback), E_FAIL);
        callback->output->fail = false;
        input->fail = true;
        callback->output->bytes.clear();
        callback->results.clear();
        QCOMPARE(archive_->Extract(&index, 1, 0, callback), S_OK);
        QCOMPARE(callback->results,
                 QVector<Int32>({NArchive::NExtract::NOperationResult::kDataError}));
        QVERIFY(callback->output->bytes.isEmpty());
        input->fail = false;
        callback->output->bytes.clear();
        QCOMPARE(archive_->Extract(&index, 1, 0, callback), S_OK);
        QCOMPARE(callback->result, Int32(NArchive::NExtract::NOperationResult::kOK));
        QCOMPARE(callback->output->bytes, payload());
    }
    void limitedVolumesKeepReadableFiles() {
        for (const QString &bits : {"12", "16", "32"}) {
            for (const QString &kind : {"clean", "dirty", "short", "combined"}) {
                const QByteArray data = fixture(kind + bits);
                QVERIFY(!data.isEmpty());
                QCOMPARE(open(data), S_OK);
                checkLimitations(kind == "dirty" || kind == "combined",
                                 kind == "short" || kind == "combined");
                const UInt32 index = 0;
                CMyComPtr2<IArchiveExtractCallback, Extract> callback;
                callback.Create_if_Empty();
                QCOMPARE(archive_->Extract(&index, 1, 0, callback), S_OK);
                QCOMPARE(callback->result, Int32(NArchive::NExtract::NOperationResult::kOK));
                QCOMPARE(callback->output->bytes, payload());
                QCOMPARE(archive_->Close(), S_OK);
                checkLimitations(false, false);
            }
        }
        QCOMPARE(open(fixture("partial32")), S_OK);
        checkLimitations(false, true);
        const UInt32 index = 0;
        CMyComPtr2<IArchiveExtractCallback, Extract> callback;
        callback.Create_if_Empty();
        QCOMPARE(archive_->Extract(&index, 1, 0, callback), S_OK);
        QCOMPARE(callback->result, Int32(NArchive::NExtract::NOperationResult::kDataError));
        QVERIFY(callback->output->bytes != payload());
    }
    void optionalFsInfoFailuresAndReopening() {
        const auto data = fixture("clean32");
        for (bool seek : {false, true})
            for (HRESULT failure : {E_FAIL, E_ABORT, E_OUTOFMEMORY}) {
                auto *input = new Input(data);
                CMyComPtr<IInStream> source = input;
                input->failAt = 512;
                input->failSeek = seek;
                input->failure = failure;
                QCOMPARE(archive_->Open(source, nullptr, nullptr),
                         failure == E_FAIL ? S_OK : failure);
                input->failAt = UINT64_MAX;
                QCOMPARE(archive_->Open(source, nullptr, nullptr), S_OK);
                checkLimitations(false, false);
            }
    }
    void streamSurvivesCloseReopenAndRelease() {
        QCOMPARE(open(bytes_), S_OK);
        CMyComPtr<IInArchiveGetStream> provider;
        QCOMPARE(
            archive_->QueryInterface(IID_IInArchiveGetStream, reinterpret_cast<void **>(&provider)),
            S_OK);
        CMyComPtr<ISequentialInStream> sequential;
        QCOMPARE(provider->GetStream(0, &sequential), S_OK);
        CMyComPtr<IInStream> stream;
        QCOMPARE(sequential->QueryInterface(IID_IInStream, reinterpret_cast<void **>(&stream)),
                 S_OK);
        auto changed = bytes_;
        const auto expected = payload();
        const auto start = changed.indexOf(expected.left(32));
        QVERIFY(start >= 0);
        changed[start] = 'Z';
        QCOMPARE(archive_->Close(), S_OK);
        QCOMPARE(open(changed), S_OK);
        provider.Release();
        archive_.Release();
        sequential.Release();
        for (qint64 offset : {qint64(510), qint64(0), qint64(expected.size() - 17)}) {
            QCOMPARE(stream->Seek(offset, STREAM_SEEK_SET, nullptr), S_OK);
            char bytes[37];
            const auto wanted = expected.mid(offset, sizeof(bytes));
            QByteArray actual;
            while (actual.size() < wanted.size()) {
                UInt32 count = 0;
                QCOMPARE(stream->Read(bytes, UInt32(wanted.size() - actual.size()), &count), S_OK);
                QVERIFY(count > 0);
                actual.append(bytes, count);
            }
            QCOMPARE(actual, wanted);
        }
        createArchive();
    }
    void backendDiscoveryAndLegacyExtraction() {
        QTemporaryDir runtime;
        QVERIFY(runtime.isValid());
#ifdef Q_OS_WIN
        const QString engineName = "7z.exe", coreName = "7z.dll";
#else
        const QString engineName = "7z", coreName = "7z.so";
#endif
        const QDir prepared(QString::fromUtf8(SEVENZIP_FAT_RUNTIME));
        const QString engine = runtime.filePath(engineName), core = runtime.filePath(coreName);
        QVERIFY(QFile::copy(prepared.filePath(engineName), engine));
        QVERIFY(QFile::copy(prepared.filePath(coreName), core));
        QVERIFY(QDir(runtime.path()).mkdir("Formats"));
        const QString module = QString::fromUtf8(SEVENZIP_FAT_MODULE);
        QVERIFY(QFile::copy(module, runtime.filePath("Formats/" + QFileInfo(module).fileName())));
        const QString image = runtime.filePath(QStringLiteral("образ 空.img"));
        QFile source(image);
        QVERIFY(source.open(QIODevice::WriteOnly));
        QCOMPARE(source.write(bytes_), bytes_.size());
        source.close();
        ArchiveBackend backend(nullptr, engine);
        QSignalSpy ready(&backend, &ArchiveBackend::codecCatalogReady);
        QVERIFY(backend.discoverCodecCatalog(core));
        QTRY_COMPARE_WITH_TIMEOUT(ready.size(), 1, 15000);
        QVERIFY2(ready[0][0].toBool(), qPrintable(ready[0][1].toString()));
        QVERIFY(backend.codecCatalog()->formatIndex("FAT") >= 0);
        QVERIFY(!backend.codecCatalog()->canUpdate("FAT"));
        QList<ArchiveEntry> entries;
        connect(&backend, &ArchiveBackend::entriesReady, this,
                [&](const QList<ArchiveEntry> &result) { entries = result; });
        QSignalSpy finished(&backend, &ArchiveBackend::finished);
        backend.list(image);
        QTRY_COMPARE_WITH_TIMEOUT(finished.size(), 1, 10000);
        QVERIFY2(finished[0][0].toBool(), qPrintable(finished[0][1].toString()));
        QCOMPARE(entries.size(), 1);
        QCOMPARE(entries[0].path, QString("LEGACY.BIN"));
        finished.clear();
        backend.test(image);
        QTRY_COMPARE_WITH_TIMEOUT(finished.size(), 1, 10000);
        QVERIFY2(finished[0][0].toBool(), qPrintable(finished[0][1].toString()));
        finished.clear();
        const QString output = runtime.filePath("output");
        backend.extract(image, output, {"LEGACY.BIN"});
        QTRY_COMPARE_WITH_TIMEOUT(finished.size(), 1, 10000);
        QVERIFY2(finished[0][0].toBool(), qPrintable(finished[0][1].toString()));
        QFile result(QDir(output).filePath("LEGACY.BIN"));
        QVERIFY(result.open(QIODevice::ReadOnly));
        QCOMPARE(result.readAll(), payload());
        QVERIFY(source.open(QIODevice::ReadOnly));
        QCOMPARE(source.readAll(), bytes_);
    }
    void cleanupTestCase() { archive_.Release(); }

  private:
    static QByteArray payload() {
        QByteArray result;
        for (int repeat = 0; repeat < 4; ++repeat)
            for (int value = 0; value < 256; ++value)
                result.append(char(value));
        return result + "fragmented FAT tail\n";
    }
};
QTEST_GUILESS_MAIN(FatModuleTests)
#include "fat_module_tests.moc"
