#include "archivereadonlyview.h"
#include "7zip/Common/FileStreams.h"
#include "Common/ComTry.h"
#include "Common/StringConvert.h"
#include "Windows/PropVariant.h"
#include "Windows/PropVariantConv.h"
#include "Windows/TimeUtils.h"
#include "archivebackend.h"
#include "archiveinputlayer.h"
#include "archivenativeopen.h"
#include "qtarchivefileresource.h"
#include "qtarchiveitemstream.h"
#include "qtarchivereadoperation.h"
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QHash>
#include <QMutexLocker>
#include <QTemporaryDir>
#include <QThread>
#include <algorithm>
#include <atomic>
#include <limits>
#include <new>

namespace {
class ReadRequest final : public IProgress, public CMyUnknownImp {
    Z7_COM_UNKNOWN_IMP_1(IProgress)
    const std::atomic_bool &cancelled_;

  public:
    explicit ReadRequest(const std::atomic_bool &cancelled) : cancelled_(cancelled) {}
    Z7_COM7F_IMF(SetTotal(UInt64)) override { return cancelled_ ? E_ABORT : S_OK; }
    Z7_COM7F_IMF(SetCompleted(const UInt64 *)) override { return cancelled_ ? E_ABORT : S_OK; }
};
QString Key(const QString &path) {
    return path.normalized(QString::NormalizationForm_C).toCaseFolded();
}
bool SafeComponent(const QString &name) {
    return !name.isEmpty() && name != QStringLiteral(".") && name != QStringLiteral("..") &&
           name.size() <= 255 && !name.contains(QChar::Null) && !name.contains(u':') &&
           !name.endsWith(u'.') && !name.endsWith(u' ') && QString::fromUtf8(name.toUtf8()) == name;
}
} // namespace

struct ArchiveReadOnlyView::State {
    struct Item {
        UInt32 Index = UInt32(-1);
        quint64 Used = 0;
        quint64 Bytes = 0;
        QString Path;
        std::shared_ptr<ArchiveInputLayer> Stream;
        Int32 Operation = -1;
        bool Encrypted = false, Unverified = false;
    };
    CMyComPtr<IUnknown> Resource;
    std::shared_ptr<ArchiveNativeOpen> Reader;
    bool NativeMetadata = false, Prepared = false;
    quint64 Depth = 0;
    QString Limitations;
    QList<Entry> Entries;
    QHash<quint64, Item> Items;
    QHash<QString, quint64> Paths;
    quint64 Quota, Cached = 0, Clock = 0;
    unsigned Active = 0;
    QString Password;
    std::function<bool(const QString &, quint64)> ExtractionConsent;
    std::function<bool(const QString &, quint64, Int32)> RecoveryConsent;
    std::unique_ptr<QTemporaryDir> Directory;
    std::atomic_bool Cancelled{false};
    QMutex OperationMutex;
    QtArchiveReadOperation *Operation = nullptr;

