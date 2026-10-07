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
        *error = QtLocalText("Delete after compression is available only in Add and replace mode.",
                             "Удаление после сжатия доступно только в режиме добавления и замены.");
        return false;
    }
#ifndef Q_OS_WIN
    if (options.NtSecurity.Val || options.AltStreams.Val) {
        *error = QtLocalText("NTFS security and streams are available only on Windows.",
                             "Защита NTFS и потоки доступны только в Windows.");
        return false;
    }
    if (options.PreserveATime.Def && options.PreserveATime.Val) {
        *error = QtLocalText("The engine supports preserving access time only on Windows.",
                             "Сохранение времени доступа поддерживается движком только в Windows.");
        return false;
    }
#endif
    const QStringList methods = NCompressDialog::GetMethodNames(suffix);
    const QString requestedMethod = QString::fromWCharArray(options.Method.Ptr());
    if (options.Level > 9 || !(formatInfo.LevelsMask & (UInt32(1) << options.Level)) ||
        (!requestedMethod.isEmpty() && !methods.contains(requestedMethod))) {
        *error = QtLocalText("Invalid compression parameters for the selected format.",
                             "Недопустимые параметры сжатия для выбранного формата.");
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
        *error = QtLocalText("The thread count is invalid for the selected compression method.",
                             "Число потоков не подходит для выбранного метода сжатия.");
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
        *error = QtLocalText("The dictionary, word or solid block size is invalid "
                             "for the selected compression method.",
                             "Размер словаря, слова или непрерывного блока не подходит "
                             "для выбранного метода сжатия.");
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
        *error = QtLocalText("Invalid encryption method for the selected format.",
                             "Недопустимый метод шифрования для выбранного формата.");
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

bool ArchiveBackend::compressionSwitches(const QString &suffix, const QString &password,
                                         bool encryptHeaders, const CompressionOptions &options,
                                         QStringList *switches, QString *error) {
    CObjectVector<CProperty> properties;
    if (!compressionProperties(suffix, password, encryptHeaders, options, &properties, error))
        return false;
    switches->clear();
    // UpdateGUI.cpp's lifecycle projection is transported through the source
    // ArchiveCommandLine.cpp switches, outside method properties.
    if (options.SetArcMTime.Val)
        switches->append("-stl");
    if (options.PreserveATime.Def && options.PreserveATime.Val)
        switches->append("-ssp");
    if (options.OpenShareForWrite)
        switches->append("-ssw");
    // ArchiveCommandLine.cpp uses kMinus and SetBoolPair for kSymLinks.
    if (options.SymLinks.Def || options.SymLinks.Val)
        switches->append(options.SymLinks.Val ? "-snl" : "-snl-");
    if (options.HardLinks.Def || options.HardLinks.Val)
        switches->append(options.HardLinks.Val ? "-snh" : "-snh-");
    if (options.AltStreams.Def || options.AltStreams.Val)
        switches->append(options.AltStreams.Val ? "-sns" : "-sns-");
    if (options.NtSecurity.Val)
        switches->append("-sni");
    FOR_VECTOR(i, properties) {
        const CProperty &property = properties[i];
        QString argument = "-m" + QString::fromWCharArray(property.Name.Ptr());
        if (!property.Value.IsEmpty())
            argument += '=' + QString::fromWCharArray(property.Value.Ptr());
        switches->append(argument);
    }
    if (!password.isEmpty())
        switches->append("-p");
    return true;
}
