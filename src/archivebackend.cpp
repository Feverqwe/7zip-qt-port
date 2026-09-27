#include "archivebackend.h"

#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>
#include <QSet>
#include <QStandardPaths>
#include <QUuid>

ArchiveBackend::ArchiveBackend(QObject *parent, QString executable) : QObject(parent) {
    if (executable.isEmpty()) {
        executable = QStandardPaths::findExecutable("7zz");
        if (executable.isEmpty()) {
            for (const QString &candidate : {"/opt/homebrew/bin/7zz", "/usr/local/bin/7zz"}) {
                if (QFileInfo(candidate).isExecutable()) {
                    executable = candidate;
                    break;
                }
            }
        }
        if (executable.isEmpty())
            executable = "7zz";
    }
    process_.setProgram(executable);
    process_.setProcessChannelMode(QProcess::MergedChannels);
    connect(&process_, &QProcess::readyRead, this, [this] { consumeOutput(process_.readAll()); });
    connect(&process_, qOverload<int, QProcess::ExitStatus>(&QProcess::finished), this,
            &ArchiveBackend::complete);
    connect(&process_, &QProcess::errorOccurred, this, [this](QProcess::ProcessError error) {
        if (error == QProcess::FailedToStart && job_ != Job::None) {
            if (job_ == Job::Create || job_ == Job::Add || job_ == Job::Delete ||
                job_ == Job::Rename || job_ == Job::ReplaceDelete || job_ == Job::ReplaceAdd)
                QFile::remove(createTemporary_);
            job_ = Job::None;
            pendingPassword_.clear();
            extractPassword_.clear();
            emit finished(
                false,
                QStringLiteral(
                    "Не удалось запустить 7zz. Установите официальный 7zz и добавьте его в PATH."));
        }
    });
}

bool ArchiveBackend::busy() const { return job_ != Job::None; }

void ArchiveBackend::start(Job job, const QStringList &arguments, const QString &workingDirectory) {
    if (busy()) {
        emit finished(false, QStringLiteral("Другая операция ещё выполняется."));
        return;
    }
    job_ = job;
    output_.clear();
    pendingOutput_.clear();
    cancelled_ = false;
    passwordPrompted_ = false;
    process_.setWorkingDirectory(workingDirectory);
    QStringList processArguments = arguments;
    processArguments.insert(1, "-sccUTF-8");
    process_.setArguments(processArguments);
    process_.start();
    if (!pendingPassword_.isEmpty())
        providePassword(pendingPassword_);
}

void ArchiveBackend::providePassword(const QString &password) {
    if (!busy() || password.isEmpty())
        return;
    pendingPassword_ = password;
    if (job_ == Job::ExtractCheck)
        extractPassword_ = password;
    process_.write(password.toUtf8() + '\n');
}

void ArchiveBackend::consumeLine(const QByteArray &line) {
    QString text = QString::fromUtf8(line);
    text.remove(QLatin1Char('\b'));
    text = text.trimmed();
    if (!pendingPassword_.isEmpty())
        text.replace(pendingPassword_, QStringLiteral("[скрыто]"));
    if (text.isEmpty())
        return;
    output_ += text + '\n';
    emit outputReceived(text);
    static const QRegularExpression percent(R"((?:^|\s)(\d{1,3})%)");
    auto match = percent.globalMatch(text);
    int value = -1;
    QString detail;
    while (match.hasNext()) {
        const auto found = match.next();
        value = found.captured(1).toInt();
        detail = text.mid(found.capturedEnd()).trimmed();
    }
    if (value >= 0 && value <= 100) {
        if (detail.startsWith('+') || detail.startsWith('-'))
            detail = detail.mid(1).trimmed();
        emit progress(value, detail);
    } else if (text.startsWith("+ ") || text.startsWith("- ")) {
        emit progress(-1, text.mid(2).trimmed());
    }
}

void ArchiveBackend::consumeOutput(const QByteArray &chunk, bool flush) {
    pendingOutput_ += chunk;
    if (!passwordPrompted_ && pendingPassword_.isEmpty() &&
        pendingOutput_.contains("Enter password:")) {
        passwordPrompted_ = true;
        emit passwordRequired();
    }
    qsizetype start = 0;
    while (true) {
        const qsizetype cr = pendingOutput_.indexOf('\r', start);
        const qsizetype lf = pendingOutput_.indexOf('\n', start);
        const qsizetype end = cr < 0 ? lf : (lf < 0 ? cr : qMin(cr, lf));
        if (end < 0)
            break;
        if (end > start)
            consumeLine(pendingOutput_.mid(start, end - start));
        start = end + 1;
    }
    if (start > 0)
        pendingOutput_.remove(0, start);
    if (flush && !pendingOutput_.isEmpty()) {
        consumeLine(pendingOutput_);
        pendingOutput_.clear();
    }
}

