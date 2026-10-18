#include "volumeoperations.h"
#include "7zip/UI/FileManager/resource.h"
#include "CPP/7zip/UI/FileManager/FormatUtils.h"
#include "CPP/7zip/UI/FileManager/LangUtils.h"
#include "CPP/7zip/UI/FileManager/PanelSplitFile.h"
#include "CPP/7zip/UI/FileManager/ProgressDialog2.h"
#include "CPP/7zip/UI/FileManager/SplitUtils.h"
#include "CPP/7zip/UI/FileManager/ViewSettings.h"
#include "qtdiagnostictext.h"
#include "qtlangcontrols.h"
#include "qtlanguageservices.h"
#include "qtvolumetransaction.h"
#include <QDir>
#include <QFileInfo>
#include <limits>

VolumeOperation::VolumeOperation(Mode mode, QString source, QString destination, qint64 volumeSize,
                                 QObject *parent)
    : VolumeOperation(mode, std::move(source), std::move(destination), QList<qint64>{volumeSize},
                      parent) {}
VolumeOperation::VolumeOperation(Mode mode, QString source, QString destination,
                                 QList<qint64> volumeSizes, QObject *parent)
    : QThread(parent), mode_(mode), source_(std::move(source)),
      destination_(std::move(destination)) {
    for (qint64 size : volumeSizes) {
        invalidSizes_ |= size <= 0;
        volumeSizes_.Add(UInt64(size));
    }
}
VolumeOperation::VolumeOperation(const CThreadSplit &plan, QObject *parent)
    : QThread(parent), mode_(Mode::Split), source_(ToQString(fs2us(plan.FilePath))),
      destination_(QFileInfo(ToQString(fs2us(plan.VolBasePath))).absolutePath()),
      volumeSizes_(plan.VolumeSizes), volBasePath_(plan.VolBasePath), numVolumes_(plan.NumVolumes),
      nativePlan_(true) {}
VolumeOperation::VolumeOperation(const CThreadCombine &plan, QObject *parent)
    : QThread(parent), mode_(Mode::Combine),
      source_(ToQString(fs2us(plan.InputDirPrefix + plan.Names.Front()))),
      destination_(ToQString(fs2us(plan.OutputPath))), inputDirPrefix_(plan.InputDirPrefix),
      partNames_(plan.Names), totalSize_(plan.TotalSize), nativePlan_(true) {}
void VolumeOperation::cancel() { canceled_ = true; }

QList<qint64> VolumeOperation::parseSizes(const QString &text) {
    UString s = ToUString(text);
    s.Trim();
    CRecordVector<UInt64> values;
    if (!ParseVolumeSizes(s, values) || values.IsEmpty())
        return {};
    QList<qint64> result;
    FOR_VECTOR(i, values) {
        if (values[i] == 0 || values[i] > UInt64(std::numeric_limits<qint64>::max()))
            return {};
        result.append(qint64(values[i]));
    }
    return result;
}
qint64 VolumeOperation::parseSize(const QString &text) {
    const auto sizes = parseSizes(text);
    return sizes.size() == 1 ? sizes.first() : 0;
}

VolumeParts VolumeOperation::inspectParts(const QString &firstPart, QString *error) {
    const QFileInfo first(firstPart);
    CVolSeqName sequence;
    if (!sequence.ParseName(ToUString(first.fileName()))) {
        *error = ToQString(LangString(IDS_COMBINE_CANT_DETECT_SPLIT_FILE));
        return {};
    }
    CThreadCombine combiner;
    const UString prefix = ToUString(first.absolutePath() + '/');
    combiner.LoadParts(prefix, ToUString(first.fileName()), sequence);
    if (combiner.Names.Size() == 1) {
        *error = ToQString(LangString(IDS_COMBINE_CANT_FIND_MORE_THAN_ONE_PART));
        return {};
    }
    if (combiner.TotalSize == 0) {
        *error = QCoreApplication::translate("volumeoperations", "No data");
        return {};
    }
    VolumeParts result;
    FOR_VECTOR(i, combiner.Names) {
        const QString path = ToQString(prefix + fs2us(combiner.Names[i]));
        if (QFileInfo(path).isSymLink()) {
            *error = QCoreApplication::translate("volumeoperations", "File part is unavailable: %1")
                         .arg(path);
            return {};
        }
        result.paths << path;
    }
    if (combiner.TotalSize > UInt64(std::numeric_limits<qint64>::max())) {
        *error = QCoreApplication::translate("volumeoperations",
                                             "Part sizes are unavailable or too large.");
        return {};
    }
    result.totalSize = qint64(combiner.TotalSize);
    return result;
}
QStringList VolumeOperation::discoverParts(const QString &firstPart, QString *error) {
    return inspectParts(firstPart, error).paths;
}

