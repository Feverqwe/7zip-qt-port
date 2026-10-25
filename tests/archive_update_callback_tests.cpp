#include "CPP/7zip/UI/Agent/Agent.h"
#include "CPP/7zip/UI/Agent/UpdateCallbackAgent.h"
#include "CPP/7zip/UI/Common/EnumDirItems.h"
#include "CPP/7zip/UI/FileManager/UpdateCallback100.h"
#include "Common/StringConvert.h"
#include "qtarchiveproperties.h"
#include "qtpanelfolder.h"
#include <QDir>
#include <QFile>
#include <QTemporaryDir>
#include <QtTest>

namespace {
FString nativePath(const QString &path) {
    const auto value = QDir::toNativeSeparators(path).toStdWString();
    return us2fs(value.c_str());
}
class CRecordingArchive final : public IInArchive, public IOutArchive, public CMyUnknownImp {
    Z7_COM_UNKNOWN_IMP_2(IInArchive, IOutArchive)
    CMyComPtr<IInArchive> _metadata;

  public:
    explicit CRecordingArchive(IInArchive *metadata) : _metadata(metadata) {}
    CUIntVector Changed;
    UString Name;
    QByteArray Data;
    unsigned Calls = 0;
    HRESULT Result = S_OK;
    Z7_COM7F_IMF(Open(IInStream *, const UInt64 *, IArchiveOpenCallback *)) override {
        return E_NOTIMPL;
    }
    Z7_COM7F_IMF(Close()) override { return S_OK; }
    Z7_COM7F_IMF(GetNumberOfItems(UInt32 *count)) override {
        return _metadata->GetNumberOfItems(count);
    }
    Z7_COM7F_IMF(GetProperty(UInt32 index, PROPID id, PROPVARIANT *value)) override {
        return _metadata->GetProperty(index, id, value);
    }
    Z7_COM7F_IMF(Extract(const UInt32 *, UInt32, Int32, IArchiveExtractCallback *)) override {
        return E_NOTIMPL;
    }
    Z7_COM7F_IMF(GetArchiveProperty(PROPID id, PROPVARIANT *value)) override {
        return _metadata->GetArchiveProperty(id, value);
    }
    Z7_COM7F_IMF(GetNumberOfProperties(UInt32 *count)) override {
        return _metadata->GetNumberOfProperties(count);
    }
    Z7_COM7F_IMF(GetPropertyInfo(UInt32 index, BSTR *name, PROPID *id, VARTYPE *type)) override {
        return _metadata->GetPropertyInfo(index, name, id, type);
    }
    Z7_COM7F_IMF(GetNumberOfArchiveProperties(UInt32 *count)) override {
        return _metadata->GetNumberOfArchiveProperties(count);
    }
    Z7_COM7F_IMF(GetArchivePropertyInfo(UInt32 index, BSTR *name, PROPID *id, VARTYPE *type))
    override { return _metadata->GetArchivePropertyInfo(index, name, id, type); }
    Z7_COM7F_IMF(GetFileTimeType(UInt32 *type)) override {
        *type = NFileTimeType::kWindows;
        return S_OK;
    }
    Z7_COM7F_IMF(UpdateItems(ISequentialOutStream *, UInt32 count,
                             IArchiveUpdateCallback *callback))
    override {
        Calls++;
        Changed.Clear();
        Data.clear();
        for (UInt32 i = 0; i < count; i++) {
            Int32 newData = 0, newProps = 0;
            UInt32 arcIndex = UInt32(-1);
            RINOK(callback->GetUpdateItemInfo(i, &newData, &newProps, &arcIndex))
            if (arcIndex != i || newProps != newData)
                return E_FAIL;
            if (!newData)
                continue;
            Changed.Add(arcIndex);
            NWindows::NCOM::CPropVariant name;
            RINOK(callback->GetProperty(i, kpidPath, &name))
            if (name.vt != VT_BSTR)
                return E_FAIL;
            Name = name.bstrVal;
            CMyComPtr<ISequentialInStream> stream;
            RINOK(callback->GetStream(i, &stream))
            if (!stream)
                return E_FAIL;
            char buffer[128];
            for (;;) {
                UInt32 read = 0;
                RINOK(stream->Read(buffer, sizeof(buffer), &read))
                if (!read)
                    break;
                Data.append(buffer, int(read));
            }
        }
        return Result;
    }
};
struct CCallbackFixture {
    CProgressDialog Progress;
    CMyComPtr2_Create<IFolderArchiveUpdateCallback, CUpdateCallback100Imp> Ui;
    CUpdateCallbackAgent Agent;
    CDirItems Disk;
    CRecordVector<CUpdatePair2> Pairs;
    CArc Arc;
    CMyComPtr2_Create<IArchiveUpdateCallback, CArchiveUpdateCallback> Update;
    CCallbackFixture() {
        Ui->ProgressDialog = &Progress;
        Ui->Init();
        Agent.SetCallback(Ui);
        CPanelItem item;
        item.RelPath = QStringLiteral("inside/исходное имя.bin");
        item.Size = 3;
        item.SizeDefined = true;
        CObjectVector<CPanelItem> items;
        items.Add(item);
        CMyComPtr<IFolderFolder> metadata =
            new QtPanelFolder({}, items, QtPanelFolder::PropertyCatalog::ArchiveTransport);
        if (CreateArchivePropertyTransport(metadata, &Arc.Archive) != S_OK)
            throw 1;
        CUpdatePair2 pair;
        pair.SetAs_NoChangeArcItem(0);
        pair.DirIndex = 0;
        pair.NewData = true;
        pair.NewProps = true;
        pair.UseArcProps = false;
        Pairs.Add(pair);
        Update->Arc = &Arc;
        Update->Archive = Arc.Archive;
        Update->DirItems = &Disk;
        Update->UpdatePairs = &Pairs;
        Update->Callback = &Agent;
        Update->KeepOriginalItemNames = true;
    }
    HRESULT enumerate(const QString &path) {
        FStringVector paths;
        paths.Add(nativePath(path));
        return Disk.EnumerateItems2(FString(), UString(), paths, nullptr);
    }
};
} // namespace

