#include "7zip/Common/StreamObjects.h"
#include "CPP/7zip/UI/FileManager/ProgressDialog2.h"
#include "archivebackend.h"
#include "qtarchivefileresource.h"
#include "qtpanelfolder.h"
#include "source_extract_fixture.h"
#include <QTemporaryDir>
#include <QtTest>
using namespace SourceExtractFixture;
namespace {
class CContractCallback final : public IArchiveExtractCallback, public CMyUnknownImp {
    Z7_COM_UNKNOWN_IMP_1(IArchiveExtractCallback)
    CMyComPtr2<ISequentialOutStream, CDynBufSeqOutStream> _stream;

  public:
    UInt64 Total = 0;
    QVector<UInt64> Completed;
    QVector<UInt32> Indices;
    QList<QByteArray> Bytes;
    QVector<Int32> Results;
    Z7_COM7F_IMF(SetTotal(UInt64 total)) override {
        Total = total;
        return S_OK;
    }
    Z7_COM7F_IMF(SetCompleted(const UInt64 *completed)) override {
        if (completed)
            Completed.append(*completed);
        return S_OK;
    }
    Z7_COM7F_IMF(GetStream(UInt32 index, ISequentialOutStream **stream, Int32)) override {
        Indices.append(index);
        _stream.SetFromCls(new CDynBufSeqOutStream);
        CMyComPtr<ISequentialOutStream> owner = _stream.Interface();
        *stream = owner.Detach();
        return S_OK;
    }
    Z7_COM7F_IMF(PrepareOperation(Int32)) override { return S_OK; }
    Z7_COM7F_IMF(SetOperationResult(Int32 result)) override {
        Results.append(result);
        if (result != NArchive::NExtract::NOperationResult::kOK)
            return E_FAIL;
        Bytes.append(
            QByteArray(reinterpret_cast<const char *>(_stream->GetBuffer()), _stream->GetSize()));
        return S_OK;
    }
};

class CExtractContractWorker final : public CProgressThreadVirt {
    CMyComPtr<IUnknown> _resource;
    QtArchiveFileResource *_resourceSpec;
    CMyComPtr<IFolderFolder> _metadata;
    std::unique_ptr<IQtProgressTransport> _transport;
    HRESULT ProcessVirt() override {
        CMyComPtr<IInArchive> archive;
        RINOK(_metadata.QueryInterface(IID_IInArchive, &archive))
        const UInt32 ignored = UInt32(-1);
        return _resourceSpec->Extract(archive, &ignored, UInt32(-1), false, Callback.Interface());
    }

  public:
    CMyComPtr2_Create<IArchiveExtractCallback, CContractCallback> Callback;
    CExtractContractWorker(ArchiveBackend &backend, const QString &path, IFolderFolder *metadata)
        : _resourceSpec(new QtArchiveFileResource(path)), _metadata(metadata) {
        _resource = _resourceSpec;
        _resourceSpec->AttachExtractService(backend, "zip");
        _transport = _resourceSpec->CreateProgressTransport(*this);
    }
};
} // namespace
class SourceExtractContractTests final : public QObject {
    Q_OBJECT
  private slots:
    void cancelledSourceItemDoesNotReportDataError() {
        QTemporaryDir directory;
        const auto archive = makeArchive(directory.path(), QByteArray(4 << 20, 'x'));
        QVERIFY(!archive.isEmpty());
        const QByteArray original = readFile(archive);
        CPanelItem item;
        item.RelPath = kName;
        item.SizeDefined = true;
        item.Size = 4 << 20;
        CObjectVector<CPanelItem> items;
        items.Add(item);
        CMyComPtr<IFolderFolder> metadata =
            new QtPanelFolder({}, items, QtPanelFolder::PropertyCatalog::ArchiveTransport);
        ArchiveBackend backend;
        connect(&backend, &ArchiveBackend::engineStarted, &backend,
                [&backend] { backend.cancel(); });
        CExtractContractWorker worker(backend, archive, metadata);
        QCOMPARE(worker.Create(L"Cancelled source item"), S_OK);
        QCOMPARE(worker.Result, E_ABORT);
        QVERIFY(worker.Callback->Results.isEmpty());
        QVERIFY(worker.Callback->Bytes.isEmpty());
        QVERIFY(!backend.busy());
        QCOMPARE(readFile(archive), original);
    }

    void allItemsIgnoreIndicesAndAccumulateProgress() {
        QTemporaryDir directory;
        const QByteArray first("first\0binary", 12), second(256 << 10, 'z');
        const auto archive = makeArchive(directory.path(), first);
        QVERIFY(!archive.isEmpty());
        const QString other = QStringLiteral("ещё один.bin");
        QVERIFY(writeFile(directory.filePath(other), second));
        QProcess process;
        process.setWorkingDirectory(directory.path());
        process.start(QStandardPaths::findExecutable("7zz"), {"a", "input.zip", other});
        QVERIFY(process.waitForFinished(10000));
        QCOMPARE(process.exitCode(), 0);
        ArchiveBackend backend;
        QList<ArchiveEntry> listed;
        connect(&backend, &ArchiveBackend::entriesReady, &backend,
                [&listed](const QList<ArchiveEntry> &entries) { listed = entries; });
        backend.list(archive);
        QTRY_VERIFY(!backend.busy());
        CObjectVector<CPanelItem> items;
        for (const auto &pair : {qMakePair(kName, first), qMakePair(other, second)}) {
            CPanelItem item;
            QString actualName;
            for (const auto &entry : listed) {
                QString compared = entry.path;
                compared.replace('\\', '/');
                if (!entry.directory && compared.normalized(QString::NormalizationForm_C) ==
                                            pair.first.normalized(QString::NormalizationForm_C)) {
                    QVERIFY(actualName.isEmpty());
                    actualName = entry.path;
                }
            }
            QVERIFY(!actualName.isEmpty());
            item.RelPath = actualName;
            item.SizeDefined = true;
            item.Size = pair.second.size();
            items.Add(item);
        }
        CMyComPtr<IFolderFolder> metadata =
            new QtPanelFolder({}, items, QtPanelFolder::PropertyCatalog::ArchiveTransport);
        CExtractContractWorker worker(backend, archive, metadata);
        QCOMPARE(worker.Create(L"All entries"), S_OK);
        QCOMPARE(worker.Result, S_OK);
        QCOMPARE(worker.Callback->Indices, QVector<UInt32>({0, 1}));
        QCOMPARE(worker.Callback->Bytes, QList<QByteArray>({first, second}));
        QCOMPARE(worker.Callback->Total, UInt64(first.size() + second.size()));
        QVERIFY(!worker.Callback->Completed.isEmpty());
        UInt64 previous = 0;
        for (UInt64 value : worker.Callback->Completed) {
            QVERIFY(value >= previous);
            QVERIFY(value <= worker.Callback->Total);
            previous = value;
        }
        QCOMPARE(previous, worker.Callback->Total);
    }
};
QTEST_MAIN(SourceExtractContractTests)
#include "source_extract_contract_tests.moc"
