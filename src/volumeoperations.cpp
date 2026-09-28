#include "volumeoperations.h"
#include "CPP/7zip/UI/FileManager/PanelSplitFile.h"
#include "CPP/7zip/UI/FileManager/SplitUtils.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>
#include <QUuid>
#include <limits>

VolumeOperation::VolumeOperation(Mode mode, QString source, QString destination, qint64 volumeSize,
                                 QObject *parent)
    : VolumeOperation(mode, std::move(source), std::move(destination), QList<qint64>{volumeSize},
                      parent) {}

VolumeOperation::VolumeOperation(Mode mode, QString source, QString destination,
                                 QList<qint64> volumeSizes, QObject *parent)
    : QThread(parent), mode_(mode), source_(std::move(source)),
      destination_(std::move(destination)), volumeSizes_(std::move(volumeSizes)) {}

void VolumeOperation::cancel() { canceled_ = true; }

QList<qint64> VolumeOperation::parseSizes(const QString &text) {
    std::vector<UInt64> values;
    if (!ParseVolumeSizes(text.trimmed().toStdWString(), values) || values.empty())
        return {};
    QList<qint64> result;
    for (UInt64 value : values) {
        if (value == 0 || value > UInt64(std::numeric_limits<qint64>::max()))
            return {};
        result.append(qint64(value));
    }
    return result;
}

qint64 VolumeOperation::parseSize(const QString &text) {
    const auto sizes = parseSizes(text);
    return sizes.size() == 1 ? sizes.first() : 0;
}

QStringList VolumeOperation::discoverParts(const QString &firstPart, QString *error) {
    const QFileInfo first(firstPart);
    const QRegularExpression suffix(R"(^(.*)\.0{2,}1$)");
    const auto match = suffix.match(first.fileName());
    if (!match.hasMatch() || match.captured(1).isEmpty()) {
        *error = QStringLiteral("Выберите первую часть с именем .001.");
        return {};
    }
    const QString base = match.captured(1);
    const int width = first.fileName().size() - base.size() - 1;
    CVolSeqName sequence;
    if (!sequence.ParseName(first.fileName())) {
        *error = QStringLiteral("Выберите первую часть с именем .001.");
        return {};
    }
    QStringList parts;
    QString name = first.fileName();
    for (int number = 1; number <= 10000; ++number) {
        const QString path = first.dir().filePath(name);
        const QFileInfo info(path);
        if (!info.exists())
            break;
        if (!info.isFile() || info.isSymLink() || info.size() == 0) {
            *error = QStringLiteral("Часть недоступна или пуста: %1").arg(path);
            return {};
        }
        parts << path;
        name = sequence.GetNextName();
    }
    if (parts.size() < 2) {
        *error = QStringLiteral("Нужны как минимум две последовательные части.");
        return {};
    }
    const QRegularExpression other(
        QStringLiteral("^%1\\.(\\d{%2,})$").arg(QRegularExpression::escape(base)).arg(width));
    for (const QString &name : first.dir().entryList(QDir::Files | QDir::Hidden | QDir::System)) {
        const auto candidate = other.match(name);
        if (candidate.hasMatch() && candidate.captured(1).toInt() > parts.size()) {
            *error = QStringLiteral("В последовательности частей есть пропуск.");
            return {};
        }
    }
    return parts;
}

VolumeParts VolumeOperation::inspectParts(const QString &firstPart, QString *error) {
    VolumeParts result;
    result.paths = discoverParts(firstPart, error);
    for (const QString &path : result.paths) {
        const qint64 size = QFileInfo(path).size();
        if (size <= 0 || size > std::numeric_limits<qint64>::max() - result.totalSize) {
            *error = QStringLiteral("Размер частей недоступен или слишком велик.");
            return {};
        }
        result.totalSize += size;
    }
    return result;
}

bool VolumeOperation::copyBytes(QFile &input, QFile &output, qint64 count, qint64 *progress,
                                QString *error) {
    while (count > 0) {
        if (canceled_) {
            *error = QStringLiteral("Операция отменена.");
            return false;
        }
        const QByteArray data = input.read(qMin(count, qint64(1 << 20)));
        if (data.isEmpty() || output.write(data) != data.size()) {
            *error = QStringLiteral("Ошибка чтения или записи файла.");
            return false;
        }
        count -= data.size();
        *progress += data.size();
        emit advanced(*progress);
    }
    return true;
}

