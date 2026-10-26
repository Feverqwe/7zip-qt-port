#include "7zip/Common/StreamObjects.h"
#include "archivebackend.h"
#include "archivecodeccatalog.h"
#include "fixtures/codec_runtime.h"
#include <QFile>
#include <QProcess>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QtTest>

namespace {
QByteArray Read(const QString &path) {
    QFile file(path);
    return file.open(QIODevice::ReadOnly) ? file.readAll() : QByteArray();
}
bool Write(const QString &path, const QByteArray &bytes) {
    QFile file(path);
    return file.open(QIODevice::WriteOnly) && file.write(bytes) == bytes.size();
}
QByteArray Expected(const QString &name) {
    if (name == "empty")
        return {};
    if (name == "small")
        return QString::fromUtf8("Brotli: Привет, мир! 空\n").toUtf8();
    if (name == "large")
        return QByteArray(2 * 1024 * 1024, 'A');
    QByteArray bytes;
    quint32 state = 0x12345678;
    for (int i = 0; i < 98304; ++i) {
        state ^= state << 13;
        state ^= state >> 17;
        state ^= state << 5;
        bytes.append(char(state & 255));
    }
    return bytes;
}
class COutput final : public ISequentialOutStream, public CMyUnknownImp {
    Z7_COM_UNKNOWN_IMP_1(ISequentialOutStream)
  public:
    QByteArray Bytes;
    bool Fail = false;
    Z7_COM7F_IMF(Write(const void *data, UInt32 size, UInt32 *processed)) override {
        if (processed)
            *processed = 0;
        if (Fail)
            return E_FAIL;
        Bytes.append(static_cast<const char *>(data), size);
        if (processed)
            *processed = size;
        return S_OK;
    }
};
class CExtract final : public IArchiveExtractCallback, public CMyUnknownImp {
    Z7_COM_UNKNOWN_IMP_1(IArchiveExtractCallback)
  public:
    CMyComPtr2<ISequentialOutStream, COutput> Output;
    Int32 Result = -1;
    bool Cancel = false;
    CExtract() { Output.Create_if_Empty(); }
    Z7_COM7F_IMF(SetTotal(UInt64)) override { return S_OK; }
    Z7_COM7F_IMF(SetCompleted(const UInt64 *)) override {
        return Cancel && !Output->Bytes.isEmpty() ? E_ABORT : S_OK;
    }
    Z7_COM7F_IMF(GetStream(UInt32, ISequentialOutStream **stream, Int32 mode)) override {
        *stream = nullptr;
        if (mode == NArchive::NExtract::NAskMode::kExtract) {
            *stream = Output.Interface();
            (*stream)->AddRef();
        }
        return S_OK;
    }
    Z7_COM7F_IMF(PrepareOperation(Int32)) override { return S_OK; }
    Z7_COM7F_IMF(SetOperationResult(Int32 result)) override {
        Result = result;
        return S_OK;
    }
};
} // namespace