bool VolumeOperation::process(QtVolumeWorker &worker, QString *error) {
    if (progressSync_)
        worker.Sync.attach(*progressSync_);
    worker.Sync.advanced = [this, &worker](UInt64 pos) {
        emit advanced(qint64(pos));
        if (canceled_)
            worker.Sync.cancel();
    };
    if (canceled_)
        worker.Sync.cancel();
    HRESULT result;
    try {
        result = mode_ == Mode::Split ? static_cast<CThreadSplit &>(worker).ProcessVirt()
                                      : static_cast<CThreadCombine &>(worker).ProcessVirt();
    } catch (const char *message) {
        *error = QString::fromWCharArray(QtDiagnosticText(message));
        return false;
    } catch (...) {
        *error = QCoreApplication::translate("volumeoperations", "Volume operation failed");
        return false;
    }
    if (canceled_ || result == E_ABORT) {
        *error = QCoreApplication::translate("volumeoperations", "Operation canceled.");
        return false;
    }
    if (result != S_OK) {
        if (mode_ == Mode::Combine && !worker.ErrorPaths.IsEmpty())
            *error = QCoreApplication::translate("volumeoperations", "File part is unavailable: %1")
                         .arg(ToQString(worker.ErrorPaths.Back()));
        else {
            *error = ToQString(HResultToMessage(result));
            if (!worker.ErrorPaths.IsEmpty())
                *error += '\n' + ToQString(worker.ErrorPaths.Back());
        }
        return false;
    }
    return true;
}

bool VolumeOperation::split(QString *error) {
    const QFileInfo source(source_);
    CThreadSplit spliter;
    if (invalidSizes_) {
        *error = QCoreApplication::translate("volumeoperations", "Volume size must be positive.");
        return false;
    }
    spliter.VolumeSizes = volumeSizes_;
    if (!source.isFile() || source.isSymLink() || !QFileInfo(destination_).isDir() ||
        QFileInfo(destination_).isSymLink() || spliter.VolumeSizes.IsEmpty() ||
        UInt64(source.size()) <= spliter.VolumeSizes.FrontItem()) {
        *error = QCoreApplication::translate("volumeoperations",
                                             "File or volume size is unavailable for splitting.");
        return false;
    }
    spliter.FilePath = us2fs(ToUString(source_));
    spliter.VolBasePath = nativePlan_
                              ? volBasePath_
                              : us2fs(ToUString(QDir(destination_).filePath(source.fileName())));
    spliter.NumVolumes =
        nativePlan_ ? numVolumes_ : GetNumberOfVolumes(UInt64(source.size()), spliter.VolumeSizes);
    QtVolumeTransaction transaction;
    spliter.createOutput = [&transaction](auto &file, const FString &path) {
        return transaction.create(file, path);
    };
    if (!process(spliter, error))
        return false;
    const QFileInfo after(source_);
    if (!after.isFile() || after.isSymLink() || after.size() != source.size()) {
        *error =
            QCoreApplication::translate("volumeoperations", "Source file changed while splitting.");
        return false;
    }
    if (!transaction.publish()) {
        *error = QCoreApplication::translate("volumeoperations", "Could not save part: %1")
                     .arg(transaction.failedPath());
        return false;
    }
    return true;
}

bool VolumeOperation::combine(QString *error) {
    VolumeParts parts;
    if (nativePlan_) {
        FOR_VECTOR(i, partNames_)
        parts.paths << ToQString(fs2us(inputDirPrefix_ + partNames_[i]));
    } else
        parts = inspectParts(source_, error);
    if (parts.paths.isEmpty())
        return false;
    const QFileInfo target(destination_);
    if (!QFileInfo(target.absolutePath()).isDir() || QFileInfo::exists(destination_) ||
        target.isSymLink()) {
        *error = QCoreApplication::translate(
            "volumeoperations", "Destination folder is unavailable or the file already exists.");
        return false;
    }
    QList<qint64> sizes;
    CThreadCombine combiner;
    combiner.InputDirPrefix =
        nativePlan_ ? inputDirPrefix_ : us2fs(ToUString(QFileInfo(source_).absolutePath() + '/'));
    combiner.OutputPath = us2fs(ToUString(destination_));
    combiner.TotalSize = nativePlan_ ? totalSize_ : UInt64(parts.totalSize);
    for (const QString &path : parts.paths) {
        if (!QFileInfo(path).isFile() || QFileInfo(path).isSymLink()) {
            *error = QCoreApplication::translate("volumeoperations", "File part is unavailable: %1")
                         .arg(path);
            return false;
        }
        combiner.Names.Add(us2fs(ToUString(QFileInfo(path).fileName())));
        sizes << QFileInfo(path).size();
    }
    QtVolumeTransaction transaction;
    combiner.createOutput = [&transaction](auto &file, const FString &path) {
        return transaction.create(file, path);
    };
    if (!process(combiner, error))
        return false;
    for (qsizetype i = 0; i < parts.paths.size(); ++i) {
        const QFileInfo after(parts.paths[i]);
        if (!after.isFile() || after.isSymLink() || after.size() != sizes[i]) {
            *error = QCoreApplication::translate("volumeoperations", "File part changed: %1")
                         .arg(parts.paths[i]);
            return false;
        }
    }
    if (!transaction.publish()) {
        *error = QCoreApplication::translate("volumeoperations", "Could not save combined file.");
        return false;
    }
    return true;
}

void VolumeOperation::run() {
    QString error;
    const bool ok = mode_ == Mode::Split ? split(&error) : combine(&error);
    emit result(ok, ok ? (mode_ == Mode::Split
                              ? QCoreApplication::translate("volumeoperations", "File split.")
                              : QCoreApplication::translate("volumeoperations", "Files combined."))
                       : error);
}