bool VolumeOperation::split(QString *error) {
    const QFileInfo sourceInfo(source_);
    const QFileInfo destinationInfo(destination_);
    std::vector<UInt64> sizes;
    for (qint64 size : volumeSizes_) {
        if (size <= 0) {
            *error = QStringLiteral("Размер тома должен быть положительным.");
            return false;
        }
        sizes.push_back(UInt64(size));
    }
    if (!sourceInfo.isFile() || sourceInfo.isSymLink() || !destinationInfo.isDir() ||
        destinationInfo.isSymLink() || sizes.empty() ||
        sourceInfo.size() <= qint64(sizes.front())) {
        *error = QStringLiteral("Файл или размер тома недоступен для разделения.");
        return false;
    }
    const qint64 total = sourceInfo.size();
    const qint64 count = qint64(GetNumberOfVolumes(UInt64(total), sizes));
    if (count > 10000) {
        *error = QStringLiteral("Слишком много частей (более 10000).");
        return false;
    }
    CVolSeqName sequence;
    sequence.UnchangedPart = sourceInfo.fileName() + '.';
    sequence.SetNumDigits(count);
    QStringList targets;
    for (qint64 n = 1; n <= count; ++n) {
        const QString target = QDir(destination_).filePath(sequence.GetNextName());
        if (QFileInfo::exists(target) || QFileInfo(target).isSymLink()) {
            *error = QStringLiteral("Файл уже существует: %1").arg(target);
            return false;
        }
        targets << target;
    }
    QFile input(source_);
    if (!input.open(QIODevice::ReadOnly)) {
        *error = input.errorString();
        return false;
    }
    QStringList staged;
    QStringList published;
    qint64 progress = 0;
    bool ok = true;
    for (qsizetype i = 0; i < targets.size() && ok; ++i) {
        const QString stage =
            targets[i] + ".sevenzip-" + QUuid::createUuid().toString(QUuid::Id128);
        staged << stage;
        QFile output(stage);
        if (!output.open(QIODevice::WriteOnly | QIODevice::NewOnly) ||
            !copyBytes(input, output,
                       qMin(volumeSizes_[qMin(i, volumeSizes_.size() - 1)], total - progress),
                       &progress, error) ||
            !output.flush()) {
            if (error->isEmpty())
                *error = QStringLiteral("Не удалось записать часть файла.");
            ok = false;
        }
        output.close();
    }
    if (ok && input.size() != total) {
        *error = QStringLiteral("Исходный файл изменился во время разделения.");
        ok = false;
    }
    if (ok && canceled_) {
        *error = QStringLiteral("Операция отменена.");
        ok = false;
    }
    for (qsizetype i = 0; i < targets.size() && ok; ++i) {
        if (QFileInfo::exists(targets[i]) || QFileInfo(targets[i]).isSymLink() ||
            !QFile::rename(staged[i], targets[i])) {
            *error = QStringLiteral("Не удалось сохранить часть: %1").arg(targets[i]);
            ok = false;
        } else {
            published << targets[i];
        }
    }
    if (!ok) {
        for (const QString &path : staged)
            QFile::remove(path);
        for (const QString &path : published)
            QFile::remove(path);
    }
    return ok;
}

bool VolumeOperation::combine(QString *error) {
    const QStringList parts = discoverParts(source_, error);
    if (parts.isEmpty())
        return false;
    const QFileInfo targetInfo(destination_);
    if (!QFileInfo(targetInfo.absolutePath()).isDir() || QFileInfo::exists(destination_) ||
        targetInfo.isSymLink()) {
        *error = QStringLiteral("Папка назначения недоступна или файл уже существует.");
        return false;
    }
    const QString stage = destination_ + ".sevenzip-" + QUuid::createUuid().toString(QUuid::Id128);
    CThreadCombine combiner;
    combiner.OutputPath = stage;
    combiner.InputDirPrefix = QFileInfo(source_).absolutePath() + '/';
    for (const QString &part : parts)
        combiner.Names << QFileInfo(part).fileName();
    combiner.Set_NumBytesCur = [this](qint64 pos) {
        emit advanced(pos);
        return !canceled_.load();
    };
    bool ok = combiner.ProcessVirt(error);
    if (ok && canceled_) {
        *error = QStringLiteral("Операция отменена.");
        ok = false;
    }
    if (ok && (QFileInfo::exists(destination_) || QFileInfo(destination_).isSymLink() ||
               !QFile::rename(stage, destination_))) {
        *error = QStringLiteral("Не удалось сохранить объединённый файл.");
        ok = false;
    }
    if (!ok)
        QFile::remove(stage);
    return ok;
}

void VolumeOperation::run() {
    QString error;
    const bool ok = mode_ == Mode::Split ? split(&error) : combine(&error);
    emit result(ok, ok ? (mode_ == Mode::Split ? QStringLiteral("Файл разделён.")
                                               : QStringLiteral("Файлы объединены."))
                       : error);
}
