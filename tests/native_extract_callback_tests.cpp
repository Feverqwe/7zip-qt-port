#include "CPP/7zip/UI/Common/ArchiveExtractCallback.h"
#include "CPP/Windows/FileDir.h"
#include "Common/StringConvert.h"
#include "qtpanelfolder.h"
#include <QDir>
#include <QFile>
#include <QTemporaryDir>
#include <QtTest>

namespace {
class CExtractUi final : public IFolderArchiveExtractCallback, public CMyUnknownImp {
    Z7_COM_UNKNOWN_IMP_1(IFolderArchiveExtractCallback)

  public:
    bool Stopped = false;
    Int32 LastResult = -1;
    QStringList Errors;
    Z7_COM7F_IMF(SetTotal(UInt64)) override { return S_OK; }
    Z7_COM7F_IMF(SetCompleted(const UInt64 *)) override { return Stopped ? E_ABORT : S_OK; }
    Z7_COM7F_IMF(AskOverwrite(const wchar_t *, const FILETIME *, const UInt64 *, const wchar_t *,
                              const FILETIME *, const UInt64 *, Int32 *answer))
    override {
        *answer = NOverwriteAnswer::kNo;
        return S_OK;
    }
    Z7_COM7F_IMF(PrepareOperation(const wchar_t *, Int32, Int32, const UInt64 *)) override {
        return Stopped ? E_ABORT : S_OK;
    }
    Z7_COM7F_IMF(MessageError(const wchar_t *message)) override {
        Errors.append(QString::fromWCharArray(message));
        return S_OK;
    }
    Z7_COM7F_IMF(SetOperationResult(Int32 result, Int32)) override {
        LastResult = result;
        return S_OK;
    }
};

struct CExtractFixture {
    CArc Arc;
    CMyComPtr2_Create<IFolderArchiveExtractCallback, CExtractUi> Ui;
    CMyComPtr2_Create<IArchiveExtractCallback, CArchiveExtractCallback> Callback;
    CExtractFixture(const QString &root, const QString &name,
                    NExtract::NOverwriteMode::EEnum mode = NExtract::NOverwriteMode::kAsk) {
        CObjectVector<CPanelItem> items;
        CPanelItem item;
        item.RelPath = name;
        item.SizeDefined = true;
        item.Size = 7;
        items.Add(item);
        CMyComPtr<IFolderFolder> metadata =
            new QtPanelFolder({}, items, QtPanelFolder::PropertyCatalog::ArchiveTransport);
        if (metadata.QueryInterface(IID_IInArchive, &Arc.Archive) != S_OK)
            throw 1;
        Arc.DefaultName = L"[Content]";
        Callback->InitForMulti(false, NExtract::NPathMode::kFullPaths, mode,
                               NExtract::NZoneIdMode::kNone, false);
        Callback->InitBeforeNewArchive();
        Callback->Init(CExtractNtOptions(), nullptr, &Arc, Ui, false, false,
                       us2fs(QDir(root).absolutePath().toStdWString().c_str()), UStringVector(),
                       false, 0);
    }
};
QByteArray readFile(const QString &path) {
    QFile file(path);
    return file.open(QIODevice::ReadOnly) ? file.readAll() : QByteArray();
}
} // namespace

class NativeExtractCallbackTests final : public QObject {
    Q_OBJECT
  private slots:
    void sourceStreamCreatesUnicodePathAndReportsResult() {
        QTemporaryDir fixture;
        QVERIFY(fixture.isValid());
        const QString name = QStringLiteral("inside/имя с пробелом.bin");
        CExtractFixture extraction(fixture.path(), name);
        CMyComPtr<ISequentialOutStream> stream;
        QCOMPARE(extraction.Callback.Interface()->GetStream(0, &stream,
                                                            NArchive::NExtract::NAskMode::kExtract),
                 S_OK);
        QVERIFY(stream);
        QCOMPARE(extraction.Callback.Interface()->PrepareOperation(
                     NArchive::NExtract::NAskMode::kExtract),
                 S_OK);
        UInt32 written = 0;
        QCOMPARE(stream->Write("payload", 7, &written), S_OK);
        QCOMPARE(written, UInt32(7));
        stream.Release();
        QCOMPARE(extraction.Callback.Interface()->SetOperationResult(
                     NArchive::NExtract::NOperationResult::kOK),
                 S_OK);
        QCOMPARE(extraction.Callback->CloseArc(), S_OK);
        QCOMPARE(readFile(QDir(fixture.path()).filePath(name)), QByteArray("payload"));
        QCOMPARE(extraction.Ui->LastResult, Int32(NArchive::NExtract::NOperationResult::kOK));
        QVERIFY(extraction.Ui->Errors.isEmpty());
    }
    void sourcePathCorrectionKeepsTraversalInsideDestination() {
        QTemporaryDir fixture;
        QVERIFY(fixture.isValid());
        const QString root = fixture.filePath("destination");
        QVERIFY(QDir().mkpath(root));
        CExtractFixture extraction(root, "../../escape.bin");
        CMyComPtr<ISequentialOutStream> stream;
        QCOMPARE(extraction.Callback.Interface()->GetStream(0, &stream,
                                                            NArchive::NExtract::NAskMode::kExtract),
                 S_OK);
        QVERIFY(stream);
        UInt32 written = 0;
        QCOMPARE(stream->Write("payload", 7, &written), S_OK);
        stream.Release();
        QCOMPARE(extraction.Callback.Interface()->SetOperationResult(
                     NArchive::NExtract::NOperationResult::kOK),
                 S_OK);
        QCOMPARE(extraction.Callback->CloseArc(), S_OK);
        QVERIFY(!QFile::exists(fixture.filePath("escape.bin")));
        QCOMPARE(readFile(QDir(root).filePath("escape.bin")), QByteArray("payload"));
    }
    void skipAndCancelledProgressPreserveExistingFile() {
        QTemporaryDir fixture;
        QVERIFY(fixture.isValid());
        QFile existing(fixture.filePath("existing.bin"));
        QVERIFY(existing.open(QIODevice::WriteOnly));
        QCOMPARE(existing.write("original"), qint64(8));
        existing.close();
        CExtractFixture extraction(fixture.path(), "existing.bin", NExtract::NOverwriteMode::kSkip);
        CMyComPtr<ISequentialOutStream> stream;
        QCOMPARE(extraction.Callback.Interface()->GetStream(0, &stream,
                                                            NArchive::NExtract::NAskMode::kExtract),
                 S_OK);
        QVERIFY(!stream);
        extraction.Ui->Stopped = true;
        UInt64 completed = 1;
        QCOMPARE(extraction.Callback.Interface()->SetCompleted(&completed), E_ABORT);
        QCOMPARE(extraction.Callback->CloseArc(), S_OK);
        QCOMPARE(readFile(existing.fileName()), QByteArray("original"));
    }
};
QTEST_GUILESS_MAIN(NativeExtractCallbackTests)
#include "native_extract_callback_tests.moc"
