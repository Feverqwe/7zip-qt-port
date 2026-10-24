#include "7zip/UI/GUI/CompressDialogRes.h"
#include "CPP/7zip/UI/Common/Update.h"
#include "CPP/7zip/UI/FileManager/LangUtils.h"
#include "CPP/7zip/UI/GUI/CompressDialog.h"
#include "CPP/7zip/UI/GUI/UpdateGUI.h"
#include "archivebackend.h"
#include "qtlanguageservices.h"

bool ArchiveBackend::compressionProperties(const QString &suffix, const QString &password,
                                           bool encryptHeaders, const CompressionOptions &options,
                                           CObjectVector<CProperty> *properties, QString *error) {
    const auto &formatInfo = NCompressDialog::GetFormatInfo(suffix);
    const auto *actionSet = NCompressDialog::FindUpdateActionSet(options.UpdateMode);
    if (options.DeleteAfterCompressing &&
        (!actionSet || CheckDeleteAfterCompressingActionSet(*actionSet) != S_OK)) {
        *error = QCoreApplication::translate(
            "archivebackend_compression",
            "Delete after compression is available only in Add and replace mode.");
        return false;
    }
#ifndef Q_OS_WIN
    if (options.NtSecurity.Val || options.AltStreams.Val) {
        *error =
            QCoreApplication::translate("archivebackend_compression",
                                        "NTFS security and streams are available only on Windows.");
        return false;
    }
    if (options.PreserveATime.Def && options.PreserveATime.Val) {
        *error = QCoreApplication::translate(
            "archivebackend_compression",
            "The engine supports preserving access time only on Windows.");
        return false;
    }
#endif
    const QStringList methods = NCompressDialog::GetMethodNames(suffix);
    const QString requestedMethod = QString::fromWCharArray(options.Method.Ptr());
    if (options.Level > 9 || !(formatInfo.LevelsMask & (UInt32(1) << options.Level)) ||
        (!requestedMethod.isEmpty() && !methods.contains(requestedMethod))) {
        *error =
            QCoreApplication::translate("archivebackend_compression",
                                        "Invalid compression parameters for the selected format.");
        return false;
    }
    const QString method = requestedMethod.isEmpty() ? (suffix == "7z" ? QStringLiteral("LZMA2")
                                                        : methods.isEmpty() ? QString()
                                                                            : methods.front())
                                                     : requestedMethod;
    const int methodID =
        options.Level == 0 && suffix != "tar" ? -1 : NCompressDialog::GetMethodID(method, suffix);
    const auto dictionary = NCompressDialog::SetDictionary2(methodID, UInt32(options.Level));
    const UInt64 dict = options.Dict64 != UInt64(-1)        ? options.Dict64
                        : dictionary.AutoDict == UInt32(-1) ? UInt64(-1)
                                                            : UInt64(dictionary.AutoDict);
    const auto system = NCompressDialog::GetSystemInfo();
    UInt32 blockSizeLog = UInt32(-1);
    if (options.SolidIsSpecified) {
        if (options.SolidBlockSize == UInt64(-1))
            blockSizeLog = 64;
        else if (options.SolidBlockSize == 0)
            blockSizeLog = 0;
        else
            for (UInt32 i = 20; i <= 36; i++)
                if (options.SolidBlockSize == (UInt64(1) << i)) {
                    blockSizeLog = i;
                    break;
                }
    }
    const auto threadOptions = NCompressDialog::SetNumThreads2(
        suffix, methodID, UInt32(options.Level), dict, blockSizeLog, system, options.MemUsage);
    if (options.NumThreads != UInt32(-1) && !threadOptions.Values.contains(options.NumThreads)) {
        *error = QCoreApplication::translate(
            "archivebackend_compression",
            "The thread count is invalid for the selected compression method.");
        return false;
    }
    bool validSolid = !options.SolidIsSpecified;
    if (options.SolidIsSpecified) {
        const auto solid = NCompressDialog::SetSolidBlockSize2(
            suffix, UInt32(options.Level), NCompressDialog::GetMethodID(method, suffix),
            UInt64(-1));
        for (UInt32 log : solid.Values) {
            NCompressDialog::CInfo info;
            NCompressDialog::OnOK(log, info);
            if (info.SolidBlockSize == options.SolidBlockSize) {
                validSolid = true;
                break;
            }
        }
    }
    if ((options.Dict64 != UInt64(-1) &&
         (!NCompressDialog::SetDictionary2(NCompressDialog::GetMethodID(method, suffix),
                                           UInt32(options.Level))
               .Values.contains(options.Dict64))) ||
        (options.Order != UInt32(-1) &&
         !NCompressDialog::SetOrder2(NCompressDialog::GetMethodID(method, suffix),
                                     UInt32(options.Level))
              .Values.contains(options.Order)) ||
        !validSolid ||
        (options.Level == 0 && (options.Dict64 != UInt64(-1) || options.Order != UInt32(-1)))) {
        *error = QCoreApplication::translate("archivebackend_compression",
                                             "The dictionary, word or solid block size is invalid "
                                             "for the selected compression method.");
        return false;
    }
    UInt64 decompressMemory;
    const UInt64 memUsage = NCompressDialog::GetMemoryUsage_Threads_Dict_DecompMem(
        suffix, methodID, UInt32(options.Level),
        options.NumThreads != UInt32(-1) ? options.NumThreads : threadOptions.AutoNumThreads, dict,
        blockSizeLog, decompressMemory);
    UString memoryError;
    if (!CCompressDialog::OnOK(memUsage, system, options.MemUsage, memoryError)) {
        *error = QString::fromWCharArray(memoryError.Ptr());
        return false;
    }
    if ((!options.EncryptionMethod.isEmpty() &&
         options.EncryptionMethod.compare("AES256", Qt::CaseInsensitive) != 0) ||
        (suffix == "7z" && !options.EncryptionMethod.isEmpty()) ||
        (suffix == "zip" && encryptHeaders)) {
        *error = QCoreApplication::translate("archivebackend_compression",
                                             "Invalid encryption method for the selected format.");
        return false;
    }
    const auto passwordError =
        CCompressDialog::CheckPassword(password, password, true, suffix, options.EncryptionMethod);
    if (passwordError != CCompressDialog::kPasswordOK) {
        *error = QString::fromWCharArray(
            LangString(passwordError == CCompressDialog::kPasswordTooLong ? IDS_PASSWORD_TOO_LONG
                                                                          : IDS_PASSWORD_USE_ASCII)
                .Ptr());
        return false;
    }
    // The request already carries the original GUI property DTO. Source
    // SetOutProperties/parser owns method selection, overrides and time props.
    NCompressDialog::CInfo info = options;
    if (options.Level == 0 && suffix != "tar")
        info.Method.Empty();
    info.OrderMode = NCompressDialog::GetOrderMode(methodID);
    info.EncryptHeadersIsAllowed = !password.isEmpty() && suffix == "7z";
    info.EncryptHeaders = encryptHeaders;
    if (password.isEmpty())
        info.EncryptionMethod.clear();
    *properties = NCompressDialog::GetCompressionProperties(info, suffix == "7z");
    return true;
}