class ArchiveUpdateCallbackTests final : public QObject {
    Q_OBJECT
  private slots:
    void sourceUpdateOneFileSelectsOneRealIndex() {
        QTemporaryDir fixture;
        QVERIFY(fixture.isValid());
        const QString path = fixture.filePath("different disk name.bin");
        QFile file(path);
        QVERIFY(file.open(QIODevice::WriteOnly));
        QCOMPARE(file.write("replacement"), qint64(11));
        file.close();
        CObjectVector<CPanelItem> items;
        for (const QString &name :
             {QStringLiteral("inside/first.bin"), QStringLiteral("inside/исходное имя.bin"),
              QStringLiteral("untouched.bin")}) {
            CPanelItem item;
            item.RelPath = name;
            item.SizeDefined = true;
            item.Size = 1;
            items.Add(item);
        }
        CMyComPtr<IFolderFolder> metadata =
            new QtPanelFolder({}, items, QtPanelFolder::PropertyCatalog::ArchiveTransport);
        CMyComPtr2_Create<IFolderArcProps, CAgent> agent;
        QCOMPARE(agent->Init(metadata, UString("zip")), S_OK);
        auto *engine = new CRecordingArchive(agent->GetArchive());
        CMyComPtr<IInArchive> engineOwner = engine;
        agent->_archiveLink.Arcs[0].Archive = engineOwner;
        CMyComPtr<IFolderFolder> root, folder;
        QCOMPARE(agent->BindToRootFolder(&root), S_OK);
        QCOMPARE(root->BindToFolder(L"inside", &folder), S_OK);
        QCOMPARE(folder->LoadItems(), S_OK);
        QCOMPARE(agent->SetFolder(folder), S_OK);
        CProgressDialog progress;
        CMyComPtr2_Create<IFolderArchiveUpdateCallback, CUpdateCallback100Imp> ui;
        ui->ProgressDialog = &progress;
        ui->Init();
        const UInt32 selected = 1;
        const auto diskPath = path.toStdWString();
        QCOMPARE(agent->UpdateOneFile(nullptr, &selected, 1, diskPath.c_str(), ui), S_OK);
        QCOMPARE(engine->Changed.Size(), 1u);
        QCOMPARE(engine->Changed[0], 1u);
        QCOMPARE(QDir::fromNativeSeparators(QString::fromWCharArray(engine->Name.Ptr())),
                 QStringLiteral("inside/исходное имя.bin"));
        QCOMPARE(engine->Data, QByteArray("replacement"));
        const UInt32 two[] = {0, 1};
        QCOMPARE(agent->UpdateOneFile(nullptr, two, 2, diskPath.c_str(), ui), E_FAIL);
        QCOMPARE(engine->Calls, 1u);
        QVERIFY(QFile::remove(path));
        QCOMPARE(agent->UpdateOneFile(nullptr, &selected, 1, diskPath.c_str(), ui), E_FAIL);
        QCOMPARE(engine->Calls, 1u);
        agent->_archiveLink.Arcs[0].ErrorInfo.ThereIsTail = true;
        QCOMPARE(agent->UpdateOneFile(nullptr, &selected, 1, diskPath.c_str(), ui), E_NOTIMPL);
    }

