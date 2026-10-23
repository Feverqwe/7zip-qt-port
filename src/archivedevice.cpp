#include "archivedevice.h"
#include "7zip/Common/LimitedStreams.h"
#include "7zip/PropID.h"
#include "Common/ComTry.h"
#include "Windows/PropVariant.h"
#include "archivecodeccatalog.h"
#include "archivenativeopen.h"
#include "deviceaccess.h"
#include <QCoreApplication>
#include <QSet>
#include <limits>

namespace {
HRESULT Limit(IInStream *input, UInt64 offset, UInt64 size, IInStream **result) {
    CMyComPtr<ISequentialInStream> limited;
    RINOK(CreateLimitedInStream(input, offset, size, &limited))
    return limited->QueryInterface(IID_IInStream, reinterpret_cast<void **>(result));
}

HRESULT Open(const std::shared_ptr<const ArchiveCodecCatalog> &catalog, IInStream *input,
             const QString &name, const QString &format, IOpenCallbackUI *callback,
             std::shared_ptr<ArchiveNativeOpen> &reader, const COpenOptions *requested = nullptr) {
    reader = std::make_shared<ArchiveNativeOpen>(catalog);
    COpenOptions options;
    if (requested)
        options = *requested;
    else
        options.props = nullptr;
    options.stream = input;
    options.filePath = name.toStdWString().c_str();
    CObjectVector<COpenType> types;
    CIntVector excluded;
    if (!catalog->parseOpenTypes(format, types))
        return E_INVALIDARG;
    // Never descend automatically into a partition: the user selects one,
    // including on disks with several filesystems or hidden service partitions.
    FOR_VECTOR(i, types) {
        types[i].Recursive = false;
        types[i].MaxStartOffset_Defined = true;
        types[i].MaxStartOffset = 0;
    }
    options.types = &types;
    options.excludedFormats = &excluded;
    return reader->Open(name, format, callback, &options);
}

HRESULT Number(IInArchive *archive, UInt32 index, PROPID id, UInt64 &number) {
    NWindows::NCOM::CPropVariant value;
    RINOK(archive->GetProperty(index, id, &value))
    if (value.vt != VT_UI8)
        return E_FAIL;
    number = value.uhVal.QuadPart;
    return S_OK;
}

QString Text(IInArchive *archive, UInt32 index, PROPID id) {
    NWindows::NCOM::CPropVariant value;
    if (archive->GetProperty(index, id, &value) != S_OK || value.vt != VT_BSTR)
        return {};
    return QString::fromWCharArray(value.bstrVal);
}

HRESULT CheckFeatures(IInArchive *archive, bool ext, bool allowIncompleteRead, QString *error) {
    // The reader has already opened the filesystem. Incompatible features still
    // require decoding support; other readonly-compatible declarations can be
    // exposed with a per-opening choice. Disk parsing stays with the handler.
    const QSet<QString> incompatible{"FILETYPE", "EXTENTS",     "64BIT",    "FLEX_BG",
                                     "META_BG",  "MMP",         "EA_INODE", "CSUM_SEED",
                                     "LARGEDIR", "INLINE_DATA", "CASEFOLD", "ENCRYPT"};
    const QSet<QString> readOnly{
        "SPARSE_SUPER",  "LARGE_FILE", "HUGE_FILE", "GDT_CSUM", "DIR_NLINK", "EXTRA_ISIZE", "QUOTA",
        "METADATA_CSUM", "READONLY",   "BTREE_DIR", "BIGALLOC", "PROJECT",   "VERITY"};
    UInt32 count = 0;
    RINOK(archive->GetNumberOfArchiveProperties(&count))
    bool checkedIncompatible = false, checkedReadOnly = false;
    QStringList unsupported, limitations;
    QString readerLimitations;
    for (UInt32 i = 0; i < count; ++i) {
        CMyComBSTR name;
        PROPID id;
        VARTYPE type;
        RINOK(archive->GetArchivePropertyInfo(i, &name, &id, &type))
        const QString label = name ? QString::fromWCharArray(name) : QString();
        if (label == QStringLiteral("Read-only limitations")) {
            NWindows::NCOM::CPropVariant value;
            RINOK(archive->GetArchiveProperty(id, &value))
            if (value.vt == VT_BSTR)
                readerLimitations = QString::fromWCharArray(value.bstrVal);
            else if (value.vt != VT_EMPTY)
                return E_NOTIMPL;
            continue;
        }
        const QSet<QString> *allowed = nullptr;
        if (ext && label == QStringLiteral("Incompatible Features")) {
            allowed = &incompatible;
            checkedIncompatible = true;
        } else if (ext && label == QStringLiteral("Readonly-compatible Features")) {
            allowed = &readOnly;
            checkedReadOnly = true;
        }
        if (!allowed)
            continue;
        NWindows::NCOM::CPropVariant value;
        RINOK(archive->GetArchiveProperty(id, &value))
        if (value.vt == VT_EMPTY)
            continue;
        if (value.vt != VT_BSTR) {
            if (error)
                *error = QCoreApplication::translate(
                    "archivedevice",
                    "Could not verify the filesystem features for direct reading.");
            return E_NOTIMPL;
        }
        for (const auto &feature :
             QString::fromWCharArray(value.bstrVal).split(' ', Qt::SkipEmptyParts)) {
            if (allowed->contains(feature))
                continue;
            if (allowed == &readOnly || feature == QStringLiteral("RECOVER"))
                limitations.append(feature);
            else
                unsupported.append(feature);
        }
    }
    if (ext && (!checkedIncompatible || !checkedReadOnly)) {
        if (error)
            *error = QCoreApplication::translate(
                "archivedevice", "Could not verify the filesystem features for direct reading.");
        return E_NOTIMPL;
    }
    if (!unsupported.isEmpty()) {
        if (error)
            *error =
                QCoreApplication::translate("archivedevice", "Unsupported filesystem features: %1.")
                    .arg(unsupported.join(QStringLiteral(", ")));
        return E_NOTIMPL;
    }
    if ((limitations.isEmpty() && readerLimitations.isEmpty()) || allowIncompleteRead)
        return S_OK;
    if (error) {
        if (!limitations.isEmpty())
            *error = QCoreApplication::translate("archivedevice", "Limited read-only access: %1.")
                         .arg(limitations.join(QStringLiteral(", ")));
        if (!readerLimitations.isEmpty()) {
            if (!error->isEmpty())
                *error += '\n';
            *error += readerLimitations;
        }
    }
    return kDeviceReadLimitationsRequired;
}
} // namespace