    State(IUnknown *resource, quint64 quota, QString password,
          std::function<bool(const QString &, quint64)> extractionConsent,
          std::function<bool(const QString &, quint64, Int32)> recoveryConsent)
        : Resource(resource), Quota(quota), Password(std::move(password)),
          ExtractionConsent(std::move(extractionConsent)),
          RecoveryConsent(std::move(recoveryConsent)) {
        if (const auto *owner = dynamic_cast<QtArchiveFileResource *>(resource)) {
            Reader = owner->NativeReaderLease();
            NativeMetadata = owner->NativeItemProperties() != nullptr;
            Depth = owner->ContainerDepth();
            Limitations = owner->FilesystemReadLimitations();
            if (Limitations.isEmpty() && owner->AllowsIncompleteDeviceRead())
                Limitations = QCoreApplication::translate(
                    "ArchiveReadOnlyView",
                    "Limited access was allowed for this device. Files may be "
                    "missing, outdated or unreadable.");
        }
    }
    bool evict(quint64 except, quint64 needed) {
        while (Cached > Quota - needed || Active >= 64) {
            auto oldest = Items.end();
            for (auto i = Items.begin(); i != Items.end(); ++i)
                if (i.key() != except && i->Stream &&
                    (oldest == Items.end() || i->Used < oldest->Used))
                    oldest = i;
            if (oldest == Items.end())
                return false;
            oldest->Stream.reset();
            --Active;
            if (!oldest->Path.isEmpty()) {
                if (!QFile::remove(oldest->Path))
                    return false;
                oldest->Path.clear();
            }
            Cached -= oldest->Bytes;
            oldest->Bytes = 0;
            // Keep item identity, but count only retained streams below.
        }
        return true;
    }
    HRESULT stream(quint64 id, ReadResult &result) {
        auto &item = Items[id];
        item.Used = ++Clock;
        if (item.Stream) {
            result.Operation = item.Operation;
            if (item.Unverified)
                result.Error = QStringLiteral("Unverified or incomplete extracted data.");
            return S_OK;
        }
        CMyComPtr<IProgress> request = new ReadRequest(Cancelled);
        CMyComPtr<ISequentialInStream> sequential;
        const HRESULT streamed =
            CreateArchiveItemStream(Resource, Reader, Depth, item.Index, &sequential, request);
        if (streamed == E_ABORT)
            return E_ABORT;
        CMyComPtr<IInStream> seekable;
        if (streamed == S_OK && sequential)
            sequential.QueryInterface(IID_IInStream, &seekable);
        if (seekable) {
            if (!evict(id, 0))
                return E_OUTOFMEMORY;
            const HRESULT status = ArchiveInputLayer::Create(seekable, item.Stream, request);
            if (status == S_OK)
                ++Active;
            return status;
        }

        const quint64 size = Entries.at(qsizetype(id - 1)).Size;
        if (size > Quota) {
            result.Error =
                QStringLiteral("This entry requires decoding beyond the mount cache limit.");
            return E_OUTOFMEMORY;
        }
        QString name;
        for (quint64 current = id; current > 1;) {
            const auto &entry = Entries.at(qsizetype(current - 1));
            name = name.isEmpty() ? entry.Name : entry.Name + u'/' + name;
            current = entry.Parent;
        }
        // No temporary output or decoding starts without explicit consent.
        if (!ExtractionConsent || !ExtractionConsent(name, size)) {
            result.Error =
                QStringLiteral("Extracting this entry into the mount cache requires confirmation.");
            return E_ABORT;
        }
        if (Cancelled)
            return E_ABORT;
        if (!evict(id, size)) {
            result.Error = QStringLiteral("Could not release space in the mount cache.");
            return E_OUTOFMEMORY;
        }
        if (!Directory)
            Directory = std::make_unique<QTemporaryDir>();
        if (!Directory->isValid()) {
            result.Error = Directory->errorString();
            return E_FAIL;
        }
        const QString path = Directory->filePath(QString::number(id));
        QtArchiveReadOperation operation(Reader, Password);
        operation.Output.NativeIndex = item.Index;
        operation.Output.Path = path;
        operation.Output.Limit = size;
        {
            QMutexLocker lock(&OperationMutex);
            Operation = &operation;
            if (Cancelled)
                operation.cancel();
        }
        operation.start();
        operation.wait();
        {
            QMutexLocker lock(&OperationMutex);
            Operation = nullptr;
        }
        result.Operation = operation.OutputOperation;
        const qint64 actual = QFileInfo(path).size();
        const bool complete = operation.Result == S_OK && operation.OutputClosed && actual >= 0 &&
                              quint64(actual) == size;
        using namespace NArchive::NExtract::NOperationResult;
        const bool recoverable =
            !item.Encrypted && operation.Result != E_ABORT && operation.OutputClosed &&
            actual > 0 && quint64(actual) <= size &&
            (operation.OutputOperation == kDataError || operation.OutputOperation == kCRCError ||
             operation.OutputOperation == kUnexpectedEnd ||
             operation.OutputOperation == kDataAfterEnd);
        const bool recovered = !complete && recoverable && RecoveryConsent &&
                               RecoveryConsent(name, quint64(actual), operation.OutputOperation);
        if ((!complete && !recovered) || Cancelled) {
            QFile::remove(path);
            result.Error =
                QStringLiteral("The entry could not be decoded completely and verified.");
            return Cancelled ? E_ABORT : operation.Result == S_OK ? E_FAIL : operation.Result;
        }
        CMyComPtr2<IInStream, CInFileStream> file;
        file.Create_if_Empty();
        if (!file->Open(us2fs(path.toStdWString().c_str()))) {
            QFile::remove(path);
            return E_FAIL;
        }
        const HRESULT status = ArchiveInputLayer::Create(file.Interface(), item.Stream, request);
        if (status != S_OK) {
            file.Detach()->Release();
            QFile::remove(path);
            return status;
        }
        item.Path = path;
        item.Bytes = quint64(actual);
        item.Operation = operation.OutputOperation;
        item.Unverified = recovered;
        if (recovered)
            result.Error = QStringLiteral("Unverified or incomplete extracted data.");
        Cached += item.Bytes;
        ++Active;
        return S_OK;
    }
};