void ArchiveBackend::list(const QString &archive, const QString &password) {
    if (busy()) {
        emit finished(false, QStringLiteral("Другая операция ещё выполняется."));
        return;
    }
    archive_ = QFileInfo(archive).absoluteFilePath();
    entries_.clear();
    listed_ = false;
    encrypted_ = false;
    pendingPassword_ = password;
    start(Job::List, {"l", "-slt", "--", archive_});
}

void ArchiveBackend::extract(const QString &archive, const QString &destination,
                             const QStringList &selected, const QString &password,
                             const ExtractionOptions &options) {
    if (busy()) {
        emit finished(false, QStringLiteral("Другая операция ещё выполняется."));
        return;
    }
    if (!listed_ || QFileInfo(archive).absoluteFilePath() != archive_) {
        emit finished(false, QStringLiteral("Сначала откройте архив и проверьте его содержимое."));
        return;
    }
    for (const QString &name : selected) {
        bool found = false;
        for (const ArchiveEntry &entry : entries_)
            found |= entry.path == name;
        if (!found) {
            emit finished(false,
                          QStringLiteral("Выбранный файл отсутствует в архиве: %1").arg(name));
            return;
        }
    }
    QString validationError;
    if (!validateExtraction(destination, selected, options, &validationError)) {
        emit finished(false, validationError);
        return;
    }
    QStringList arguments = {options.pathMode == ExtractionOptions::PathMode::NoPaths ? "e" : "x",
                             "-y",
                             "-bsp1",
                             "-bb1",
                             "-spd",
                             "-o" + QFileInfo(destination).absoluteFilePath(),
                             "--",
                             QFileInfo(archive).absoluteFilePath()};
    if (options.overwriteMode == ExtractionOptions::OverwriteMode::SkipExisting)
        arguments.insert(2, "-aos");
    else if (options.overwriteMode == ExtractionOptions::OverwriteMode::OverwriteExisting)
        arguments.insert(2, "-aoa");
    arguments += selected;
    pendingPassword_ = password;
    if (encrypted_) {
        extractArguments_ = arguments;
        extractPassword_ = password;
        start(Job::ExtractCheck, {"t", "-bsp1", "-bb1", "--", archive_});
    } else
        start(Job::Extract, arguments);
}

void ArchiveBackend::test(const QString &archive, const QString &password) {
    if (busy()) {
        emit finished(false, QStringLiteral("Другая операция ещё выполняется."));
        return;
    }
    if (!listed_ || QFileInfo(archive).absoluteFilePath() != archive_) {
        emit finished(false, QStringLiteral("Сначала откройте архив и проверьте его содержимое."));
        return;
    }
    pendingPassword_ = password;
    start(Job::Test, {"t", "-bsp1", "-bb1", "--", archive_});
}

void ArchiveBackend::hash(const QStringList &paths, const QString &method) {
    if (busy()) {
        emit finished(false, QStringLiteral("Другая операция ещё выполняется."));
        return;
    }
    static const QStringList methods = {"CRC32",    "CRC64",    "XXH64",  "MD5",
                                        "SHA1",     "SHA256",   "SHA384", "SHA512",
                                        "SHA3-256", "BLAKE2sp", "*"};
    if (!methods.contains(method) || paths.isEmpty()) {
        emit finished(false, QStringLiteral("Не выбраны файлы или метод хеширования."));
        return;
    }
    for (const QString &path : paths) {
        const QFileInfo info(path);
        if (!info.exists() || info.isSymLink()) {
            emit finished(false,
                          QStringLiteral("Файл отсутствует или является ссылкой: %1").arg(path));
            return;
        }
    }
    QStringList arguments = {"h", "-bsp1", "-bb1", "-spd", "-scrc" + method, "--"};
    arguments += paths;
    pendingPassword_.clear();
    start(Job::Hash, arguments);
}

void ArchiveBackend::cancel() {
    if (!busy())
        return;
    cancelled_ = true;
    pendingPassword_.clear();
    process_.kill();
}

