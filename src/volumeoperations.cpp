#include "volumeoperations.h"
#include "7zip/UI/FileManager/resource.h"
#include "CPP/7zip/UI/FileManager/FormatUtils.h"
#include "CPP/7zip/UI/FileManager/LangUtils.h"
#include "CPP/7zip/UI/FileManager/PanelSplitFile.h"
#include "CPP/7zip/UI/FileManager/SplitUtils.h"
#include "qtlanguageservices.h"

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
    CRecordVector<UInt64> values;
    if (!ParseVolumeSizes(UString(text.trimmed().toStdWString().c_str()), values) ||
        values.IsEmpty())
        return {};
    QList<qint64> result;
    FOR_VECTOR(i, values) {
        const UInt64 value = values[i];
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
    CVolSeqName sequence;
    if (!sequence.ParseName(first.fileName())) {
        *error = QString::fromWCharArray(LangString(IDS_COMBINE_CANT_DETECT_SPLIT_FILE));
        return {};
    }
    const QString base = sequence.UnchangedPart;
    const qsizetype width = sequence.ChangedPart.size();
    QStringList parts;
    QString name = first.fileName();
    for (int number = 1; number <= 10000; ++number) {
        const QString path = first.dir().filePath(name);
        const QFileInfo info(path);
        if (!info.exists())
            break;
        if (!info.isFile() || info.isSymLink() || info.size() == 0) {
            *error = QtLocalText("Part is unavailable or empty: %1",
                                 QStringLiteral("Часть недоступна или пуста: %1"))
                         .arg(path);
            return {};
        }
        parts << path;
        name = sequence.GetNextName();
    }
    if (parts.size() < 2) {
        *error = QString::fromWCharArray(LangString(IDS_COMBINE_CANT_FIND_MORE_THAN_ONE_PART));
        return {};
    }
    const QRegularExpression other(
        QStringLiteral("^%1(\\d{%2,})$").arg(QRegularExpression::escape(base)).arg(width));
    for (const QString &name : first.dir().entryList(QDir::Files | QDir::Hidden | QDir::System)) {
        const auto candidate = other.match(name);
        if (candidate.hasMatch() && candidate.captured(1).toInt() > parts.size()) {
            *error = QtLocalText("A part is missing from the sequence.",
                                 QStringLiteral("В последовательности частей есть пропуск."));
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
            *error = QtLocalText("Part sizes are unavailable or too large.",
                                 QStringLiteral("Размер частей недоступен или слишком велик."));
            return {};
        }
        result.totalSize += size;
    }
    return result;
}

bool VolumeOperation::split(QString *error) {
    const QFileInfo sourceInfo(source_);
    const QFileInfo destinationInfo(destination_);
    CRecordVector<UInt64> sizes;
    for (qint64 size : volumeSizes_) {
        if (size <= 0) {
            *error = QtLocalText("Volume size must be positive.",
                                 QStringLiteral("Размер тома должен быть положительным."));
            return false;
        }
        sizes.Add(UInt64(size));
    }
    if (!sourceInfo.isFile() || sourceInfo.isSymLink() || !destinationInfo.isDir() ||
        destinationInfo.isSymLink() || sizes.IsEmpty() ||
        sourceInfo.size() <= qint64(sizes.FrontItem())) {
        *error = QtLocalText("File or volume size is unavailable for splitting.",
                             QStringLiteral("Файл или размер тома недоступен для разделения."));
        return false;
    }
    const qint64 total = sourceInfo.size();
    const qint64 count = qint64(GetNumberOfVolumes(UInt64(total), sizes));
    if (count > 10000) {
        *error = QtLocalText("Too many parts (more than 10000).",
                             QStringLiteral("Слишком много частей (более 10000)."));
        return false;
    }
    CVolSeqName sequence;
    sequence.UnchangedPart = sourceInfo.fileName() + '.';
    sequence.SetNumDigits(count);
    QStringList targets;
    for (qint64 n = 1; n <= count; ++n) {
        const QString target = QDir(destination_).filePath(sequence.GetNextName());
        if (QFileInfo::exists(target) || QFileInfo(target).isSymLink()) {
            *error = QString::fromWCharArray(
                MyFormatNew(IDS_FILE_EXIST, UString(target.toStdWString().c_str())));
            return false;
        }
        targets << target;
    }
    QStringList staged;
    for (const QString &target : targets)
        staged << target + ".sevenzip-" + QUuid::createUuid().toString(QUuid::Id128);
    QStringList published;
    CThreadSplit spliter;
    spliter.FilePath = source_;
    spliter.OutputPaths = staged;
    spliter.VolumeSizes = volumeSizes_;
    spliter.TotalSize = total;
    spliter.Set_NumBytesCur = [this](qint64 pos) {
        emit advanced(pos);
        return !canceled_.load();
    };
    bool ok = spliter.ProcessVirt(error);
    if (ok && canceled_) {
        *error = QtLocalText("Operation canceled.", QStringLiteral("Операция отменена."));
        ok = false;
    }
    for (qsizetype i = 0; i < targets.size() && ok; ++i) {
        if (QFileInfo::exists(targets[i]) || QFileInfo(targets[i]).isSymLink() ||
            !QFile::rename(staged[i], targets[i])) {
            *error = QtLocalText("Could not save part: %1",
                                 QStringLiteral("Не удалось сохранить часть: %1"))
                         .arg(targets[i]);
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
        *error =
            QtLocalText("Destination folder is unavailable or the file already exists.",
                        QStringLiteral("Папка назначения недоступна или файл уже существует."));
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
        *error = QtLocalText("Operation canceled.", QStringLiteral("Операция отменена."));
        ok = false;
    }
    if (ok && (QFileInfo::exists(destination_) || QFileInfo(destination_).isSymLink() ||
               !QFile::rename(stage, destination_))) {
        *error = QtLocalText("Could not save combined file.",
                             QStringLiteral("Не удалось сохранить объединённый файл."));
        ok = false;
    }
    if (!ok)
        QFile::remove(stage);
    return ok;
}

void VolumeOperation::run() {
    QString error;
    const bool ok = mode_ == Mode::Split ? split(&error) : combine(&error);
    emit result(ok, ok ? (mode_ == Mode::Split
                              ? QtLocalText("File split.", QStringLiteral("Файл разделён."))
                              : QtLocalText("Files combined.", QStringLiteral("Файлы объединены.")))
                       : error);
}
