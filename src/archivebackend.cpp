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
            if (job_ == Job::Create || job_ == Job::Add)
                QFile::remove(createTemporary_);
            job_ = Job::None;
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
    process_.setWorkingDirectory(workingDirectory);
    process_.setArguments(arguments);
    process_.start();
}

void ArchiveBackend::consumeLine(const QByteArray &line) {
    QString text = QString::fromLocal8Bit(line);
    text.remove(QLatin1Char('\b'));
    text = text.trimmed();
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

void ArchiveBackend::list(const QString &archive) {
    if (busy()) {
        emit finished(false, QStringLiteral("Другая операция ещё выполняется."));
        return;
    }
    archive_ = QFileInfo(archive).absoluteFilePath();
    entries_.clear();
    listed_ = false;
    start(Job::List, {"l", "-slt", "-ba", "--", archive_});
}

bool ArchiveBackend::safePath(const QString &path) {
    QString normalized = QDir::fromNativeSeparators(path);
    normalized.replace('\\', '/');
    if (normalized.isEmpty() || normalized.startsWith('/') ||
        QRegularExpression(R"(^[A-Za-z]:)").match(normalized).hasMatch())
        return false;
    for (const QString &part : normalized.split('/')) {
        const QString base = part.section('.', 0, 0).toUpper();
        if (part.isEmpty() || part == ".." || part == "." || part.endsWith('.') ||
            part.endsWith(' ') || part.contains(':') ||
            QRegularExpression(R"([\x00-\x1f])").match(part).hasMatch() || base == "CON" ||
            base == "PRN" || base == "AUX" || base == "NUL" ||
            QRegularExpression(R"(^(COM[1-9]|LPT[1-9])$)").match(base).hasMatch())
            return false;
    }
    return true;
}

bool ArchiveBackend::validateExtraction(const QString &destination, QString *error) const {
    QFileInfo target(destination);
    if (!target.exists() || !target.isDir() || target.isSymLink()) {
        *error = QStringLiteral("Папка назначения должна существовать и не быть ссылкой: %1")
                     .arg(destination);
        return false;
    }
    QString ancestor = target.canonicalFilePath();
    while (true) {
        if (QFileInfo(ancestor).isSymLink()) {
            *error = QStringLiteral("Путь назначения содержит ссылку: %1").arg(ancestor);
            return false;
        }
        const QString parent = QFileInfo(ancestor).absolutePath();
        if (parent == ancestor)
            break;
        ancestor = parent;
    }

    QSet<QString> names;
    QSet<QString> files;
    for (const ArchiveEntry &entry : entries_) {
        if (!safePath(entry.path) || entry.link) {
            *error =
                QStringLiteral("Архив содержит небезопасный путь или ссылку: %1").arg(entry.path);
            return false;
        }
        QString relative = entry.path;
        relative.replace('\\', '/');
        const QString key = relative.normalized(QString::NormalizationForm_C).toCaseFolded();
        if (names.contains(key)) {
            *error = QStringLiteral("Архив содержит конфликт имён: %1").arg(entry.path);
            return false;
        }
        names.insert(key);
        if (!entry.directory)
            files.insert(key);
        QString candidate = target.absoluteFilePath();
        for (const QString &part : relative.split('/')) {
            candidate = QDir(candidate).filePath(part);
            const QFileInfo existing(candidate);
            if (existing.isSymLink() || (existing.exists() && !existing.isDir())) {
                *error =
                    QStringLiteral("Путь назначения занят файлом или ссылкой: %1").arg(candidate);
                return false;
            }
        }
        if (QFileInfo(candidate).exists()) {
            *error = QStringLiteral("В папке назначения уже существует: %1").arg(entry.path);
            return false;
        }
    }
    for (const QString &name : names) {
        QString parent = name;
        while (parent.contains('/')) {
            parent = parent.section('/', 0, -2);
            if (files.contains(parent)) {
                *error = QStringLiteral("Архив содержит конфликт файла и папки: %1").arg(name);
                return false;
            }
        }
    }
    return true;
}

void ArchiveBackend::extract(const QString &archive, const QString &destination,
                             const QStringList &selected) {
    if (busy()) {
        emit finished(false, QStringLiteral("Другая операция ещё выполняется."));
        return;
    }
    if (!listed_ || QFileInfo(archive).absoluteFilePath() != archive_) {
        emit finished(false, QStringLiteral("Сначала откройте архив и проверьте его содержимое."));
        return;
    }
    QString validationError;
    if (!validateExtraction(destination, &validationError)) {
        emit finished(false, validationError);
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
    QStringList arguments = {"x",     "-y",
                             "-bsp1", "-bb1",
                             "-spd",  "-o" + QFileInfo(destination).absoluteFilePath(),
                             "--",    QFileInfo(archive).absoluteFilePath()};
    arguments += selected;
    start(Job::Extract, arguments);
}

void ArchiveBackend::create(const QString &archive, const QStringList &sources) {
    if (busy()) {
        emit finished(false, QStringLiteral("Другая операция ещё выполняется."));
        return;
    }
    const QString suffix = QFileInfo(archive).suffix().toLower();
    if ((suffix != "7z" && suffix != "zip") || sources.isEmpty()) {
        emit finished(false, QStringLiteral("Выберите файлы или папку и имя архива .7z или .zip."));
        return;
    }
    const QFileInfo target(archive);
    if (!QFileInfo(target.absolutePath()).isDir() || target.exists() || target.isSymLink()) {
        emit finished(false,
                      QStringLiteral("Папка назначения не найдена или архив уже существует: %1")
                          .arg(target.absoluteFilePath()));
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
    QStringList arguments = {"a", "-y", "-bsp1", "-bb1", "-t" + suffix, "--"};
    for (const QString &source : sources) {
        if (!QFileInfo::exists(source)) {
            emit finished(false, QStringLiteral("Файл или папка не найдены: %1").arg(source));
            return;
        }
        const QString relative = QDir(root).relativeFilePath(QFileInfo(source).absoluteFilePath());
        if (relative == ".." || relative.startsWith("../") || QDir::isAbsolutePath(relative)) {
            emit finished(false, QStringLiteral("Файлы должны находиться на одном диске."));
            return;
        }
        arguments << relative;
    }
    for (const QString &source : sources) {
        const QFileInfo info(source);
        if (info.isDir()) {
            const QString relative =
                QDir(info.canonicalFilePath()).relativeFilePath(target.absoluteFilePath());
            if (relative == "." || (relative != ".." && !relative.startsWith("../") &&
                                    !QDir::isAbsolutePath(relative))) {
                emit finished(false,
                              QStringLiteral("Нельзя сохранять архив внутри выбранной папки."));
                return;
            }
        }
    }
    createTarget_ = target.absoluteFilePath();
    createTemporary_ = target.absolutePath() + "/.sevenzip-" +
                       QUuid::createUuid().toString(QUuid::WithoutBraces) + "." + suffix;
    arguments.insert(6, createTemporary_);
    start(Job::Create, arguments, root);
}

void ArchiveBackend::add(const QString &archive, const QStringList &sources) {
    if (busy()) {
        emit finished(false, QStringLiteral("Другая операция ещё выполняется."));
        return;
    }
    const QFileInfo target(archive);
    if (!listed_ || target.absoluteFilePath() != archive_ || !target.isFile() ||
        target.isSymLink() || sources.isEmpty()) {
        emit finished(false, QStringLiteral("Откройте существующий архив и выберите источники."));
        return;
    }
    const QString suffix = target.suffix().toLower();
    if (suffix != "7z" && suffix != "zip") {
        emit finished(false, QStringLiteral("Поддерживаются только архивы .7z и .zip."));
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
            emit finished(false, QStringLiteral("Недоступный источник: %1").arg(source));
            return;
        }
        QStringList paths = {relative};
        if (info.isDir()) {
            const QString destination =
                QDir(info.canonicalFilePath()).relativeFilePath(target.absoluteFilePath());
            if (destination == "." || (destination != ".." && !destination.startsWith("../") &&
                                       !QDir::isAbsolutePath(destination))) {
                emit finished(false, QStringLiteral("Архив находится внутри выбранной папки."));
                return;
            }
            QDirIterator iterator(info.absoluteFilePath(),
                                  QDir::AllEntries | QDir::NoDotAndDotDot | QDir::Hidden |
                                      QDir::System,
                                  QDirIterator::Subdirectories);
            while (iterator.hasNext()) {
                iterator.next();
                if (iterator.fileInfo().isSymLink()) {
                    emit finished(false, QStringLiteral("Источники не должны содержать ссылки: %1")
                                             .arg(iterator.filePath()));
                    return;
                }
                paths << QDir(root).relativeFilePath(iterator.filePath());
            }
        }
        for (const QString &path : paths) {
            if (!safePath(path)) {
                emit finished(false, QStringLiteral("Небезопасное имя источника: %1").arg(path));
                return;
            }
            const QString key = path.normalized(QString::NormalizationForm_C).toCaseFolded();
            if (existing.contains(key) || incoming.contains(key)) {
                emit finished(false,
                              QStringLiteral("Имя уже есть в архиве или выборе: %1").arg(path));
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
    if (!QFile::copy(createTarget_, createTemporary_)) {
        QFile::remove(createTemporary_);
        emit finished(false, QStringLiteral("Не удалось подготовить копию архива для записи."));
        return;
    }
    QStringList arguments = {"a", "-y", "-bsp1", "-bb1", "-t" + suffix, "--", createTemporary_};
    arguments += sourceNames;
    start(Job::Add, arguments, root);
}

void ArchiveBackend::cancel() {
    if (!busy())
        return;
    cancelled_ = true;
    process_.kill();
}

QList<ArchiveEntry> ArchiveBackend::parseListing(const QString &output) {
    QList<ArchiveEntry> result;
    ArchiveEntry current;
    bool havePath = false;
    auto flush = [&] {
        if (havePath)
            result.append(current);
        current = ArchiveEntry{};
        havePath = false;
    };
    for (const QString &line : output.split('\n')) {
        const QString trimmed = line.trimmed();
        if (trimmed.isEmpty()) {
            flush();
            continue;
        }
        const int separator = trimmed.indexOf(" = ");
        if (separator < 0)
            continue;
        const QString key = trimmed.left(separator);
        const QString value = trimmed.mid(separator + 3);
        if (key == "Path") {
            flush();
            current.path = value;
            havePath = true;
        } else if (key == "Size")
            current.size = value.toLongLong();
        else if (key == "Packed Size")
            current.packedSize = value.toLongLong();
        else if (key == "Modified")
            current.modified = value;
        else if (key == "Attributes") {
            current.directory = value.startsWith('D');
            current.link |= value.contains('l') || value.contains("Link", Qt::CaseInsensitive);
        } else if (key == "Symbolic Link" || key == "Hard Link" || key == "Reparse Point")
            current.link = true;
    }
    flush();
    return result;
}

void ArchiveBackend::complete(int exitCode, QProcess::ExitStatus status) {
    if (job_ == Job::None)
        return;
    consumeOutput(process_.readAll(), true);
    const Job completed = job_;
    job_ = Job::None;
    if (cancelled_) {
        if (completed == Job::Create || completed == Job::Add)
            QFile::remove(createTemporary_);
        emit finished(false, QStringLiteral("Операция отменена."));
        return;
    }
    if (status != QProcess::NormalExit || exitCode != 0) {
        if (completed == Job::Create || completed == Job::Add)
            QFile::remove(createTemporary_);
        const QString detail = output_.trimmed().right(600);
        emit finished(false,
                      QStringLiteral("7zz завершился с кодом %1. %2").arg(exitCode).arg(detail));
        return;
    }
    if (completed == Job::Create && !QFile::rename(createTemporary_, createTarget_)) {
        QFile::remove(createTemporary_);
        emit finished(false,
                      QStringLiteral("Не удалось сохранить новый архив: %1").arg(createTarget_));
        return;
    }
    if (completed == Job::Add) {
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
        emit entriesReady(entries_);
    }
    emit progress(100, QString());
    emit finished(true, completed == Job::List      ? QStringLiteral("Архив открыт.")
                        : completed == Job::Extract ? QStringLiteral("Извлечение завершено.")
                        : completed == Job::Create  ? QStringLiteral("Архив создан.")
                                                    : QStringLiteral("Файлы добавлены."));
}
