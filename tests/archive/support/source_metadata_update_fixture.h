#pragma once
#include "filemanager/UpdateCallback100.h"
#include <QFileInfo>

class CLinkUpdateFault final : public IFolderArchiveUpdateCallback,
                               public ICompressProgressInfo,
                               public CMyUnknownImp {
    Z7_COM_UNKNOWN_IMP_2(IFolderArchiveUpdateCallback, ICompressProgressInfo)
    CMyComPtr<IFolderArchiveUpdateCallback> callback_;
    HRESULT failure_;
    bool &observed_;
    HRESULT Check(const UInt64 *value) {
        if (!value || !*value)
            return S_OK;
        observed_ = true;
        return failure_;
    }

  public:
    CLinkUpdateFault(IFolderArchiveUpdateCallback *callback, HRESULT failure, bool &observed)
        : callback_(callback), failure_(failure), observed_(observed) {}
    Z7_COM7F_IMF(SetTotal(UInt64 value)) override { return callback_->SetTotal(value); }
    Z7_COM7F_IMF(SetCompleted(const UInt64 *value)) override {
        RINOK(Check(value))
        return callback_->SetCompleted(value);
    }
    Z7_COM7F_IMF(SetRatioInfo(const UInt64 *value, const UInt64 *)) override {
        return Check(value);
    }
    Z7_COM7F_IMF(CompressOperation(const wchar_t *name)) override {
        return callback_->CompressOperation(name);
    }
    Z7_COM7F_IMF(DeleteOperation(const wchar_t *name)) override {
        return callback_->DeleteOperation(name);
    }
    Z7_COM7F_IMF(OperationResult(Int32 value)) override {
        return callback_->OperationResult(value);
    }
    Z7_COM7F_IMF(UpdateErrorMessage(const wchar_t *message)) override {
        return callback_->UpdateErrorMessage(message);
    }
    Z7_COM7F_IMF(SetNumFiles(UInt64 count)) override { return callback_->SetNumFiles(count); }
};
class CLinkUpdateWorker final : public CProgressThreadVirt {
    CMyComPtr<IFolderOperations> operations_;
    QString source_;
    HRESULT ProcessVirt() override {
        CMyComPtr2_Create<IFolderArchiveUpdateCallback, CUpdateCallback100Imp> ui;
        ui->ProgressDialog = this;
        ui->Init();
        CMyComPtr<IFolderArchiveUpdateCallback> callback = ui.Interface();
        if (Failure != S_OK)
            callback = new CLinkUpdateFault(ui.Interface(), Failure, Observed);
        const std::wstring parent = QFileInfo(source_).absolutePath().toStdWString();
        const std::wstring name = QFileInfo(source_).fileName().toStdWString();
        const wchar_t *names[] = {name.c_str()};
        return operations_->CopyFrom(0, parent.c_str(), names, 1, callback);
    }

  public:
    HRESULT Failure = S_OK;
    bool Observed = false;
    CLinkUpdateWorker(IFolderOperations *operations, const QString &source)
        : operations_(operations), source_(source) {}
};
