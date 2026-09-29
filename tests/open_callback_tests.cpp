#include "CPP/7zip/UI/Common/ZipRegistry.h"
#include "CPP/7zip/UI/FileManager/OpenCallback.h"
#include "CPP/7zip/UI/FileManager/PasswordDialog.h"
#include "Windows/PropVariant.h"
#include <QApplication>
#include <QCheckBox>
#include <QFile>
#include <QLineEdit>
#include <QTemporaryDir>
#include <QTimer>
#include <QtTest>

namespace {
struct CPasswordRequest {
    COpenArchiveCallback Callback;
    HRESULT Result = E_FAIL;
    BSTR Password = nullptr;
    ~CPasswordRequest() { SysFreeString(Password); }
    static THREAD_FUNC_DECL Run(void *param) {
        auto &request = *static_cast<CPasswordRequest *>(param);
        CProgressCloser closer(request.Callback.ProgressDialog);
        request.Result = request.Callback.Open_CryptoGetTextPassword(&request.Password);
        return 0;
    }
};
} // namespace

class OpenCallbackTests : public QObject {
    Q_OBJECT
  private slots:
    void countersAndStop() {
        COpenArchiveCallback callback;
        UInt64 files = 9, bytes = 4096;
        QCOMPARE(callback.Open_SetTotal(&files, &bytes), HRESULT(S_OK));
        QCOMPARE(callback.ProgressDialog.Sync._totalFiles, files);
        QCOMPARE(callback.ProgressDialog.Sync._totalBytes, bytes);
        QVERIFY(callback.ProgressDialog.Sync._filesProgressMode);
        files = 3;
        bytes = 127;
        QCOMPARE(callback.Open_SetCompleted(&files, &bytes), HRESULT(S_OK));
        QCOMPARE(callback.ProgressDialog.Sync._curFiles, files);
        QCOMPARE(callback.ProgressDialog.Sync._completedBytes, bytes);
        QCOMPARE(callback.Open_SetTotal(nullptr, nullptr), HRESULT(S_OK));
        QCOMPARE(callback.ProgressDialog.Sync._totalFiles, UInt64(-1));
        QVERIFY(!callback.ProgressDialog.Sync._filesProgressMode);
        callback.ProgressDialog.Sync.Set_Stopped(true);
        QCOMPARE(callback.Open_SetTotal(&files, &bytes), HRESULT(E_ABORT));
        QCOMPARE(callback.Open_CheckBreak(), HRESULT(E_ABORT));
        QCOMPARE(callback.Open_Finished(), HRESULT(E_ABORT));
    }

    void volumeSafetyAndRetainedStream() {
        QTemporaryDir temp;
        QVERIFY(temp.isValid());
        const QString name = QStringLiteral("том й space.002");
        QFile file(temp.filePath(name));
        QVERIFY(file.open(QIODevice::WriteOnly));
        QCOMPARE(file.write("volume fixture"), qint64(14));
        file.close();
        auto *implementation = new COpenCallbackImp;
        CMyComPtr<IArchiveOpenCallback> owner = implementation;
        CMyComPtr<IArchiveOpenVolumeCallback> volumes;
        QCOMPARE(owner.QueryInterface(IID_IArchiveOpenVolumeCallback, &volumes), HRESULT(S_OK));
        const std::wstring prefix = (temp.path() + '/').toStdWString();
        const std::wstring nativeName = name.toStdWString();
        QCOMPARE(implementation->Init2(us2fs(prefix.c_str()), us2fs(nativeName.c_str())),
                 HRESULT(S_OK));
        CMyComPtr<IInStream> stream;
        for (const QString &unsafe : {QStringLiteral("../outside"), temp.filePath(name),
                                      QStringLiteral("dir/../../outside"), QStringLiteral(".")}) {
            const std::wstring path = unsafe.toStdWString();
            QCOMPARE(volumes->GetStream(path.c_str(), &stream), HRESULT(S_FALSE));
            QVERIFY(!stream);
        }
        QCOMPARE(volumes->GetStream(nativeName.c_str(), &stream), HRESULT(S_OK));
        QVERIFY(stream);
        QCOMPARE(implementation->FileNames.Size(), unsigned(1));
        QVERIFY(implementation->FileNames_WasUsed[0]);
        char data[15]{};
        UInt32 read = 0;
        QCOMPARE(stream->Read(data, 6, &read), HRESULT(S_OK));
        QCOMPARE(read, UInt32(6));
        QCOMPARE(QByteArray(data, int(read)), QByteArray("volume"));
        // Native LRU close/reopen must preserve the current logical stream position.
        implementation->Volumes.Streams[0].LocalPos = 6;
        implementation->Volumes.CloseFile(0);
        volumes.Release();
        owner.Release();
        QCOMPARE(stream->Read(data, 8, &read), HRESULT(S_OK));
        QCOMPARE(read, UInt32(8));
        QCOMPARE(QByteArray(data, int(read)), QByteArray(" fixture"));
        stream.Release(); // last reference also releases volume owner safely
    }