    void originalNameAndNewDataUseSameNativePair() {
        QTemporaryDir fixture;
        QVERIFY(fixture.isValid());
        const QString path = fixture.filePath("новые данные с пробелом.bin");
        QFile file(path);
        QVERIFY(file.open(QIODevice::WriteOnly));
        const QByteArray data("new data");
        QCOMPARE(file.write(data), qint64(data.size()));
        file.close();
        CCallbackFixture callback;
        QCOMPARE(callback.enumerate(path), S_OK);
        QCOMPARE(callback.Disk.Items.Size(), 1u);
        Int32 newData = 0, newProps = 0;
        UInt32 archiveIndex = 99;
        QCOMPARE(
            callback.Update.Interface()->GetUpdateItemInfo(0, &newData, &newProps, &archiveIndex),
            S_OK);
        QCOMPARE(newData, 1);
        QCOMPARE(newProps, 1);
        QCOMPARE(archiveIndex, 0u);
        NWindows::NCOM::CPropVariant name;
        QCOMPARE(callback.Update.Interface()->GetProperty(0, kpidPath, &name), S_OK);
        QCOMPARE(name.vt, VARTYPE(VT_BSTR));
        QCOMPARE(QDir::fromNativeSeparators(QString::fromWCharArray(name.bstrVal)),
                 QStringLiteral("inside/исходное имя.bin"));
        NWindows::NCOM::CPropVariant size;
        QCOMPARE(callback.Update.Interface()->GetProperty(0, kpidSize, &size), S_OK);
        QCOMPARE(size.vt, VARTYPE(VT_UI8));
        QCOMPARE(size.uhVal.QuadPart, UInt64(data.size()));
        CMyComPtr<ISequentialInStream> stream;
        QCOMPARE(callback.Update.Interface()->GetStream(0, &stream), S_OK);
        QVERIFY(stream);
        QVERIFY(!callback.Update->AreAllFilesClosed());
        char buffer[32];
        UInt32 read = 0;
        QCOMPARE(stream->Read(buffer, sizeof(buffer), &read), S_OK);
        QCOMPARE(QByteArray(buffer, int(read)), data);
        stream.Release();
        QVERIFY(callback.Update->AreAllFilesClosed());
        callback.Ui->PasswordIsDefined = true;
        callback.Ui->Password = L"fixture password";
        Int32 defined = 0;
        CMyComBSTR password;
        CMyComPtr<ICryptoGetTextPassword2> crypto;
        QCOMPARE(callback.Update.Interface()->QueryInterface(IID_ICryptoGetTextPassword2,
                                                             (void **)&crypto),
                 S_OK);
        QCOMPARE(crypto->CryptoGetTextPassword2(&defined, &password), S_OK);
        QCOMPARE(defined, 1);
        QCOMPARE(QString::fromWCharArray(password), QStringLiteral("fixture password"));
    }

    void failedFileAndCancellationKeepStreamsClosed() {
        QTemporaryDir fixture;
        QVERIFY(fixture.isValid());
        const QString path = fixture.filePath("temporary.bin");
        QFile file(path);
        QVERIFY(file.open(QIODevice::WriteOnly));
        QCOMPARE(file.write("data"), qint64(4));
        file.close();
        CCallbackFixture callback;
        QCOMPARE(callback.enumerate(path), S_OK);
        QVERIFY(QFile::remove(path));
        CMyComPtr<ISequentialInStream> stream;
        QCOMPARE(callback.Update.Interface()->GetStream(0, &stream), S_FALSE);
        QVERIFY(!stream);
        QVERIFY(callback.Update->AreAllFilesClosed());
        QVERIFY(!callback.Progress.Sync.Messages.IsEmpty());
        callback.Progress.Sync.Set_Stopped(true);
        UInt64 completed = 1;
        QCOMPARE(callback.Update.Interface()->SetCompleted(&completed), E_ABORT);
        QCOMPARE(callback.Update.Interface()->GetStream(0, &stream), E_ABORT);
        QVERIFY(!stream);
        QVERIFY(callback.Update->AreAllFilesClosed());
        CMyComPtr<IFolderArchiveUpdateCallback_MoveArc> move;
        QCOMPARE(callback.Ui.Interface()->QueryInterface(IID_IFolderArchiveUpdateCallback_MoveArc,
                                                         (void **)&move),
                 S_OK);
        QCOMPARE(move->Before_ArcReopen(), S_OK);
        QCOMPARE(callback.Update.Interface()->SetCompleted(&completed), S_OK);
    }
};

QTEST_MAIN(ArchiveUpdateCallbackTests)
#include "archive_update_callback_tests.moc"