HRESULT OpenDeviceFilesystem(const std::shared_ptr<const ArchiveCodecCatalog> &catalog,
                             IInStream *input, const QString &name, IOpenCallbackUI *callback,
                             std::shared_ptr<ArchiveNativeOpen> &reader, QString *error,
                             const COpenOptions *options) {
    COM_TRY_BEGIN
    reader.reset();
    if (error)
        error->clear();
    if (!input || !catalog || catalog->result() != S_OK)
        return E_INVALIDARG;
    // Only filesystem readers are candidates. General archive autodetection
    // could accept a boot payload or descend into a different partition.
    // UDF precedes ISO for media that publish both directory trees.
    for (const QString &format :
         {QStringLiteral("APFS"), QStringLiteral("Ext"), QStringLiteral("NTFS"),
          QStringLiteral("exFAT"), QStringLiteral("HFS"), QStringLiteral("EROFS"),
          QStringLiteral("SquashFS"), QStringLiteral("CramFS"), QStringLiteral("Udf"),
          QStringLiteral("Iso"), QStringLiteral("FAT")}) {
        if (catalog->formatIndex(format) < 0)
            continue;
        if (callback)
            RINOK(callback->Open_CheckBreak())
        RINOK(input->Seek(0, STREAM_SEEK_SET, nullptr))
        std::shared_ptr<ArchiveNativeOpen> opened;
        const HRESULT result = Open(catalog, input, name, format, callback, opened, options);
        if (result == S_FALSE)
            continue;
        RINOK(result)
        const auto *direct = dynamic_cast<DirectDeviceInput *>(input);
        RINOK(CheckFeatures(opened->arc().Archive, format == QStringLiteral("Ext"),
                            direct && direct->allowsIncompleteDeviceRead(), error))
        reader = std::move(opened);
        return S_OK;
    }
    return S_FALSE;
    COM_TRY_END
}

