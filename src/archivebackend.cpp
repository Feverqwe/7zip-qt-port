#include "archivebackend.h"

#include <QDir>
#include <QFileInfo>
#include <QRegularExpression>
#include <QStandardPaths>

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
    const QString normalized = QDir::fromNativeSeparators(path);
    if (normalized.isEmpty() || normalized.startsWith('/') || normalized.startsWith("//") ||
        QRegularExpression(R"(^[A-Za-z]:)").match(normalized).hasMatch())
        return false;
    for (const QString &part : normalized.split('/')) {
        if (part == ".." || part == ".")
            return false;
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
    for (const ArchiveEntry &entry : entries_) {
        if (!safePath(entry.path) || entry.link) {
            emit finished(
                false,
                QStringLiteral("Архив содержит небезопасный путь или ссылку: %1").arg(entry.path));
            return;
        }
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
    QStringList arguments = {
        "a", "-y", "-bsp1", "-bb1", "-t" + suffix, "--", QFileInfo(archive).absoluteFilePath()};
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
    start(Job::Create, arguments, root);
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
            current.link = value.contains('l') || value.contains("Link", Qt::CaseInsensitive);
        }
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
        emit finished(false, QStringLiteral("Операция отменена."));
        return;
    }
    if (status != QProcess::NormalExit || exitCode != 0) {
        const QString detail = output_.trimmed().right(600);
        emit finished(false,
                      QStringLiteral("7zz завершился с кодом %1. %2").arg(exitCode).arg(detail));
        return;
    }
    if (completed == Job::List) {
        entries_ = parseListing(output_);
        listed_ = true;
        emit entriesReady(entries_);
    }
    emit progress(100, QString());
    emit finished(true, completed == Job::List      ? QStringLiteral("Архив открыт.")
                        : completed == Job::Extract ? QStringLiteral("Извлечение завершено.")
                                                    : QStringLiteral("Архив создан."));
}
