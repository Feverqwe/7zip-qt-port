#include "qtarchivevolumeoutput.h"
#include "7zip/Common/FileStreams.h"
#include "7zip/Common/MultiOutStream.h"
#include "7zip/Compress/CopyCoder.h"
#include "CPP/7zip/UI/Common/Update.h"
#include "Common/StringConvert.h"
#include "Windows/FileFind.h"
#include "qtlanguageservices.h"
#include "qtownedtempdir.h"
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QScopeGuard>

namespace {
class CVolumeProgress final : public ICompressProgressInfo, public CMyUnknownImp {
    Z7_COM_UNKNOWN_IMP_1(ICompressProgressInfo)
    QtArchiveVolumeOutput &_owner;
    quint64 _size;
    HRESULT STDMETHODCALLTYPE SetRatioInfo(const UInt64 *input, const UInt64 *) throw() override {
        RINOK(_owner.checkpoint())
        if (input && _owner.Progress)
            _owner.Progress(*input, _size);
        return S_OK;
    }

  public:
    CVolumeProgress(QtArchiveVolumeOutput &owner, quint64 size) : _owner(owner), _size(size) {}
};
} // namespace

QtArchiveVolumeOutput::QtArchiveVolumeOutput(QString source, QString target,
                                             CRecordVector<UInt64> sizes, bool setArcMTime,
                                             QString sfxModule, QString sfxTarget)
    : source_(std::move(source)), target_(std::move(target)), sfxModule_(std::move(sfxModule)),
      sfxTarget_(std::move(sfxTarget)), sizes_(std::move(sizes)), setArcMTime_(setArcMTime) {}

void QtArchiveVolumeOutput::cancel() { progress_.cancel(); }
void QtArchiveVolumeOutput::setPaused(bool paused) { progress_.setPaused(paused); }
HRESULT QtArchiveVolumeOutput::checkpoint() { return progress_.checkpoint(); }

HRESULT QtArchiveVolumeOutput::createVolumes() {
    RINOK(checkpoint())
    const QFileInfo target(target_);
    QtOwnedTempDir stage(target.absolutePath() + "/.sevenzip-volumes-XXXXXX");
    if (!stage.isValid())
        return E_FAIL;
    const QString prefix = stage.filePath(target.fileName() + '.');
    CMyComPtr2_Create<IInStream, CInFileStream> input;
    if (!input->Open(us2fs(source_.toStdWString().c_str())))
        return GetLastError_noZero_HRESULT();
    UInt64 size = 0;
    RINOK(input->GetSize(&size))
    CMyComPtr2_Create<IOutStream, CMultiOutStream> output;
    output->Prefix = us2fs(prefix.toStdWString().c_str());
    output->Init(sizes_);
    // Update.cpp assigns MTime before UpdateItems/FinalFlush. The CLI has
    // already calculated the source latest time on this completed archive.
    if (setArcMTime_) {
        NWindows::NFile::NFind::CFileInfo info;
        if (!info.Find(us2fs(source_.toStdWString().c_str())))
            return GetLastError_noZero_HRESULT();
        output->MTime = info.MTime;
        output->MTime_Defined = true;
    }
    CMyComPtr<ICompressProgressInfo> progress = new CVolumeProgress(*this, size);
    RINOK(NCompress::CopyStream(input, output, progress))
    unsigned count = 0;
    RINOK(output->FinalFlush_and_CloseFiles(count))
    output->NeedDelete = false;
    const QString stagedSfx = stage.filePath("sfx-module");
    if (!sfxModule_.isEmpty()) {
        CUpdateErrorInfo error;
        RINOK(WriteSfxModule(us2fs(sfxModule_.toStdWString().c_str()),
                             us2fs(stagedSfx.toStdWString().c_str()), error))
#ifndef Q_OS_WIN
        if (!QFile::setPermissions(stagedSfx, QFile::permissions(stagedSfx) | QFile::ExeOwner |
                                                  QFile::ExeUser))
            return E_FAIL;
#endif
        if (QFileInfo::exists(sfxTarget_) || QFileInfo(sfxTarget_).isSymLink())
            return E_FAIL;
    }
    QStringList destinations;
    for (unsigned i = 1; i <= count; ++i) {
        RINOK(checkpoint())
        const QString suffix = QString::number(i).rightJustified(3, '0');
        const QString destination = target_ + '.' + suffix;
        if (QFileInfo::exists(destination) || QFileInfo(destination).isSymLink()) {
            Error =
                QCoreApplication::translate("qtarchivevolumeoutput", "Volume already exists: %1")
                    .arg(destination);
            return E_FAIL;
        }
        destinations.append(destination);
    }
    if (!sfxModule_.isEmpty())
        destinations.append(sfxTarget_);
    qsizetype publishedCount = 0;
    auto rollback = qScopeGuard([&] {
        for (qsizetype i = 0; i < publishedCount; ++i)
            QFile::remove(destinations[i]);
    });
    for (unsigned i = 1; i <= count; ++i) {
        RINOK(checkpoint())
        const QString suffix = QString::number(i).rightJustified(3, '0');
        const QString &destination = destinations[i - 1];
        if (!QFile::rename(prefix + suffix, destination)) {
            Error =
                QCoreApplication::translate("qtarchivevolumeoutput", "Could not save volume: %1")
                    .arg(destination);
            return E_FAIL;
        }
        ++publishedCount;
    }
    RINOK(checkpoint())
    if (!sfxModule_.isEmpty()) {
        if (!QFile::rename(stagedSfx, sfxTarget_))
            return E_FAIL;
        ++publishedCount;
    }
    RINOK(checkpoint())
    PublishedPaths.swap(destinations);
    rollback.dismiss();
    return S_OK;
}

void QtArchiveVolumeOutput::run() {
    try {
        Result = createVolumes();
    } catch (...) {
        Result = E_OUTOFMEMORY;
    }
    if (Result != S_OK) {
        for (const QString &path : PublishedPaths)
            QFile::remove(path);
        PublishedPaths.clear();
        if (Error.isEmpty())
            Error =
                Result == E_ABORT
                    ? QCoreApplication::translate("qtarchivevolumeoutput", "Operation canceled.")
                    : QCoreApplication::translate("qtarchivevolumeoutput",
                                                  "Could not create archive volumes.");
    }
}