    void passwordReuseAndReopenRouting() {
        COpenArchiveCallback callback;
        callback.PasswordIsDefined = true;
        callback.Password = L"fixture";
        auto *implementation = new COpenCallbackImp;
        CMyComPtr<IArchiveOpenCallback> owner = implementation;
        implementation->Callback = &callback;
        CMyComPtr<ICryptoGetTextPassword> passwordCallback;
        QCOMPARE(owner.QueryInterface(IID_ICryptoGetTextPassword, &passwordCallback),
                 HRESULT(S_OK));
        BSTR password = nullptr;
        QCOMPARE(passwordCallback->CryptoGetTextPassword(&password), HRESULT(S_OK));
        QCOMPARE(QString::fromWCharArray(password), QStringLiteral("fixture"));
        SysFreeString(password);
        QVERIFY(callback.PasswordWasAsked);
        QVERIFY(implementation->PasswordWasAsked);
        UInt64 files = 2, bytes = 99;
        QCOMPARE(owner->SetTotal(&files, &bytes), HRESULT(S_OK));
        QCOMPARE(callback.ProgressDialog.Sync._totalBytes, bytes);
        auto *reopenImplementation = new COpenCallbackImp;
        CMyComPtr<IArchiveOpenCallback> reopen = reopenImplementation;
        reopenImplementation->ReOpenCallback = owner;
        bytes = 123;
        QCOMPARE(reopen->SetCompleted(nullptr, &bytes), HRESULT(S_OK));
        QCOMPARE(callback.ProgressDialog.Sync._completedBytes, bytes);
    }

    void passwordPresentationAndPersistence_data() {
        QTest::addColumn<bool>("accept");
        QTest::newRow("accepted-empty") << true;
        QTest::newRow("cancelled") << false;
    }
    void passwordPresentationAndPersistence() {
        QFETCH(bool, accept);
        QTemporaryDir temp;
        QVERIFY(temp.isValid());
        CPasswordRequest request;
        request.Callback.SettingsPath = temp.filePath("options.ini");
        QVERIFY(!NExtract::Read_ShowPassword(request.Callback.SettingsPath));
        bool promptSeen = false;
        QTimer responder;
        connect(&responder, &QTimer::timeout, this, [&] {
            auto *dialog = qobject_cast<QDialog *>(QApplication::activeModalWidget());
            if (!dialog || dialog->objectName() != "passwordDialog")
                return;
            promptSeen = true;
            auto *show = dialog->findChild<QCheckBox *>("passwordShow");
            auto *edit = dialog->findChild<QLineEdit *>("passwordEdit");
            QVERIFY(show);
            QVERIFY(edit);
            QVERIFY(!show->isChecked());
            show->setChecked(true);
            edit->clear();
            responder.stop();
            if (accept)
                dialog->accept();
            else
                dialog->reject();
        });
        responder.start(5);
        NWindows::CThread worker;
        QCOMPARE(worker.Create(CPasswordRequest::Run, &request), WRes(0));
        request.Callback.StartProgressDialog(UString(L"Open fixture"), worker);
        responder.stop();
        QVERIFY(promptSeen);
        QCOMPARE(request.Result, HRESULT(accept ? S_OK : E_ABORT));
        QCOMPARE(request.Callback.PasswordIsDefined, accept);
        QCOMPARE(NExtract::Read_ShowPassword(request.Callback.SettingsPath), accept);
        if (accept) {
            QVERIFY(request.Password);
            QCOMPARE(SysStringLen(request.Password), UINT(0));
        }
        QVERIFY(request.Callback.PasswordWasAsked);
    }
};

QTEST_MAIN(OpenCallbackTests)
#include "open_callback_tests.moc"