ArchiveReadOnlyView::ArchiveReadOnlyView(
    IUnknown *resource, quint64 quota, QString password,
    std::function<bool(const QString &, quint64)> extractionConsent,
    std::function<bool(const QString &, quint64, Int32)> recoveryConsent)
    : state_(std::make_unique<State>(resource, quota, std::move(password),
                                     std::move(extractionConsent), std::move(recoveryConsent))) {}
ArchiveReadOnlyView::~ArchiveReadOnlyView() {
    // Close cached file handles before QTemporaryDir removes its files.
    state_->Items.clear();
}
const QList<ArchiveReadOnlyView::Entry> &ArchiveReadOnlyView::entries() const {
    return state_->Entries;
}
QString ArchiveReadOnlyView::limitations() const { return state_->Limitations; }
void ArchiveReadOnlyView::cancel() {
    state_->Cancelled = true;
    QMutexLocker lock(&state_->OperationMutex);
    if (state_->Operation)
        state_->Operation->cancel();
}

HRESULT ArchiveReadOnlyView::initialize(QString &error) {
    auto &state = *state_;
    if (state.Prepared)
        return E_FAIL;
    state.Prepared = true;
    if (!state.Reader || !state.NativeMetadata) {
        error = QStringLiteral("This container has no retained native reader.");
        return E_NOINTERFACE;
    }
    std::lock_guard<std::mutex> lock(state.Reader->itemStreamMutex());
    CMyComPtr<IProgress> request = new ReadRequest(state.Cancelled);
    ArchiveInputReadRequestScope scope(state.Reader->inputStream(), request);
    state.Entries.append({1, 0, {}, 0, true});
    state.Paths.insert({}, 1);
    UInt32 count = 0;
    RINOK(state.Reader->arc().Archive->GetNumberOfItems(&count))
    for (UInt32 index = 0; index < count; ++index) {
        if (state.Cancelled || QThread::currentThread()->isInterruptionRequested())
            return E_ABORT;
        CReadArcItem native;
        RINOK(state.Reader->arc().GetItem(index, native))
        NWindows::NCOM::CPropVariant time;
        const bool hasTime =
            state.Reader->arc().Archive->GetProperty(index, kpidMTime, &time) == S_OK &&
            time.vt == VT_FILETIME;
        const quint64 modified = hasTime ? FILETIME_To_UInt64(time.filetime) : 0;
        QString path = QString::fromWCharArray(native.Path.Ptr(), native.Path.Len());
        path.replace(u'\\', u'/');
        if (native.IsDir && path.endsWith(u'/'))
            path.chop(1);
        const auto parts = path.split(u'/');
        if (parts.isEmpty() || path.size() > 32767 ||
            std::any_of(parts.cbegin(), parts.cend(),
                        [](const QString &part) { return !SafeComponent(part); })) {
            error = QStringLiteral("An entry name cannot be represented safely in an SMB mount: ") +
                    path;
            return E_INVALIDARG;
        }
        quint64 parent = 1;
        QString prefix;
        for (qsizetype component = 0; component < parts.size(); ++component) {
            const bool last = component + 1 == parts.size();
            const bool directory = !last || native.IsDir;
            if (!prefix.isEmpty())
                prefix += u'/';
            prefix += parts.at(component);
            const auto key = Key(prefix);
            auto existing = state.Paths.constFind(key);
            if (existing != state.Paths.cend()) {
                auto &entry = state.Entries[qsizetype(*existing - 1)];
                if (!directory || !entry.Directory || entry.Name != parts.at(component)) {
                    error = QStringLiteral("Entry names conflict in the SMB namespace: ") + path;
                    return E_INVALIDARG;
                }
                parent = *existing;
                if (last)
                    entry.Modified = modified;
                continue;
            }
            quint64 size = 0;
            if (!directory) {
                NWindows::NCOM::CPropVariant value;
                RINOK(state.Reader->arc().Archive->GetProperty(index, kpidSize, &value))
                UInt64 nativeSize;
                if (!ConvertPropVariantToUInt64(value, nativeSize)) {
                    error = QStringLiteral("The reader does not report this entry's size: ") + path;
                    return E_NOTIMPL;
                }
                size = nativeSize;
            }
            const quint64 id = quint64(state.Entries.size()) + 1;
            state.Entries.append(
                {id, parent, parts.at(component), size, directory, last ? modified : 0});
            state.Paths.insert(key, id);
            if (!directory) {
                State::Item item{index};
                NWindows::NCOM::CPropVariant encrypted;
                if (state.Reader->arc().Archive->GetProperty(index, kpidEncrypted, &encrypted) ==
                    S_OK)
                    item.Encrypted = encrypted.vt == VT_BOOL && encrypted.boolVal != VARIANT_FALSE;
                state.Items.insert(id, std::move(item));
            }
            parent = id;
        }
    }
    return S_OK;
}

