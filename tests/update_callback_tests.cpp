#include "7zip/UI/GUI/ExtractRes.h"
#include "CPP/7zip/UI/FileManager/LangUtils.h"
#include "CPP/7zip/UI/FileManager/PasswordDialog.h"
#include "CPP/7zip/UI/FileManager/UpdateCallback100.h"
#include <QApplication>
#include <QLineEdit>
#include <QThread>
#include <QTimer>
#include <QtTest>

namespace {
QString text(const UString &value) { return QString::fromWCharArray(value.Ptr()); }
class CPasswordWorker : public CProgressThreadVirt {
  public:
    CMyComPtr<ICryptoGetTextPassword> Callback;
    BSTR Password = nullptr;
    ~CPasswordWorker() { SysFreeString(Password); }
    HRESULT ProcessVirt() override { return Callback->CryptoGetTextPassword(&Password); }
};
} // namespace

class UpdateCallbackTests : public QObject {
    Q_OBJECT
  private slots:
    void interfacesAndCounters() {
        CProgressDialog progress;
        auto *callback = new CUpdateCallback100Imp;
        CMyComPtr<IFolderArchiveUpdateCallback> owner = callback;
        callback->ProgressDialog = &progress;
        callback->Init();
        const GUID *interfaces[] = {&IID_IFolderArchiveUpdateCallback,
                                    &IID_IFolderArchiveUpdateCallback2,
                                    &IID_IFolderArchiveUpdateCallback_MoveArc,
                                    &IID_IFolderScanProgress,
                                    &IID_ICryptoGetTextPassword2,
                                    &IID_ICryptoGetTextPassword,
                                    &IID_IArchiveOpenCallback,
                                    &IID_ICompressProgressInfo,
                                    &IID_IUnknown};
        for (const GUID *iid : interfaces) {
            void *result = nullptr;
            QCOMPARE(owner->QueryInterface(*iid, &result), HRESULT(S_OK));
            QVERIFY(result);
            static_cast<IUnknown *>(result)->Release();
        }
        QCOMPARE(static_cast<IFolderScanProgress *>(callback)->ScanProgress(3, 7, 4096,
                                                                            L"файл с пробелом", 1),
                 HRESULT(S_OK));
        QCOMPARE(progress.Sync._totalFiles, UInt64(7));
        QCOMPARE(progress.Sync._totalBytes, UInt64(4096));
        QCOMPARE(owner->SetNumFiles(9), HRESULT(S_OK));
        QCOMPARE(owner->SetTotal(8192), HRESULT(S_OK));
        UInt64 completed = 512, packed = 128;
        QCOMPARE(owner->SetCompleted(&completed), HRESULT(S_OK));
        QCOMPARE(static_cast<ICompressProgressInfo *>(callback)->SetRatioInfo(&completed, &packed),
                 HRESULT(S_OK));
        QCOMPARE(progress.Sync._completedBytes, completed);
        QCOMPARE(progress.Sync._inSize, completed);
        QCOMPARE(progress.Sync._outSize, packed);
        QCOMPARE(owner->OperationResult(-1), HRESULT(S_OK));
        QCOMPARE(progress.Sync._curFiles, UInt64(1));
        QCOMPARE(callback->NumFiles, UInt64(1));
        // The archive-open overload intentionally leaves counters unchanged.
        QCOMPARE(static_cast<IArchiveOpenCallback *>(callback)->SetTotal(&packed, &packed),
                 HRESULT(S_OK));
        QCOMPARE(progress.Sync._totalBytes, UInt64(8192));
        QCOMPARE(owner->SetCompleted(nullptr), HRESULT(S_OK));
        QCOMPARE(progress.Sync._completedBytes, completed);
    }
    void errorsOperationsAndReopen() {
        CProgressDialog progress;
        CUpdateCallback100Imp callback;
        callback.ProgressDialog = &progress;
        callback.Init();
        auto &update = static_cast<IFolderArchiveUpdateCallback &>(callback);
        auto &update2 = static_cast<IFolderArchiveUpdateCallback2 &>(callback);
        auto &open = static_cast<IArchiveOpenCallback &>(callback);
        auto &move = static_cast<IFolderArchiveUpdateCallback_MoveArc &>(callback);
        auto &scan = static_cast<IFolderScanProgress &>(callback);
        QCOMPARE(update.CompressOperation(L"новый файл"), HRESULT(S_OK));
        QVERIFY(!progress.Sync._status.IsEmpty());
        QCOMPARE(text(progress.Sync._filePath), QStringLiteral("новый файл"));
        QCOMPARE(update2.ReportUpdateOperation(999, L"каталог", 1), HRESULT(S_OK));
        QVERIFY(progress.Sync._status.IsEmpty());
        QVERIFY(progress.Sync._isDir);
        QCOMPARE(update2.ReportExtractResult(NArchive::NExtract::NOperationResult::kOK, 0, L"OK"),
                 HRESULT(S_OK));
        QCOMPARE(progress.Sync.Messages.Size(), unsigned(0));
        QCOMPARE(update2.ReportExtractResult(NArchive::NExtract::NOperationResult::kCRCError, 1,
                                             L"../опасный файл"),
                 HRESULT(S_OK));
        QVERIFY(text(progress.Sync.Messages[0]).contains(QStringLiteral("../опасный файл")));
        QVERIFY(text(progress.Sync.Messages[0])
                    .contains(text(LangString(IDS_EXTRACT_MSG_WRONG_PSW_GUESS))));
        QCOMPARE(update2.ReportExtractResult(999, 0, L"unknown"), HRESULT(S_OK));
        QCOMPARE(text(progress.Sync.Messages[1]), QStringLiteral("Error #999 : unknown"));
        QCOMPARE(update.UpdateErrorMessage(L"явная ошибка"), HRESULT(S_OK));
        QCOMPARE(scan.ScanError(L"scan", E_FAIL), HRESULT(S_OK));
        QCOMPARE(update2.OpenFileError(L"open", E_FAIL), HRESULT(S_OK));
        QCOMPARE(update2.ReadingFileError(L"read", E_FAIL), HRESULT(S_OK));
        QCOMPARE(progress.Sync.Messages.Size(), unsigned(6));
        progress.Sync.Set_Stopped(true);
        QCOMPARE(open.SetCompleted(nullptr, nullptr), HRESULT(E_ABORT));
        progress.Sync.Set_Paused(true);
        QCOMPARE(move.Before_ArcReopen(), HRESULT(S_OK));
        QVERIFY(!progress.Sync.Get_Stopped());
        QVERIFY(progress.Sync.Get_Paused()); // Source reopen clears stop, retaining pause.
        progress.Sync.Set_Paused(false);
        QCOMPARE(open.SetCompleted(nullptr, nullptr), HRESULT(S_OK));
    }
    void moveStatusAndPasswordReuse() {
        CProgressDialog progress;
        CUpdateCallback100Imp callback;
        callback.ProgressDialog = &progress;
        callback.Init();
        auto &move = static_cast<IFolderArchiveUpdateCallback_MoveArc &>(callback);
        auto &crypto = static_cast<ICryptoGetTextPassword2 &>(callback);
        QCOMPARE(move.MoveArc_Start(L"temp файл", L"архив с пробелом", 3 << 20, 2), HRESULT(S_OK));
        QCOMPARE(move.MoveArc_Progress(3 << 20, 1 << 20), HRESULT(S_OK));
        QVERIFY(text(progress.Sync._status).startsWith(QStringLiteral("33% : 1 MiB / 3 MiB")));
        QCOMPARE(text(progress.Sync._filePath), QStringLiteral("архив с пробелом"));
        const UInt64 large = UInt64(1) << 60;
        QCOMPARE(move.MoveArc_Progress(large, large / 2), HRESULT(S_OK));
        QVERIFY(text(progress.Sync._status).startsWith(QStringLiteral("50%")));
        progress.Sync.Set_Stopped(true);
        QCOMPARE(move.MoveArc_Progress(large, large / 2), HRESULT(E_ABORT));
        move.Before_ArcReopen();
        QCOMPARE(move.MoveArc_Finish(), HRESULT(S_OK));
        QVERIFY(progress.Sync._status.IsEmpty());
        QVERIFY(progress.Sync._filePath.IsEmpty());
        BSTR password = nullptr;
        Int32 defined = -1;
        QCOMPARE(crypto.CryptoGetTextPassword2(&defined, &password), HRESULT(S_OK));
        QCOMPARE(defined, Int32(0));
        QVERIFY(!password);
        callback.Password = L"fixture пароль";
        callback.PasswordIsDefined = true;
        QCOMPARE(crypto.CryptoGetTextPassword2(&defined, &password), HRESULT(S_OK));
        QCOMPARE(defined, Int32(1));
        QCOMPARE(QString::fromWCharArray(password), QStringLiteral("fixture пароль"));
        SysFreeString(password);
    }
    void workerPasswordPrompt_data() {
        QTest::addColumn<bool>("accept");
        QTest::addColumn<QString>("value");
        QTest::newRow("accept") << true << QStringLiteral("  fixture пароль 密碼 🗜  ");
        QTest::newRow("empty") << true << QString();
        QTest::newRow("cancel") << false << QStringLiteral("discarded fixture");
    }
    void workerPasswordPrompt() {
        QFETCH(bool, accept);
        QFETCH(QString, value);
        CPasswordWorker worker;
        auto *callback = new CUpdateCallback100Imp;
        worker.Callback = callback;
        callback->ProgressDialog = &worker;
        callback->Init();
        bool observed = false, ownerThread = false;
        QTimer responder;
        responder.setInterval(5);
        connect(&responder, &QTimer::timeout, &worker, [&] {
            auto *dialog = dynamic_cast<CPasswordDialog *>(QApplication::activeModalWidget());
            if (!dialog || observed)
                return;
            observed = true;
            ownerThread = dialog->thread() == QApplication::instance()->thread();
            QVERIFY(dialog->parentWidget() == &worker);
            dialog->findChild<QLineEdit *>("passwordEdit")->setText(value);
            if (accept)
                dialog->accept();
            else
                dialog->reject();
        });
        responder.start();
        QCOMPARE(worker.Create(L"Пароль", nullptr), HRESULT(S_OK));
        QVERIFY(observed);
        QVERIFY(ownerThread);
        QCOMPARE(worker.Result, HRESULT(accept ? S_OK : E_ABORT));
        QCOMPARE(callback->PasswordIsDefined, accept);
        QVERIFY(!callback->PasswordWasAsked); // The source does not set this flag here.
        if (accept)
            QCOMPARE(QString::fromWCharArray(worker.Password), value);
        else
            QVERIFY(!worker.Password);
    }
};
QTEST_MAIN(UpdateCallbackTests)
#include "update_callback_tests.moc"
