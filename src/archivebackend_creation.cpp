#include "archivebackend.h"
#include "qtarchivecreationlifecycle.h"
#include "qtlanguageservices.h"

#include "7zip/UI/FileManager/resourceGui.h"
#include "7zip/UI/GUI/CompressDialogRes.h"
#include "CPP/7zip/UI/FileManager/LangUtils.h"
#include "CPP/7zip/UI/FileManager/RegistryUtils.h"
#include "CPP/7zip/UI/GUI/UpdateGUI.h"
#include "archivesfxmodule.h"

#include <QDir>
#include <QFileInfo>
#include <QRegularExpression>
#include <QSet>
#include <QUuid>

void ArchiveBackend::finishCreationLifecycle() {
    if (!creationLifecycle_ || (job_ != Job::CreatePrepare && job_ != Job::CreateDeleteInputs))
        return;
    creationLifecycle_->wait();
    finishWorkerProgress();
    const bool preparing = job_ == Job::CreatePrepare;
    lastResult_ = creationLifecycle_->Result;
    if (cancelled_)
        lastResult_ = E_ABORT;
    const QString error =
        cancelled_ ? QtLocalText("Operation canceled.", QStringLiteral("Операция отменена."))
                   : creationLifecycle_->Error;
    job_ = Job::None;
    if (lastResult_ != S_OK) {
        creationLifecycle_.reset();
        createStages_.clear();
        createPassword_.clear();
        emit finished(false, error);
        return;
    }
    if (preparing) {
        const CreateStage first = createStages_.takeFirst();
        pendingPassword_ = createPassword_;
        start(Job::Create, first.arguments, first.directory);
    } else {
        creationLifecycle_.reset();
        emit finished(true, QtLocalText("Archive created.", QStringLiteral("Архив создан.")));
    }
}

bool ArchiveBackend::startCreationDeletion() {
    if (!creationLifecycle_)
        return false;
    job_ = Job::CreateDeleteInputs;
    const quint64 progressGeneration = beginWorkerProgress();
    const QString caption = QtLocalText("Deleting: %1", QStringLiteral("Удаление: %1"));
    creationLifecycle_->Progress = [this, progressGeneration, caption](const QString &path, bool) {
        postWorkerProgress(progressGeneration, 100, caption.arg(path));
    };
    creationLifecycle_->beginDeletion();
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
