#pragma once
#include "7zip/IPassword.h"
#include "7zip/UI/FileManager/IFolder.h"
#include "Common/StringConvert.h"
#include "archive/agent/IFolderArchive.h"

class CArchiveCopyCallback final : public IFolderOperationsExtractCallback,
                                   public IFolderArchiveExtractCallback,
                                   public ICryptoGetTextPassword,
                                   public CMyUnknownImp {
    Z7_COM_UNKNOWN_IMP_3(IFolderOperationsExtractCallback, IFolderArchiveExtractCallback,
                         ICryptoGetTextPassword)
  public:
    bool Cancel = false;
    unsigned Errors = 0;
    Int32 LastResult = 0;
    const wchar_t *Password = nullptr;
    unsigned PasswordRequests = 0;
    Z7_COM7F_IMF(CryptoGetTextPassword(BSTR *password)) override {
        ++PasswordRequests;
        return Password ? StringToBstr(Password, password) : E_ABORT;
    }
    Z7_COM7F_IMF(SetTotal(UInt64)) override { return Cancel ? E_ABORT : S_OK; }
    Z7_COM7F_IMF(SetCompleted(const UInt64 *)) override { return Cancel ? E_ABORT : S_OK; }
    Z7_COM7F_IMF(SetCurrentFilePath(const wchar_t *)) override { return S_OK; }
    Z7_COM7F_IMF(SetNumFiles(UInt64)) override { return S_OK; }
    Z7_COM7F_IMF(ShowMessage(const wchar_t *)) override {
        ++Errors;
        return S_OK;
    }
    Z7_COM7F_IMF(AskWrite(const wchar_t *, Int32, const FILETIME *, const UInt64 *,
                          const wchar_t *path, BSTR *result, Int32 *answer))
    override {
        *answer = 1;
        return StringToBstr(path, result);
    }
    Z7_COM7F_IMF(AskOverwrite(const wchar_t *, const FILETIME *, const UInt64 *, const wchar_t *,
                              const FILETIME *, const UInt64 *, Int32 *answer))
    override {
        *answer = NOverwriteAnswer::kYes;
        return S_OK;
    }
    Z7_COM7F_IMF(PrepareOperation(const wchar_t *, Int32, Int32, const UInt64 *)) override {
        return Cancel ? E_ABORT : S_OK;
    }
    Z7_COM7F_IMF(MessageError(const wchar_t *)) override {
        ++Errors;
        return S_OK;
    }
    Z7_COM7F_IMF(SetOperationResult(Int32 result, Int32)) override {
        LastResult = result;
        if (result)
            ++Errors;
        return S_OK;
    }
};
