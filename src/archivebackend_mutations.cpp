#include "7zip/Archive/Common/ItemNameUtils.h"
#include "7zip/UI/FileManager/resourceGui.h"
#include "7zip/UI/GUI/CompressDialogRes.h"
#include "CPP/7zip/UI/Common/Update.h"
#include "CPP/7zip/UI/FileManager/LangUtils.h"
#include "CPP/7zip/UI/FileManager/RegistryUtils.h"
#include "CPP/7zip/UI/GUI/CompressDialog.h"
#include "CPP/7zip/UI/GUI/UpdateGUI.h"
#include "archivebackend.h"
#include "archivesfxmodule.h"
#include "qtarchivecreationlifecycle.h"
#include "qtlanguageservices.h"

#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>
#include <QSet>
#include <QUuid>

namespace {
QString mutationPath(const QString &path) {
    UString native(path.toStdWString().c_str());
    NArchive::NItemName::ReplaceSlashes_OsToUnix(native);
    return QString::fromWCharArray(native.Ptr());
}
} // namespace

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
    const auto methodID =
        options.Level == 0 ? NCompressDialog::kCopy : NCompressDialog::GetMethodID(method, suffix);
    const auto dictionary = NCompressDialog::SetDictionary2(methodID, UInt32(options.Level));
    const UInt64 dict = options.Dict64 != UInt64(-1) ? options.Dict64
                        : dictionary.AutoDict        ? UInt64(dictionary.AutoDict)
                                                     : UInt64(-1);
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
    const UInt64 memLimit = NCompressDialog::Get_MemUse_Bytes(options.MemUsage, system);
    if ((options.MemUsage.IsDefined || system.RamSizeDefined) && memUsage != UInt64(-1) &&
        memUsage > memLimit) {
        UString usage;
        LangString_OnlyFromLangFile(IDS_MEM_REQUIRED_MEM_SIZE, usage);
        if (usage.IsEmpty()) {
            usage = LangString(IDT_COMPRESS_MEMORY);
            usage.RemoveChar(L':');
        }
        UString message;
        SetErrorMessage_MemUsage(message, memUsage, system.RamSize, memLimit, usage);
        *error = QString::fromWCharArray(message.Ptr());
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

void ArchiveBackend::create(const QString &archive, const QStringList &sources,
                            const QString &password, bool encryptHeaders,
                            const CompressionOptions &options) {
    if (busy()) {
        emit finished(false, QtLocalText("Another operation is still running.",
                                         "Другая операция ещё выполняется."));
        return;
    }
    if (sources.isEmpty()) {
        lastResult_ = E_INVALIDARG;
        emit finished(false, QtLocalText("Select source files or folders.",
                                         "Выберите файлы или папки для архива."));
        return;
    }
    QString suffix, formatError;
    if (!selectCreationFormat(archive, sources, options, suffix, formatError)) {
        lastResult_ = E_INVALIDARG;
        emit finished(false, formatError);
        return;
    }
    QStringList compressionArguments;
    QString compressionError;
    if (!compressionSwitches(suffix, password, encryptHeaders, options, &compressionArguments,
                             &compressionError)) {
        emit finished(false, compressionError);
        return;
    }
    QString validatedSfxModule;
    if (options.SFXMode) {
        const auto module = sfxModule(options.SfxModule);
        const auto method =
            options.Level == 0
                ? NCompressDialog::kCopy
                : NCompressDialog::GetMethodID(options.Method.IsEmpty()
                                                   ? QString("LZMA2")
                                                   : QString::fromWCharArray(options.Method.Ptr()),
                                               suffix);
        if (suffix != "7z" || !NCompressDialog::IsMethodSupportedBySfx(method) ||
            !module.available()) {
            lastResult_ = E_INVALIDARG;
            emit finished(
                false, module.available()
                           ? QtLocalText("The selected format or method is not supported by SFX.",
                                         "Выбранный формат или метод не поддерживается SFX.")
                           : module.error());
            return;
        }
        validatedSfxModule = module.path();
        if (options.VolumeSizes.IsEmpty())
            compressionArguments.append("-sfx" + validatedSfxModule);
    }
    const QFileInfo target(archive);
    if (!QFileInfo(target.absolutePath()).isDir() || target.exists() || target.isSymLink()) {
        emit finished(false,
                      QtLocalText("Destination folder not found or archive already exists: %1",
                                  "Папка назначения не найдена или архив уже существует: %1")
                          .arg(target.absoluteFilePath()));
        return;
    }
    FOR_VECTOR(i, options.VolumeSizes) {
        if (options.VolumeSizes[i] == 0 || options.VolumeSizes[i] > UInt64(INT64_MAX)) {
            emit finished(false,
                          QString::fromWCharArray(LangString(IDS_INCORRECT_VOLUME_SIZE).Ptr()));
            return;
        }
    }
    QString volumeTarget = archive;
    if (options.SFXMode) {
#ifdef Q_OS_WIN
        if (volumeTarget.endsWith(".exe", Qt::CaseInsensitive))
            volumeTarget.chop(4);
#endif
        // Original SFX OpenStreamOrFile looks for <self>.7z.001 on Unix too.
        // GetFinalVolPath omits this extension with Unix's empty SFX extension.
        volumeTarget += ".7z";
    }
    if (!options.VolumeSizes.IsEmpty()) {
        QString error;
        if (!checkVolumeTargets(volumeTarget, &error)) {
            emit finished(false, error);
            return;
        }
    }
    QList<CreateStage> stages;
    QSet<QString> logicalNames;
    auto appendStage = [&stages](const QString &directory, const QString &name) {
        for (CreateStage &stage : stages)
            if (stage.directory == directory) {
                stage.arguments.append(name);
                return;
            }
        stages.append({directory, {name}});
    };
    for (const QString &source : sources) {
        const QFileInfo info(source);
        if (!info.exists() && !(options.SymLinks.Val && info.isSymLink())) {
            emit finished(
                false, QtLocalText("File or folder not found: %1", "Файл или папка не найдены: %1")
                           .arg(source));
            return;
        }
        const QString absolute = QDir::fromNativeSeparators(info.absoluteFilePath());
        QString directory;
        QString logical;
        if (options.PathMode == NWildcard::k_RelatPath) {
            directory = info.absolutePath();
            logical = info.fileName();
        } else if (options.PathMode == NWildcard::k_FullPath) {
#ifdef Q_OS_WIN
            const QRegularExpression rootPattern(R"(^(?:[A-Za-z]:/|//[^/]+/[^/]+/))");
            const auto match = rootPattern.match(absolute);
            if (!match.hasMatch()) {
                emit finished(false, QtLocalText("Could not determine the source root.",
                                                 "Не удалось определить корень источника."));
                return;
            }
            directory = match.captured();
#else
            directory = QDir::rootPath();
#endif
            logical = absolute.mid(directory.length());
        } else if (options.PathMode == NWildcard::k_AbsPath) {
            directory = QDir::rootPath();
            logical = absolute;
        } else {
            emit finished(false, QtLocalText("Invalid archive path mode.",
                                             "Недопустимый режим путей архива."));
            return;
        }
        if (logical.isEmpty() || logical.contains('*') || logical.contains('?') ||
            logical.startsWith('@') ||
            (options.PathMode != NWildcard::k_AbsPath && !sourceUpdatePathIsSafe(logical))) {
            emit finished(false,
                          QtLocalText("Invalid source path: %1", "Недопустимый путь источника: %1")
                              .arg(source));
            return;
        }
        const QString key = logical.normalized(QString::NormalizationForm_C).toCaseFolded();
        if (logicalNames.contains(key)) {
            emit finished(false, QtLocalText("Sources produce the same archive name: %1",
                                             "Источники дают одинаковое имя в архиве: %1")
                                     .arg(logical));
            return;
        }
        logicalNames.insert(key);
        appendStage(directory, logical);
    }
    for (const QString &source : sources) {
        const QFileInfo info(source);
        if (info.isDir()) {
            const QString relative =
                QDir(info.canonicalFilePath()).relativeFilePath(target.absoluteFilePath());
            if (relative == "." || (relative != ".." && !relative.startsWith("../") &&
                                    !QDir::isAbsolutePath(relative))) {
                emit finished(false,
                              QtLocalText("Cannot save the archive inside the selected folder.",
                                          "Нельзя сохранять архив внутри выбранной папки."));
                return;
            }
        }
    }
    createTarget_ = target.absoluteFilePath();
    createNativeSfx_ = options.SFXMode;
    createSfxModule_ = validatedSfxModule;
    createVolumeTarget_ = QFileInfo(volumeTarget).absoluteFilePath();
    const FString workDir = NCompressDialog::GetCompressionWorkingDir(
        UString(createTarget_.toStdWString().c_str()), FmSettingsPath());
    if (!workDir.IsEmpty())
        compressionArguments.append("-w" + QString::fromWCharArray(fs2us(workDir)));
    createTemporary_ = target.absolutePath() + "/.sevenzip-" +
                       QUuid::createUuid().toString(QUuid::WithoutBraces) + "." + suffix;
    bool firstStage = true;
    for (CreateStage &stage : stages) {
        QStringList arguments = {"a", "-y", "-bsp1", "-bb1", "-spd", "-t" + suffix};
        arguments += compressionArguments;
        // Later roots update the encrypted output created by the first stage.
        // Reuse its opened password instead of prompting for a new one too.
        if (!firstStage && suffix == "7z" && !password.isEmpty() && encryptHeaders)
            arguments.removeAll("-p");
        firstStage = false;
        if (options.PathMode == NWildcard::k_AbsPath)
            arguments << "-spf";
        arguments << "--" << createTemporary_;
        arguments += stage.arguments;
        stage.arguments = arguments;
    }
    createStages_ = stages;
    createVolumeSizes_ = options.VolumeSizes;
    createSetArcMTime_ = options.SetArcMTime.Val;
    createPassword_ = password;
    creationLifecycle_.reset();
    if (options.DeleteAfterCompressing) {
        creationLifecycle_ =
            std::make_unique<QtArchiveCreationLifecycle>(sources, options.SymLinks);
        const quint64 generation = ++creationGeneration_;
        connect(creationLifecycle_.get(), &QThread::finished, this, [this, generation] {
            if (creationLifecycle_ && creationGeneration_ == generation)
                finishCreationLifecycle();
        });
        const quint64 progressGeneration = beginWorkerProgress();
        const QString caption = QString::fromWCharArray(LangString(IDS_SCANNING).Ptr());
        creationLifecycle_->Progress = [this, progressGeneration, caption](const QString &path,
                                                                           bool) {
            postWorkerProgress(progressGeneration, 0, caption + QStringLiteral(" ") + path);
        };
        job_ = Job::CreatePrepare;
        cancelled_ = false;
        creationLifecycle_->start();
        return;
    }
    const CreateStage first = createStages_.takeFirst();
    pendingPassword_ = password;
    start(Job::Create, first.arguments, first.directory);
}

void ArchiveBackend::add(const QString &archive, const QStringList &sources) {
    if (busy()) {
        emit finished(false, QtLocalText("Another operation is still running.",
                                         "Другая операция ещё выполняется."));
        return;
    }
    if (!restoreListedArchive(archive)) {
        emit finished(false, QtLocalText("Open the archive and inspect its contents first.",
                                         "Сначала откройте архив и проверьте его содержимое."));
        return;
    }
    if (encrypted_) {
        emit finished(false, QtLocalText("Adding to a protected archive is currently unavailable.",
                                         "Добавление в защищённый архив пока недоступно."));
        return;
    }
    const QFileInfo target(archive);
    if (!listed_ || target.absoluteFilePath() != archive_ || !target.isFile() ||
        target.isSymLink() || sources.isEmpty()) {
        emit finished(false, QtLocalText("Open an existing archive and select sources.",
                                         "Откройте существующий архив и выберите источники."));
        return;
    }
    const QString suffix = target.suffix().toLower();
    if (suffix != "7z" && suffix != "zip") {
        emit finished(false, QtLocalText("Only .7z and .zip archives are supported.",
                                         "Поддерживаются только архивы .7z и .zip."));
        return;
    }
    QString root = QFileInfo(sources.first()).absolutePath();
    while (!root.isEmpty()) {
        bool containsAll = true;
        for (const QString &source : sources) {
            const QString relative =
                QDir(root).relativeFilePath(QFileInfo(source).absoluteFilePath());
            if (relative == ".." || relative.startsWith("../") || QDir::isAbsolutePath(relative)) {
                containsAll = false;
                break;
            }
        }
        if (containsAll)
            break;
        const QString parent = QFileInfo(root).absolutePath();
        if (parent == root)
            break;
        root = parent;
    }
    QSet<QString> existing;
    for (const ArchiveEntry &entry : entries_)
        existing.insert(entry.path.normalized(QString::NormalizationForm_C).toCaseFolded());
    QSet<QString> incoming;
    QStringList sourceNames;
    for (const QString &source : sources) {
        const QFileInfo info(source);
        const QString relative = QDir(root).relativeFilePath(info.absoluteFilePath());
        if (!info.exists() || info.isSymLink() || relative == ".." || relative.startsWith("../") ||
            QDir::isAbsolutePath(relative) ||
            info.absoluteFilePath() == target.absoluteFilePath()) {
            emit finished(
                false,
                QtLocalText("Source unavailable: %1", "Недоступный источник: %1").arg(source));
            return;
        }
        QStringList paths = {relative};
        if (info.isDir()) {
            const QString destination =
                QDir(info.canonicalFilePath()).relativeFilePath(target.absoluteFilePath());
            if (destination == "." || (destination != ".." && !destination.startsWith("../") &&
                                       !QDir::isAbsolutePath(destination))) {
                emit finished(false, QtLocalText("The archive is inside the selected folder.",
                                                 "Архив находится внутри выбранной папки."));
                return;
            }
            QDirIterator iterator(info.absoluteFilePath(),
                                  QDir::AllEntries | QDir::NoDotAndDotDot | QDir::Hidden |
                                      QDir::System,
                                  QDirIterator::Subdirectories);
            while (iterator.hasNext()) {
                iterator.next();
                if (iterator.fileInfo().isSymLink()) {
                    emit finished(false, QtLocalText("Sources must not contain links: %1",
                                                     "Источники не должны содержать ссылки: %1")
                                             .arg(iterator.filePath()));
                    return;
                }
                paths << QDir(root).relativeFilePath(iterator.filePath());
            }
        }
        for (const QString &path : paths) {
            if (!safePath(path)) {
                emit finished(
                    false, QtLocalText("Unsafe source name: %1", "Небезопасное имя источника: %1")
                               .arg(path));
                return;
            }
            const QString key = path.normalized(QString::NormalizationForm_C).toCaseFolded();
            if (existing.contains(key) || incoming.contains(key)) {
                emit finished(false,
                              QtLocalText("Name already exists in the archive or selection: %1",
                                          "Имя уже есть в архиве или выборе: %1")
                                  .arg(path));
                return;
            }
            incoming.insert(key);
        }
        sourceNames << relative;
    }
    createTarget_ = target.absoluteFilePath();
    createTemporary_ = target.absolutePath() + "/.sevenzip-" +
                       QUuid::createUuid().toString(QUuid::WithoutBraces) + "." + suffix;
    addBackup_ = target.absolutePath() + "/.sevenzip-backup-" +
                 QUuid::createUuid().toString(QUuid::WithoutBraces) + "." + suffix;
    QStringList arguments = {"a", "-y", "-bsp1", "-bb1", "-t" + suffix, "--", createTemporary_};
    arguments += sourceNames;
    prepareArchiveCopy(createTarget_, createTemporary_,
                       [this, arguments, root] { start(Job::Add, arguments, root); });
}

bool ArchiveBackend::prepareMutation(const QString &archive) {
    if (busy()) {
        emit finished(false, QtLocalText("Another operation is still running.",
                                         "Другая операция ещё выполняется."));
        return false;
    }
    const QFileInfo target(archive);
    const QString suffix = target.suffix().toLower();
    if (!restoreListedArchive(archive) || !target.isFile() || target.isSymLink() || encrypted_ ||
        (suffix != "7z" && suffix != "zip")) {
        emit finished(false, QtLocalText("Open an unencrypted .7z or .zip archive.",
                                         "Откройте незашифрованный архив .7z или .zip."));
        return false;
    }
    createTarget_ = target.absoluteFilePath();
    createTemporary_ = target.absolutePath() + "/.sevenzip-" +
                       QUuid::createUuid().toString(QUuid::WithoutBraces) + "." + suffix;
    addBackup_ = target.absolutePath() + "/.sevenzip-backup-" +
                 QUuid::createUuid().toString(QUuid::WithoutBraces) + "." + suffix;
    return true;
}

void ArchiveBackend::remove(const QString &archive, const QStringList &paths) {
    if (paths.isEmpty())
        return;
    if (!restoreListedArchive(archive)) {
        emit finished(false, QtLocalText("Open the archive and inspect its contents first.",
                                         "Сначала откройте архив и проверьте его содержимое."));
        return;
    }
    QSet<QString> known;
    for (const ArchiveEntry &entry : entries_)
        known.insert(mutationPath(entry.path));
    for (const QString &path : paths) {
        if (!known.contains(mutationPath(path)) || !safePath(path) || path.contains('*') ||
            path.contains('?') || path.startsWith('@')) {
            emit finished(
                false,
                QtLocalText("Archive item not found: %1", "Объект архива не найден: %1").arg(path));
            return;
        }
    }
    if (!prepareMutation(archive))
        return;
    QStringList arguments = {"d", "-y", "-bsp1", "-bb1", "--", createTemporary_};
    arguments += paths;
    prepareArchiveCopy(createTarget_, createTemporary_,
                       [this, arguments] { start(Job::Delete, arguments); });
}

void ArchiveBackend::renameEntry(const QString &archive, const QString &oldName,
                                 const QString &newName) {
    const QString oldPath = mutationPath(oldName);
    const QString newPath = mutationPath(newName);
    if (!restoreListedArchive(archive)) {
        emit finished(false, QtLocalText("Open the archive and inspect its contents first.",
                                         "Сначала откройте архив и проверьте его содержимое."));
        return;
    }
    bool found = false;
    bool directory = false;
    for (const ArchiveEntry &entry : entries_) {
        const QString path = mutationPath(entry.path);
        if (path == oldPath) {
            found = true;
            directory = entry.directory;
        } else if (path.startsWith(oldPath + '/')) {
            found = true;
            directory = true;
        }
    }
    const QString oldPrefix = oldPath + '/';
    QSet<QString> existing;
    QSet<QString> renamed;
    for (const ArchiveEntry &entry : entries_) {
        const QString entryPath = mutationPath(entry.path);
        const bool member = entryPath == oldPath || (directory && entryPath.startsWith(oldPrefix));
        const QString path = member ? newPath + entryPath.mid(oldPath.size()) : entryPath;
        const QString key = path.normalized(QString::NormalizationForm_C).toCaseFolded();
        if (member) {
            if (!safePath(entry.path) || entry.link) {
                emit finished(
                    false, QtLocalText("Unsafe archive item: %1", "Небезопасный объект архива: %1")
                               .arg(entry.path));
                return;
            }
            if (renamed.contains(key)) {
                emit finished(false, QtLocalText("The name already exists in the archive.",
                                                 "Имя уже есть в архиве."));
                return;
            }
            renamed.insert(key);
        } else
            existing.insert(key);
    }
    if (!found || !safePath(newPath) || oldPath.contains('*') || oldPath.contains('?') ||
        oldPath.startsWith('@') || newPath.contains('*') || newPath.contains('?') ||
        newPath.startsWith('@') || oldPath == newPath ||
        oldPath.section('/', 0, -2) != newPath.section('/', 0, -2)) {
        emit finished(false, QtLocalText("Renaming this item is unavailable.",
                                         "Переименование этого объекта недоступно."));
        return;
    }
    const QString targetKey = newPath.normalized(QString::NormalizationForm_C).toCaseFolded();
    for (const QString &key : existing) {
        if (renamed.contains(key) ||
            (directory && (key == targetKey || key.startsWith(targetKey + '/')))) {
            emit finished(false, QtLocalText("The name already exists in the archive.",
                                             "Имя уже есть в архиве."));
            return;
        }
    }
    if (!prepareMutation(archive))
        return;
    prepareArchiveCopy(createTarget_, createTemporary_, [this, oldPath, newPath] {
        start(Job::Rename, {"rn", "-y", "-bsp1", "-bb1", "--", createTemporary_, oldPath, newPath});
    });
}

void ArchiveBackend::replaceEntry(const QString &archive, const QString &entry,
                                  const QString &sourceRoot) {
    if (!restoreListedArchive(archive)) {
        emit finished(false, QtLocalText("Open the archive and inspect its contents first.",
                                         "Сначала откройте архив и проверьте его содержимое."));
        return;
    }
    bool found = false;
    for (const ArchiveEntry &item : entries_)
        found |= item.path == entry && !item.directory && !item.link;
    if (!found || !safePath(entry) || entry.contains('*') || entry.contains('?') ||
        entry.startsWith('@')) {
        emit finished(false, QtLocalText("This archive file cannot be replaced.",
                                         "Этот файл архива нельзя заменить."));
        return;
    }
    const QDir root(sourceRoot);
    const QFileInfo source(root.filePath(entry));
    if (!root.exists() || !source.isFile() || source.isSymLink() ||
        !source.canonicalFilePath().startsWith(root.canonicalPath() + '/')) {
        emit finished(false, QtLocalText("Modified file not found.", "Изменённый файл не найден."));
        return;
    }
    QString part = root.absolutePath();
    for (const QString &component : entry.split('/')) {
        part = QDir(part).filePath(component);
        if (QFileInfo(part).isSymLink()) {
            emit finished(false, QtLocalText("Links in temporary files are not allowed.",
                                             "Ссылки во временном файле запрещены."));
            return;
        }
    }
    if (!prepareMutation(archive))
        return;
    replacementEntry_ = entry;
    replacementRoot_ = root.absolutePath();
    prepareArchiveCopy(createTarget_, createTemporary_, [this, entry] {
        start(Job::ReplaceDelete,
              {"d", "-y", "-bsp1", "-bb1", "-spd", "--", createTemporary_, entry});
    });
}

bool ArchiveBackend::publishProcessMutation() {
    if (!QFile::rename(createTarget_, addBackup_)) {
        QFile::remove(createTemporary_);
        emit finished(false, QtLocalText("Could not replace archive: %1",
                                         QStringLiteral("Не удалось заменить архив: %1"))
                                 .arg(createTarget_));
        return false;
    }
    if (!QFile::rename(createTemporary_, createTarget_)) {
        const bool restored = QFile::rename(addBackup_, createTarget_);
        QFile::remove(createTemporary_);
        emit finished(
            false, restored
                       ? QtLocalText("Could not save updated archive: %1",
                                     QStringLiteral("Не удалось сохранить обновлённый архив: %1"))
                             .arg(createTarget_)
                       : QtLocalText("Could not restore archive. Backup: %1",
                                     QStringLiteral("Не удалось восстановить архив. Копия: %1"))
                             .arg(addBackup_));
        return false;
    }
    QFile::remove(addBackup_);
    listed_ = false;
    entries_.clear();
    listedSourceArchives_.remove(createTarget_);
    return true;
}