ArchiveDevice::ArchiveDevice(std::shared_ptr<const ArchiveCodecCatalog> catalog)
    : catalog_(std::move(catalog)) {}

HRESULT ArchiveDevice::inspect(IInStream *input, UInt64 size, const QString &name,
                               IOpenCallbackUI *callback) {
    COM_TRY_BEGIN
    input_.Release();
    partitions_.clear();
    name_.clear();
    if (!input || !size || size > UInt64(std::numeric_limits<Int64>::max()) || !catalog_ ||
        catalog_->result() != S_OK)
        return E_INVALIDARG;
    CMyComPtr<IInStream> bounded;
    if (dynamic_cast<DirectDeviceInput *>(input)) {
        RINOK(CreateDevicePartitionInput(input, 0, size, &bounded))
    } else {
        RINOK(Limit(input, 0, size, &bounded))
    }
    QList<ArchiveDevicePartition> partitions;
    for (const QString &format :
         {QStringLiteral("GPT"), QStringLiteral("MBR"), QStringLiteral("APM")}) {
        if (catalog_->formatIndex(format) < 0)
            continue;
        if (callback)
            RINOK(callback->Open_CheckBreak())
        RINOK(bounded->Seek(0, STREAM_SEEK_SET, nullptr))
        std::shared_ptr<ArchiveNativeOpen> layout;
        const HRESULT result = Open(catalog_, bounded, name, format, callback, layout);
        if (result == S_FALSE)
            continue;
        RINOK(result)
        IInArchive *archive = layout->arc().Archive;
        UInt32 count;
        RINOK(archive->GetNumberOfItems(&count))
        for (UInt32 index = 0; index < count; ++index) {
            if (callback)
                RINOK(callback->Open_CheckBreak())
            ArchiveDevicePartition partition;
            RINOK(Number(archive, index, kpidOffset, partition.offset))
            RINOK(Number(archive, index, kpidSize, partition.size))
            // APM can retain zero-sized Apple_Void entries between real volumes.
            if (!partition.size)
                continue;
            if (partition.offset > size || partition.size > size - partition.offset)
                return E_FAIL;
            partition.name = Text(archive, index, kpidPath);
            partition.type = Text(archive, index, kpidFileSystem);
            // MbrHandler also exposes unallocated tail bytes as an archive
            // item. Only real partitions have its FileSystem property.
            if (format == QStringLiteral("MBR") && partition.type.isEmpty())
                continue;
            partitions.append(partition);
        }
        break;
    }
    // A device can contain a filesystem directly, without a partition table.
    // The selected filesystem reader is the authority; no local superblock parser.
    if (partitions.isEmpty())
        partitions.append({name, QString(), 0, size});
    name_ = name;
    partitions_ = std::move(partitions);
    input_ = bounded;
    return S_OK;
    COM_TRY_END
}

HRESULT ArchiveDevice::openPartition(int index, IOpenCallbackUI *callback,
                                     std::shared_ptr<ArchiveNativeOpen> &reader, QString *error,
                                     const COpenOptions *options) const {
    COM_TRY_BEGIN
    reader.reset();
    if (error)
        error->clear();
    if (!input_ || index < 0 || index >= partitions_.size())
        return E_INVALIDARG;
    if (callback)
        RINOK(callback->Open_CheckBreak())
    CMyComPtr<IInStream> bounded;
    RINOK(partitionInput(index, &bounded))
    return OpenDeviceFilesystem(catalog_, bounded, name_ + '/' + partitions_[index].name, callback,
                                reader, error, options);
    COM_TRY_END
}

HRESULT ArchiveDevice::partitionInput(int index, IInStream **stream) const {
    COM_TRY_BEGIN
    if (!stream)
        return E_INVALIDARG;
    *stream = nullptr;
    if (!input_ || index < 0 || index >= partitions_.size())
        return E_INVALIDARG;
    const auto &partition = partitions_[index];
    if (dynamic_cast<DirectDeviceInput *>(input_.Interface())) {
        return CreateDevicePartitionInput(input_, partition.offset, partition.size, stream);
    }
    return Limit(input_, partition.offset, partition.size, stream);
    COM_TRY_END
}
