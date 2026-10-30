#include "archive/resources/qtarchivetransferbackup.h"
#include "7zip/Common/FileStreams.h"
#include "7zip/Compress/CopyCoder.h"
#include "Common/StringConvert.h"
#include <QFile>
#include <QSaveFile>

namespace {
class CTransferSaveStream final : public ISequentialOutStream, public CMyUnknownImp {
    Z7_COM_UNKNOWN_IMP_1(ISequentialOutStream)
    QSaveFile &file_;

  public:
    explicit CTransferSaveStream(QSaveFile &file) : file_(file) {}
    Z7_COM7F_IMF(Write(const void *data, UInt32 size, UInt32 *processed)) override {
        if (processed)
            *processed = 0;
        const qint64 written = file_.write(static_cast<const char *>(data), size);
        if (written < 0)
            return E_FAIL;
        if (processed)
            *processed = UInt32(written);
        return S_OK;
    }
};
} // namespace

QtArchiveTransferBackup::QtArchiveTransferBackup(QString destination)
    : destination_(std::move(destination)) {}

std::expected<std::unique_ptr<QtArchiveTransferBackup>, ArchiveTransferBackupError>
QtArchiveTransferBackup::Create(const QString &destination) {
    auto backup =
        std::unique_ptr<QtArchiveTransferBackup>(new QtArchiveTransferBackup(destination));
    if (!backup->directory_.isValid())
        return std::unexpected(ArchiveTransferBackupError::TemporaryStorageUnavailable);
    if (!QFile::copy(destination, backup->directory_.filePath(QStringLiteral("archive-backup"))))
        return std::unexpected(ArchiveTransferBackupError::CopyFailed);
    return backup;
}

bool QtArchiveTransferBackup::restore(const std::function<bool()> &closeArchive) const {
    // The original Close releases CArc's physical input as well as the native
    // handler. Keep its proxy/path state for the caller's existing ReOpen.
    if (closeArchive && !closeArchive())
        return false;
    auto *inputFile = new CInFileStream;
    CMyComPtr<ISequentialInStream> input = inputFile;
    const QString backup = directory_.filePath(QStringLiteral("archive-backup"));
    if (!inputFile->Open(us2fs(backup.toStdWString().c_str())))
        return false;
    QSaveFile restore(destination_);
    if (!restore.open(QIODevice::WriteOnly))
        return false;
    CMyComPtr<ISequentialOutStream> output = new CTransferSaveStream(restore);
    if (NCompress::CopyStream(input, output, nullptr) != S_OK) {
        restore.cancelWriting();
        return false;
    }
    return restore.commit();
}