ArchiveReadOnlyView::ReadResult ArchiveReadOnlyView::read(quint64 id, quint64 offset,
                                                          quint32 length) try {
    ReadResult result;
    auto &state = *state_;
    if (state.Cancelled) {
        result.Result = E_ABORT;
        return result;
    }
    if (!state.Items.contains(id) || length > 1024 * 1024 ||
        offset > quint64(std::numeric_limits<qint64>::max())) {
        result.Result = E_INVALIDARG;
        return result;
    }
    const auto size = state.Entries.at(qsizetype(id - 1)).Size;
    if (offset >= size || length == 0) {
        result.Result = S_OK;
        return result;
    }
    result.Result = state.stream(id, result);
    if (result.Result != S_OK)
        return result;
    result.Data.resize(qsizetype(std::min<quint64>(length, size - offset)));
    CMyComPtr<IProgress> request = new ReadRequest(state.Cancelled);
    UInt32 actual = 0;
    result.Result = state.Items[id].Stream->ReadAt(offset, result.Data.data(),
                                                   UInt32(result.Data.size()), &actual, request);
    if (result.Result != S_OK) {
        result.Data.clear();
        return result;
    }
    if (actual > quint64(result.Data.size())) {
        result.Data.clear();
        result.Result = E_FAIL;
        result.Error = QStringLiteral("The reader returned an invalid byte count.");
        return result;
    }
    result.Data.resize(actual);
    if (!actual) {
        result.Result = E_FAIL;
        result.Error = QStringLiteral("The source ended before the declared entry size.");
    }
    return result;
} catch (const std::bad_alloc &) {
    return {E_OUTOFMEMORY, -1, {}, QStringLiteral("Insufficient memory for the requested read.")};
} catch (...) {
    return {E_FAIL, -1, {}, QStringLiteral("The reader could not complete the requested read.")};
}

HRESULT ArchiveBackend::createReadOnlyView(
    IUnknown *resource, quint64 cacheLimit, const QString &password,
    std::shared_ptr<ArchiveReadOnlyView> &view, QString &error,
    std::function<bool(const QString &, quint64)> extractionConsent,
    std::function<bool(const QString &, quint64, Int32)> recoveryConsent) {
    COM_TRY_BEGIN
    view.reset();
    auto opened = retainReadOnlyView(resource, cacheLimit, password, std::move(extractionConsent),
                                     std::move(recoveryConsent));
    const HRESULT result = prepareReadOnlyView(opened, error);
    if (result == S_OK)
        view = std::move(opened);
    return result;
    COM_TRY_END
}

std::shared_ptr<ArchiveReadOnlyView> ArchiveBackend::retainReadOnlyView(
    IUnknown *resource, quint64 cacheLimit, const QString &password,
    std::function<bool(const QString &, quint64)> extractionConsent,
    std::function<bool(const QString &, quint64, Int32)> recoveryConsent) {
    return std::shared_ptr<ArchiveReadOnlyView>(new ArchiveReadOnlyView(
        resource, cacheLimit, password, std::move(extractionConsent), std::move(recoveryConsent)));
}
HRESULT ArchiveBackend::prepareReadOnlyView(const std::shared_ptr<ArchiveReadOnlyView> &view,
                                            QString &error) {
    COM_TRY_BEGIN
    return view ? view->initialize(error) : E_INVALIDARG;
    COM_TRY_END
}
