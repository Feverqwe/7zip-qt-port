// clang-format off
#include "Common/MyInitGuid.h"
// clang-format on
#include "7zip/Archive/IArchive.h"
#include "Common/MyCom.h"
#include "Windows/PropVariant.h"
#include "exfat_fixture.h"
#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QLibrary>
#include <QProcess>
#include <QTemporaryDir>
#include <QtEndian>
#include <QtTest>
#include <map>

namespace {
class Input final : public IInStream, public CMyUnknownImp {
    Z7_COM_UNKNOWN_IMP_2(ISequentialInStream, IInStream)
    QByteArray bytes_;
    UInt64 position_ = 0;

  public:
    bool *cancelOnRead = nullptr;
    explicit Input(QByteArray bytes) : bytes_(std::move(bytes)) {}
    Z7_COM7F_IMF(Read(void *data, UInt32 size, UInt32 *processed)) override {
        const UInt64 remaining = position_ < UInt64(bytes_.size()) ? bytes_.size() - position_ : 0;
        size = UInt32(qMin<UInt64>(size, remaining));
        if (size)
            memcpy(data, bytes_.constData() + position_, size);
        position_ += size;
        if (processed)
            *processed = size;
        if (cancelOnRead)
            *cancelOnRead = true;
        return S_OK;
    }
    Z7_COM7F_IMF(Seek(Int64 offset, UInt32 origin, UInt64 *position)) override {
        const Int64 base = origin == STREAM_SEEK_SET   ? 0
                           : origin == STREAM_SEEK_CUR ? Int64(position_)
                                                       : bytes_.size();
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
class OpenProgress final : public IArchiveOpenCallback, public CMyUnknownImp {
    Z7_COM_UNKNOWN_IMP_1(IArchiveOpenCallback)
  public:
    Z7_COM7F_IMF(SetTotal(const UInt64 *, const UInt64 *)) override { return S_OK; }
    Z7_COM7F_IMF(SetCompleted(const UInt64 *, const UInt64 *)) override { return E_ABORT; }
};
QString path(IInArchive *archive, UInt32 index) {
    NWindows::NCOM::CPropVariant prop;
    if (archive->GetProperty(index, kpidPath, &prop) != S_OK || prop.vt != VT_BSTR)
        return {};
    return QString::fromWCharArray(prop.bstrVal);
}
QByteArray read(const QString &path) {
    QFile file(path);
    return file.open(QIODevice::ReadOnly) ? file.readAll() : QByteArray();
}
} // namespace

class ExfatTests final : public QObject {
    Q_OBJECT
    QLibrary module_{QString::fromUtf8(SEVENZIP_EXFAT_MODULE)};
    CMyComPtr<IInArchive> archive_;
    ExfatFixture fixture_;
    HRESULT open(const QByteArray &bytes, IArchiveOpenCallback *callback = nullptr) {
        CMyComPtr<IInStream> input = new Input(bytes);
        return archive_->Open(input, nullptr, callback);
    }
    UInt32 indexOf(const QString &name) {
        UInt32 count = 0;
        archive_->GetNumberOfItems(&count);
        for (UInt32 i = 0; i < count; ++i)
            if (path(archive_, i) == name)
                return i;
        return UInt32(-1);
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
        QCOMPARE(clsid.vt, VARTYPE(VT_BSTR));
        GUID guid;
        memcpy(&guid, clsid.bstrVal, sizeof(guid));
        QCOMPARE(create(&guid, &IID_IInArchive, reinterpret_cast<void **>(&archive_)), S_OK);
        void *writer = nullptr;
        QCOMPARE(create(&guid, &IID_IOutArchive, &writer), CLASS_E_CLASSNOTAVAILABLE);
        QVERIFY(!writer);
    }
    void treeAndFileContents_data() {
        QTest::addColumn<int>("layout");
        QTest::newRow("exfat") << 0;
        QTest::newRow("texfat-first") << 1;
        QTest::newRow("texfat-second") << 2;
        QTest::newRow("texfat-fragmented-bitmap-first") << 3;
        QTest::newRow("texfat-fragmented-bitmap-second") << 4;
        QTest::newRow("texfat-4096-byte-sector") << 5;
    }
    void treeAndFileContents() {
        QFETCH(int, layout);
        ExfatFixture fixture;
        if (layout)
            fixture = TexfatFixture(layout == 5 ? 1 : (layout - 1) % 2, layout == 5 ? 4096 : 512,
                                    layout == 3 || layout == 4);
        QCOMPARE(open(fixture.image), S_OK);
        UInt32 count = 0;
        QCOMPARE(archive_->GetNumberOfItems(&count), S_OK);
        QCOMPARE(count, 5u);
        const std::map<QString, QByteArray> expected{
            {fixture.fragmentedName, QByteArray(fixture.sectorSize, 'A') + QByteArray(188, 'B')},
            {fixture.longName, "end"},
            {"empty.txt", {}},
            {fixture.directoryName + '/' + fixture.nestedName,
             QByteArray("tail") + QByteArray(2 * fixture.sectorSize - 4, '\0')}};
        for (const auto &[name, bytes] : expected) {
            const UInt32 index = indexOf(name);
            QVERIFY2(index != UInt32(-1), qPrintable(name));
            CMyComPtr2<IArchiveExtractCallback, Extract> callback;
            callback.Create_if_Empty();
            QCOMPARE(archive_->Extract(&index, 1, 0, callback), S_OK);
            QCOMPARE(callback->result, Int32(NArchive::NExtract::NOperationResult::kOK));
            QCOMPARE(callback->output->bytes, bytes);
        }
        NWindows::NCOM::CPropVariant timestamp;
        QCOMPARE(archive_->GetProperty(indexOf(fixture.fragmentedName), kpidMTime, &timestamp),
                 S_OK);
        QCOMPARE(timestamp.vt, VARTYPE(VT_FILETIME));
        const quint64 ticks =
            (quint64(timestamp.filetime.dwHighDateTime) << 32) | timestamp.filetime.dwLowDateTime;
        const auto utc = QDateTime(QDate(2026, 10, 17), QTime(9, 34, 57, 230), QTimeZone::UTC);
        QCOMPARE(qint64(ticks / 10000) - 11644473600000LL, utc.toMSecsSinceEpoch());
    }
    void entryTimestamps_data() {
        QTest::addColumn<int>("date");
        QTest::addColumn<int>("time");
        QTest::addColumn<int>("cs");
        QTest::addColumn<int>("tz");
        QTest::addColumn<QDateTime>("expected");
        const int date = ((2026 - 1980) << 9) | (10 << 5) | 17;
        const int time = (12 << 11) | (34 << 5) | 28;
        const QDateTime base(QDate(2026, 10, 17), QTime(12, 34, 56), QTimeZone::UTC);
        QTest::newRow("unknown-offset") << date << time << 123 << 0 << base.addMSecs(1230);
        QTest::newRow("unknown-offset-bits") << date << time << 0 << 0x7f << base;
        QTest::newRow("positive-offset")
            << date << time << 199 << 0xbf << base.addSecs(-63 * 900).addMSecs(1990);
        QTest::newRow("negative-offset")
            << date << time << 1 << 0xc0 << base.addSecs(64 * 900).addMSecs(10);
        QTest::newRow("large-increment") << date << time << 255 << 0x80 << base.addMSecs(2550);
        QTest::newRow("zero-date")
            << 0 << 0 << 0 << 0x80 << QDateTime(QDate(1979, 11, 30), QTime(0, 0), QTimeZone::UTC);
        QTest::newRow("leap-day-overflow")
            << (((2024 - 1980) << 9) | (2 << 5) | 30) << 0 << 0 << 0x80
            << QDateTime(QDate(2024, 3, 1), QTime(0, 0), QTimeZone::UTC);
        QTest::newRow("clock-overflow")
            << date << ((31 << 11) | (63 << 5) | 31) << 0 << 0x80
            << QDateTime(QDate(2026, 10, 18), QTime(8, 4, 2), QTimeZone::UTC);
    }
    void entryTimestamps() {
        QFETCH(int, date);
        QFETCH(int, time);
        QFETCH(int, cs);
        QFETCH(int, tz);
        QFETCH(QDateTime, expected);
        ExfatFixture fixture;
        const int offset = fixture.rootOffset(2);
        for (int field : {8, 12, 16}) {
            qToLittleEndian(quint16(time), fixture.image.data() + offset + field);
            qToLittleEndian(quint16(date), fixture.image.data() + offset + field + 2);
        }
        fixture.image[offset + 20] = fixture.image[offset + 21] = char(cs);
        fixture.image[offset + 22] = fixture.image[offset + 23] = fixture.image[offset + 24] =
            char(tz);
        fixture.refreshEntryChecksum(2);
        QCOMPARE(open(fixture.image), S_OK);
        const UInt32 index = indexOf(fixture.fragmentedName);
        QVERIFY(index != UInt32(-1));
        for (PROPID property : {kpidCTime, kpidMTime, kpidATime}) {
            NWindows::NCOM::CPropVariant timestamp;
            QCOMPARE(archive_->GetProperty(index, property, &timestamp), S_OK);
            QCOMPARE(timestamp.vt, VARTYPE(VT_FILETIME));
            const quint64 ticks = (quint64(timestamp.filetime.dwHighDateTime) << 32) |
                                  timestamp.filetime.dwLowDateTime;
            const auto wanted = property == kpidATime ? expected.addMSecs(-cs * 10) : expected;
            QCOMPARE(ticks, quint64(wanted.toMSecsSinceEpoch() + 11644473600000LL) * 10000);
        }
    }
    void timestampsAcrossHostTimezones() {
        for (const auto *zone : {"UTC0", "HST10"}) {
            QProcess process;
            auto environment = QProcessEnvironment::systemEnvironment();
            environment.insert("TZ", QString::fromLatin1(zone));
            process.setProcessEnvironment(environment);
            process.start(QCoreApplication::applicationFilePath(), {"entryTimestamps", "-silent"});
            QVERIFY(process.waitForFinished(15000));
            QCOMPARE(process.exitStatus(), QProcess::NormalExit);
            QVERIFY2(process.exitCode() == 0, process.readAllStandardOutput().constData());
        }
    }
    void largerLogicalSectors() {
        ExfatFixture fixture(4096);
        QCOMPARE(open(fixture.image), S_OK);
        const UInt32 index = indexOf(fixture.fragmentedName);
        QVERIFY(index != UInt32(-1));
        CMyComPtr2<IArchiveExtractCallback, Extract> callback;
        callback.Create_if_Empty();
        QCOMPARE(archive_->Extract(&index, 1, 0, callback), S_OK);
        QCOMPARE(callback->result, Int32(NArchive::NExtract::NOperationResult::kOK));
        QCOMPARE(callback->output->bytes, QByteArray(4096, 'A') + QByteArray(188, 'B'));
    }
    void malformedVolume_data() {
        QTest::addColumn<QString>("fault");
        for (const auto *fault :
             {"boot-checksum", "cluster-range", "root-cycle", "entry-checksum", "extent-range",
              "name-length", "traversal-name", "truncated", "directory-cycle"})
            QTest::newRow(fault) << QString::fromLatin1(fault);
    }
    void malformedVolume() {
        QFETCH(QString, fault);
        ExfatFixture f;
        if (fault == "boot-checksum")
            f.image[120] = 1;
        if (fault == "cluster-range") {
            f.set32(92, 0xffffffff);
            f.refreshBootChecksum();
        }
        if (fault == "root-cycle")
            f.fat(10, 2);
        if (fault == "entry-checksum")
            f.image[f.rootOffset(2) + 4] ^= 1;
        if (fault == "extent-range") {
            f.set32(f.rootOffset(3) + 20, 10000);
            f.refreshEntryChecksum(2);
        }
        if (fault == "name-length") {
            f.image[f.rootOffset(3) + 3] = 1;
            f.refreshEntryChecksum(2);
        }
        if (fault == "traversal-name") {
            f.image[f.rootOffset(4) + 2] = '/';
            f.refreshEntryChecksum(2);
        }
        if (fault == "truncated")
            f.image.chop(200000);
        if (fault == "directory-cycle") {
            const int directory = 2 + (2 + (f.fragmentedName.size() + 14) / 15);
            f.set32(f.rootOffset(directory + 1) + 20, 2);
            f.refreshEntryChecksum(directory);
        }
        QVERIFY(open(f.image) != S_OK);
        UInt32 count = 999;
        QCOMPARE(archive_->GetNumberOfItems(&count), S_OK);
        QCOMPARE(count, 0u);
        QCOMPARE(open(fixture_.image), S_OK); // Failed opens leave a reusable handler.
    }
    void dataFailureAndCancellation() {
        ExfatFixture broken;
        broken.fat(9, 5);
        QCOMPARE(open(broken.image), S_OK);
        UInt32 index = indexOf(broken.fragmentedName);
        CMyComPtr2<IArchiveExtractCallback, Extract> callback;
        callback.Create_if_Empty();
        QCOMPARE(archive_->Extract(&index, 1, 1, callback), S_OK);
        QCOMPARE(callback->result, Int32(NArchive::NExtract::NOperationResult::kDataError));
        CMyComPtr2<IArchiveOpenCallback, OpenProgress> progress;
        progress.Create_if_Empty();
        QCOMPARE(open(fixture_.image, progress), E_ABORT);
        QCOMPARE(open(fixture_.image), S_OK);
        index = indexOf(fixture_.fragmentedName);
        callback->cancel = true;
        QCOMPARE(archive_->Extract(&index, 1, 0, callback), E_ABORT);
        callback->cancel = false;
        callback->output->fail = true;
        QCOMPARE(archive_->Extract(&index, 1, 0, callback), E_FAIL);
    }
    void cancelDuringCyclicFileChain() {
        ExfatFixture broken;
        broken.fat(9, 5);
        auto *input = new Input(broken.image);
        CMyComPtr<IInStream> stream = input;
        QCOMPARE(archive_->Open(stream, nullptr, nullptr), S_OK);
        const UInt32 index = indexOf(broken.fragmentedName);
        QVERIFY(index != UInt32(-1));
        CMyComPtr2<IArchiveExtractCallback, Extract> callback;
        callback.Create_if_Empty();
        // Request cancellation when the source returns data during extraction.
        input->cancelOnRead = &callback->cancel;
        QCOMPARE(archive_->Extract(&index, 1, 0, callback), E_ABORT);
        QVERIFY(callback->output->bytes.isEmpty());
        QCOMPARE(open(fixture_.image), S_OK);
        const UInt32 good = indexOf(fixture_.fragmentedName);
        callback->cancel = false;
        QCOMPARE(archive_->Extract(&good, 1, 0, callback), S_OK);
        QCOMPARE(callback->result, Int32(NArchive::NExtract::NOperationResult::kOK));
        QCOMPARE(callback->output->bytes, QByteArray(512, 'A') + QByteArray(188, 'B'));
    }
    void texfatRefusesInconsistentVolumes_data() {
        QTest::addColumn<unsigned>("active");
        QTest::addColumn<QString>("fault");
        for (unsigned active : {0u, 1u})
            for (const auto *fault :
                 {"dirty", "media-failure", "missing-bitmap", "duplicate-bitmap",
                  "active-root-cycle", "active-root-unallocated", "bitmap-cycle"}) {
                const QByteArray row = QByteArray::number(active) + '-' + fault;
                QTest::newRow(row.constData()) << active << QString::fromLatin1(fault);
            }
    }
    void texfatRefusesInconsistentVolumes() {
        QFETCH(unsigned, active);
        QFETCH(QString, fault);
        TexfatFixture fixture(active);
        if (fault == "dirty")
            fixture.image[106] = char(active | 2);
        if (fault == "media-failure")
            fixture.image[106] = char(active | 4);
        if (fault == "missing-bitmap")
            fixture.image[fixture.rootOffset(31)] = 1;
        if (fault == "duplicate-bitmap")
            fixture.image[fixture.rootOffset(31) + 1] = 0;
        if (fault == "active-root-cycle")
            fixture.fatCopy(active, 11, 2);
        if (fault == "active-root-unallocated")
            fixture.allocated(active, 2, false);
        if (fault == "bitmap-cycle") {
            const unsigned start = active ? 13 : 3;
            fixture.fatCopy(active, start, start);
        }
        QVERIFY(open(fixture.image) != S_OK);
        UInt32 count = 999;
        QCOMPARE(archive_->GetNumberOfItems(&count), S_OK);
        QCOMPARE(count, 0u);
        QCOMPARE(open(fixture_.image), S_OK);
    }
    void texfatDoesNotRecoverFromInactivePair() {
        TexfatFixture fixture(1);
        // The inactive copy still has an acyclic chain, but its payload is stale.
        fixture.fatCopy(1, 9, 5);
        QCOMPARE(open(fixture.image), S_OK);
        const UInt32 index = indexOf(fixture.fragmentedName);
        QVERIFY(index != UInt32(-1));
        CMyComPtr2<IArchiveExtractCallback, Extract> callback;
        callback.Create_if_Empty();
        QCOMPARE(archive_->Extract(&index, 1, 0, callback), S_OK);
        QCOMPARE(callback->result, Int32(NArchive::NExtract::NOperationResult::kDataError));
        QVERIFY(callback->output->bytes.isEmpty());
    }
    void texfatCancellation() {
        TexfatFixture fixture(1);
        CMyComPtr2<IArchiveOpenCallback, OpenProgress> progress;
        progress.Create_if_Empty();
        QCOMPARE(open(fixture.image, progress), E_ABORT);
        QCOMPARE(open(fixture.image), S_OK);
        const UInt32 index = indexOf(fixture.fragmentedName);
        CMyComPtr2<IArchiveExtractCallback, Extract> callback;
        callback.Create_if_Empty();
        callback->cancel = true;
        QCOMPARE(archive_->Extract(&index, 1, 0, callback), E_ABORT);
        callback->cancel = false;
        QCOMPARE(archive_->Extract(&index, 1, 0, callback), S_OK);
        QCOMPARE(callback->result, Int32(NArchive::NExtract::NOperationResult::kOK));
        QCOMPARE(callback->output->bytes, QByteArray(512, 'A') + QByteArray(188, 'B'));
    }
    void officialEngineListsTestsAndExtracts_data() {
        QTest::addColumn<int>("active");
        QTest::newRow("exfat") << -1;
        QTest::newRow("texfat-first") << 0;
        QTest::newRow("texfat-second") << 1;
    }
    void officialEngineListsTestsAndExtracts() {
        QFETCH(int, active);
        ExfatFixture fixture;
        if (active >= 0)
            fixture = TexfatFixture(unsigned(active));
        QTemporaryDir runtime;
        QVERIFY(runtime.isValid());
        const QDir prepared(QString::fromUtf8(SEVENZIP_EXFAT_RUNTIME));
#ifdef Q_OS_WIN
        const QString engineName = "7z.exe", coreName = "7z.dll", suffix = ".dll";
#else
        const QString engineName = "7z", coreName = "7z.so", suffix = ".so";
#endif
        const QString engine = runtime.filePath(engineName);
        QVERIFY(QFile::copy(prepared.filePath(engineName), engine));
        QVERIFY(QFile::copy(prepared.filePath(coreName), runtime.filePath(coreName)));
        QVERIFY(QDir(runtime.path()).mkdir("Formats"));
        QVERIFY(QFile::copy(module_.fileName(), runtime.filePath("Formats/exfat" + suffix)));
        const QString image = runtime.filePath(QStringLiteral("образ 空.img"));
        QFile file(image);
        QVERIFY(file.open(QIODevice::WriteOnly));
        QCOMPARE(file.write(fixture.image), qint64(fixture.image.size()));
        file.close();
        const auto hash = QCryptographicHash::hash(read(image), QCryptographicHash::Sha256);
        auto run = [&](const QStringList &arguments) {
            QProcess process;
            process.start(engine, QStringList{"-sccUTF-8"} + arguments);
            if (!process.waitForFinished(10000))
                return QByteArray("PROCESS FAILED");
            const QByteArray output =
                process.readAllStandardOutput() + process.readAllStandardError();
            if (process.exitCode() != 0)
                return QByteArray("EXIT FAILED: ") + output;
            return output;
        };
        const auto listing = run({"l", "-slt", "--", image});
        QVERIFY2(listing.contains("Type = exFAT"), listing.constData());
        QVERIFY2(listing.contains(fixture.fragmentedName.toUtf8()), listing.constData());
        const auto tested = run({"t", "--", image});
        QVERIFY2(tested.contains("Everything is Ok"), tested.constData());
        const QString output = runtime.filePath("output");
        const auto extracted = run({"x", "-o" + output, "--", image});
        QVERIFY2(extracted.contains("Everything is Ok"), extracted.constData());
        QCOMPARE(read(QDir(output).filePath(fixture.fragmentedName)),
                 QByteArray(512, 'A') + QByteArray(188, 'B'));
        QCOMPARE(read(QDir(output).filePath(fixture.directoryName + '/' + fixture.nestedName)),
                 QByteArray("tail") + QByteArray(1020, '\0'));
        QCOMPARE(QCryptographicHash::hash(read(image), QCryptographicHash::Sha256), hash);
    }
    void cleanupTestCase() { archive_.Release(); }
};
QTEST_GUILESS_MAIN(ExfatTests)
#include "exfat_tests.moc"