class BrotliTests final : public QObject {
    Q_OBJECT
    QTemporaryDir runtime_;
    QString engine_, module_;
    std::shared_ptr<const ArchiveCodecCatalog> catalog_;
    QString fixture(const QString &name) const {
        return QDir(QString::fromUtf8(SEVENZIP_BROTLI_FIXTURES)).filePath(name + ".br");
    }
  private slots:
    void initTestCase() {
        QVERIFY(runtime_.isValid());
        const QDir prepared(QString::fromUtf8(SEVENZIP_BROTLI_RUNTIME));
#ifdef Q_OS_WIN
        const QString engineName = "7z.exe", moduleName = "7z.dll", suffix = ".dll";
#else
        const QString engineName = "7z", moduleName = "7z.so", suffix = ".so";
#endif
        engine_ = runtime_.filePath(engineName);
        module_ = runtime_.filePath(moduleName);
        QVERIFY(CopyCodecRuntime(prepared, QDir(runtime_.path())));
        QVERIFY(QDir(runtime_.path()).mkdir("Formats"));
        QVERIFY(QFile::copy(QString::fromUtf8(SEVENZIP_BROTLI_MODULE),
                            runtime_.filePath(QStringLiteral("Formats/Brotli 空") + suffix)));
        ArchiveBackend backend(nullptr, engine_);
        QSignalSpy ready(&backend, &ArchiveBackend::codecCatalogReady);
        QVERIFY(backend.discoverCodecCatalog(module_));
        QTRY_COMPARE_WITH_TIMEOUT(ready.size(), 1, 15000);
        QVERIFY2(ready[0][0].toBool(), qPrintable(ready[0][1].toString()));
        catalog_ = backend.codecCatalog();
    }
    void nativeStreams_data() {
        QTest::addColumn<QString>("name");
        for (const QString &name : {"empty", "small", "large", "chunks"})
            QTest::newRow(qPrintable(name)) << name;
    }
    void nativeStreams() {
        QFETCH(QString, name);
        CArchiveLink archive;
        QCOMPARE(catalog_->openArchive(archive, fixture(name), {}, nullptr), S_OK);
        QVERIFY(!catalog_->canUpdate("brotli"));
        CMyComPtr<IOutArchive> writer;
        QCOMPARE(archive.GetArc()->Archive.QueryInterface(IID_IOutArchive, &writer), E_NOINTERFACE);
        CMyComPtr2<IArchiveExtractCallback, CExtract> callback;
        callback.Create_if_Empty();
        QCOMPARE(archive.GetArc()->Archive->Extract(nullptr, UInt32(-1), 0, callback), S_OK);
        QCOMPARE(callback->Result, Int32(NArchive::NExtract::NOperationResult::kOK));
        QCOMPARE(callback->Output->Bytes, Expected(name));
        // Repeat on the same handler, then test with no output stream.
        callback->Output->Bytes.clear();
        QCOMPARE(archive.GetArc()->Archive->Extract(nullptr, UInt32(-1), 0, callback), S_OK);
        QCOMPARE(callback->Output->Bytes, Expected(name));
        callback->Output->Bytes.clear();
        QCOMPARE(archive.GetArc()->Archive->Extract(nullptr, UInt32(-1), 1, callback), S_OK);
        QCOMPARE(callback->Result, Int32(NArchive::NExtract::NOperationResult::kOK));
        QVERIFY(callback->Output->Bytes.isEmpty());
    }
    void errorsAndCancellation() {
        QTemporaryDir files;
        const QString path = files.filePath("broken.br");
        QVERIFY(Write(path, QByteArray(20, char(0xff))));
        CArchiveLink malformed;
        QVERIFY(catalog_->openArchive(malformed, path, {}, nullptr) != S_OK);
        const auto original = Read(fixture("chunks"));
        QVERIFY(Write(path, original.left(original.size() - 1)));
        CArchiveLink truncated;
        QCOMPARE(catalog_->openArchive(truncated, path, {}, nullptr), S_OK);
        CMyComPtr2<IArchiveExtractCallback, CExtract> callback;
        callback.Create_if_Empty();
        QCOMPARE(truncated.GetArc()->Archive->Extract(nullptr, UInt32(-1), 1, callback), S_OK);
        QCOMPARE(callback->Result, Int32(NArchive::NExtract::NOperationResult::kUnexpectedEnd));
        const QString trailingPath = files.filePath("trailing.br");
        QVERIFY(Write(trailingPath, Read(fixture("small")) + "trailing"));
        CArchiveLink trailing;
        QCOMPARE(catalog_->openArchive(trailing, trailingPath, {}, nullptr), S_OK);
        QCOMPARE(trailing.GetArc()->Archive->Extract(nullptr, UInt32(-1), 1, callback), S_OK);
        QCOMPARE(callback->Result, Int32(NArchive::NExtract::NOperationResult::kDataAfterEnd));
        CArchiveLink large;
        QCOMPARE(catalog_->openArchive(large, fixture("large"), {}, nullptr), S_OK);
        callback->Cancel = true;
        callback->Result = -1;
        QCOMPARE(large.GetArc()->Archive->Extract(nullptr, UInt32(-1), 0, callback), E_ABORT);
        QVERIFY(callback->Output->Bytes.size() < Expected("large").size());
        QCOMPARE(callback->Result, -1);
        callback->Cancel = false;
        callback->Output->Fail = true;
        QCOMPARE(large.GetArc()->Archive->Extract(nullptr, UInt32(-1), 0, callback), E_FAIL);
    }
    void backendAndConsole() {
        QTemporaryDir files;
        const QString path = files.filePath(QStringLiteral("стили 空.css.br"));
        const QByteArray original = Read(fixture("small"));
        QVERIFY(Write(path, original));
        ArchiveBackend backend(nullptr, engine_);
        QSignalSpy ready(&backend, &ArchiveBackend::codecCatalogReady);
        QVERIFY(backend.discoverCodecCatalog(module_));
        QTRY_COMPARE_WITH_TIMEOUT(ready.size(), 1, 15000);
        QVERIFY2(ready[0][0].toBool(), qPrintable(ready[0][1].toString()));
        QSignalSpy finished(&backend, &ArchiveBackend::finished);
        QList<ArchiveEntry> entries;
        connect(&backend, &ArchiveBackend::entriesReady, this,
                [&](const QList<ArchiveEntry> &value) { entries = value; });
        backend.list(path);
        QTRY_COMPARE(finished.size(), 1);
        QVERIFY2(finished[0][0].toBool(), qPrintable(finished[0][1].toString()));
        QCOMPARE(entries.size(), 1);
        QCOMPARE(entries[0].path, QStringLiteral("стили 空.css"));
        finished.clear();
        backend.test(path);
        QTRY_COMPARE(finished.size(), 1);
        QVERIFY2(finished[0][0].toBool(), qPrintable(finished[0][1].toString()));
        finished.clear();
        const QString output = files.filePath("output");
        backend.extract(path, output);
        QTRY_COMPARE(finished.size(), 1);
        QVERIFY2(finished[0][0].toBool(), qPrintable(finished[0][1].toString()));
        QCOMPARE(Read(QDir(output).filePath(QStringLiteral("стили 空.css"))), Expected("small"));
        QCOMPARE(Read(path), original);
        const QString destination = QDir(output).filePath(QStringLiteral("стили 空.css"));
        QVERIFY(Write(destination, "keep existing content"));
        finished.clear();
        backend.extract(path, output);
        QTRY_COMPARE(finished.size(), 1);
        QVERIFY(!finished[0][0].toBool());
        QCOMPARE(Read(destination), QByteArray("keep existing content"));
        finished.clear();
        ExtractionOptions skip;
        skip.overwriteMode = ExtractionOptions::OverwriteMode::SkipExisting;
        backend.extract(path, output, {}, {}, skip);
        QTRY_COMPARE(finished.size(), 1);
        QVERIFY2(finished[0][0].toBool(), qPrintable(finished[0][1].toString()));
        QCOMPARE(Read(destination), QByteArray("keep existing content"));
        QProcess process;
        process.start(engine_, {"x", "-so", "-bd", path});
        QVERIFY(process.waitForFinished(10000));
        QCOMPARE(process.exitCode(), 0);
        QCOMPARE(process.readAllStandardOutput(), Expected("small"));
        process.start(engine_, {"a", "-tbrotli", files.filePath("write.br"), path});
        QVERIFY(process.waitForFinished(10000));
        QVERIFY(process.exitCode() != 0);
        QVERIFY(!QFileInfo::exists(files.filePath("write.br")));
        const QString text = files.filePath("ordinary.txt");
        QVERIFY(Write(text, "This is ordinary text, not an archive."));
        CArchiveLink unknown;
        QVERIFY(catalog_->openArchive(unknown, text, {}, nullptr) != S_OK);
    }
};
QTEST_GUILESS_MAIN(BrotliTests)
#include "brotli_tests.moc"
