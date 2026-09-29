#include "qtcallbackdialogs.h"
#include "CPP/7zip/UI/FileManager/ProgressDialog2.h"
#include <QMetaObject>
#include <QThread>

HRESULT RunCallbackDialog(CProgressDialog &parent, const std::function<HRESULT()> &body) {
    if (QThread::currentThread() == parent.thread())
        return body();
    parent.WaitCreating();
    HRESULT result = E_FAIL;
    QMetaObject::invokeMethod(&parent, [&] { result = body(); }, Qt::BlockingQueuedConnection);
    return result;
}
