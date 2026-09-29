#include "qtcallbackpassword.h"
#include "CPP/7zip/UI/FileManager/PasswordDialog.h"
#include "CPP/7zip/UI/FileManager/ProgressDialog2.h"
#include <QMetaObject>
#include <QThread>

bool AskCallbackPassword(CProgressDialog &parent, UString &password) {
    bool accepted = false;
    const auto show = [&] {
        CPasswordDialog dialog(&parent);
        dialog.OnInit();
        if (dialog.exec() != QDialog::Accepted)
            return;
        const std::wstring text = dialog.Password.toStdWString();
        password = text.c_str();
        accepted = true;
    };
    // Qt widgets must be constructed, executed and destroyed on their owner thread.
    // The source worker is joined before its progress parent can be destroyed.
    if (QThread::currentThread() == parent.thread())
        show();
    else
        QMetaObject::invokeMethod(&parent, show, Qt::BlockingQueuedConnection);
    return accepted;
}