void ArchiveBackend::complete(int exitCode, QProcess::ExitStatus status) {
    if (job_ == Job::None)
        return;
    consumeOutput(process_.readAll(), true);
    pendingPassword_.clear();
    const Job completed = job_;
    job_ = Job::None;
    if (cancelled_) {
        extractPassword_.clear();
        if (completed == Job::Create || completed == Job::Add || completed == Job::Delete ||
            completed == Job::Rename || completed == Job::ReplaceDelete ||
            completed == Job::ReplaceAdd)
            QFile::remove(createTemporary_);
        emit finished(false, QStringLiteral("Операция отменена."));
        return;
    }
    if (status != QProcess::NormalExit || exitCode != 0) {
        extractPassword_.clear();
        if (completed == Job::Create || completed == Job::Add || completed == Job::Delete ||
            completed == Job::Rename || completed == Job::ReplaceDelete ||
            completed == Job::ReplaceAdd)
            QFile::remove(createTemporary_);
        const bool wrongPassword =
            output_.contains(QStringLiteral("Wrong password"), Qt::CaseInsensitive);
        const QString detail = output_.trimmed().right(600);
        emit finished(
            false, wrongPassword
                       ? QStringLiteral("Неверный пароль или повреждённый защищённый архив.")
                       : QStringLiteral("7zz завершился с кодом %1. %2").arg(exitCode).arg(detail));
        return;
    }
    if (completed == Job::ExtractCheck) {
        pendingPassword_ = extractPassword_;
        extractPassword_.clear();
        start(Job::Extract, extractArguments_);
        extractArguments_.clear();
        return;
    }
    if (completed == Job::ReplaceDelete) {
        start(Job::ReplaceAdd,
              {"a", "-y", "-bsp1", "-bb1", "-spd", "--", createTemporary_, replacementEntry_},
              replacementRoot_);
        return;
    }
    if (completed == Job::Create && !QFile::rename(createTemporary_, createTarget_)) {
        QFile::remove(createTemporary_);
        emit finished(false,
                      QStringLiteral("Не удалось сохранить новый архив: %1").arg(createTarget_));
        return;
    }
    if (completed == Job::Add || completed == Job::Delete || completed == Job::Rename ||
        completed == Job::ReplaceAdd) {
        if (!QFile::rename(createTarget_, addBackup_)) {
            QFile::remove(createTemporary_);
            emit finished(false,
                          QStringLiteral("Не удалось заменить архив: %1").arg(createTarget_));
            return;
        }
        if (!QFile::rename(createTemporary_, createTarget_)) {
            const bool restored = QFile::rename(addBackup_, createTarget_);
            QFile::remove(createTemporary_);
            emit finished(
                false,
                restored
                    ? QStringLiteral("Не удалось сохранить обновлённый архив: %1")
                          .arg(createTarget_)
                    : QStringLiteral("Не удалось восстановить архив. Копия: %1").arg(addBackup_));
            return;
        }
        QFile::remove(addBackup_);
        listed_ = false;
        entries_.clear();
    }
    if (completed == Job::List) {
        entries_ = parseListing(output_);
        listed_ = true;
        ArchiveInfo info;
        const QString header = output_.section(QStringLiteral("----------\n"), 0, 0);
        for (const QString &line : header.split('\n')) {
            if (line.startsWith(QStringLiteral("Type = ")))
                info.format = line.mid(7);
            else if (line.startsWith(QStringLiteral("Physical Size = "))) {
                bool valid = false;
                const qint64 size = line.mid(16).toLongLong(&valid);
                if (valid)
                    info.physicalSize = size;
            }
        }
        for (const ArchiveEntry &entry : entries_) {
            if (entry.directory)
                ++info.folders;
            else {
                ++info.files;
                info.unpackedSize += entry.size;
            }
        }
        info.encrypted = output_.contains(QStringLiteral("Encrypted = +")) ||
                         header.contains(QStringLiteral("7zAES"));
        encrypted_ = info.encrypted;
        emit infoReady(info);
        emit entriesReady(entries_);
    }
    emit progress(100, QString());
    emit finished(true, completed == Job::List      ? QStringLiteral("Архив открыт.")
                        : completed == Job::Extract ? QStringLiteral("Извлечение завершено.")
                        : completed == Job::Test ? QStringLiteral("Ошибок при проверке не найдено.")
                        : completed == Job::Hash ? QStringLiteral("Контрольные суммы рассчитаны.")
                        : completed == Job::Create ? QStringLiteral("Архив создан.")
                        : completed == Job::Delete ? QStringLiteral("Объекты удалены из архива.")
                        : completed == Job::Rename ? QStringLiteral("Объект переименован.")
                        : completed == Job::ReplaceAdd ? QStringLiteral("Файл обновлён в архиве.")
                                                       : QStringLiteral("Файлы добавлены."));
}
