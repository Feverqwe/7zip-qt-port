#include "qtshellarchiveservices.h"
#include "CPP/7zip/UI/Common/CompressCall.h"
#include "CPP/7zip/UI/FileManager/LangUtils.h"
#include "CPP/7zip/UI/GUI/ExtractGUI.h"
#include "CPP/7zip/UI/GUI/HashGUI.h"
#include "CPP/7zip/UI/GUI/UpdateGUI.h"
#include "Common/MyException.h"
#include "archivebackend.h"
#include "archivecodeccatalog.h"
#include "qtarchivebatchopen.h"
#include "qtextractpathpolicy.h"
#include <QDir>
#include <QMessageBox>
#include <QScopeGuard>

namespace {
// The source owns wildcard substitution and dialog edits. This policy reads
// its current output root when the source callback validates a final path.
class COutputPolicy final : public IQtExtractPathPolicy {
    const CExtractOptions &_options;
    QString Root() const {
        const QString output =
            QDir::fromNativeSeparators(QString::fromWCharArray(fs2us(_options.OutputDir).Ptr()));
        const int wildcard = output.indexOf('*');
        return wildcard < 0 ? output : output.left(output.lastIndexOf('/', wildcard) + 1);
    }

  public:
    explicit COutputPolicy(const CExtractOptions &options) : _options(options) {}
    HRESULT CheckPath(const FString &path, bool directory, bool stream, UString &message) override {
        return CQtExtractPathPolicy(Root()).CheckPath(path, directory, stream, message);
    }
    HRESULT CheckLink(const FString &path, const FString &target, bool relative,
                      UString &message) override {
        return CQtExtractPathPolicy(Root()).CheckLink(path, target, relative, message);
    }
};
} // namespace

QtShellArchiveServices::QtShellArchiveServices(ArchiveBackend &backend, QWidget *parent,
                                               OpenSink open, CompressSink compress,
                                               ErrorSink error, ModalSink modal,
                                               QString settingsPath)
    : _backend(backend), _parent(parent), _catalog(backend.codecCatalog()), _open(std::move(open)),
      _compress(std::move(compress)), _modal(std::move(modal)),
      _settingsPath(std::move(settingsPath)), _error(std::move(error)) {
    // Metadata alone supports source ParseOpenTypes. Native factories remain
    // with the retained immutable backend catalog, never copied/unloaded here.
    if (_catalog && _catalog->result() == S_OK)
        _metadata.Formats = _catalog->nativeCodecs().Formats;
}
QtShellArchiveServices::~QtShellArchiveServices() = default;
FString QtShellArchiveServices::GetSfxModulePath() const {
    if (!_catalog || _catalog != _backend.codecCatalog())
        return FString();
    return us2fs(_catalog->sfxModule().path().toStdWString().c_str());
}
HRESULT QtShellArchiveServices::PresentCompressDialog(
    CCompressDialog &source, const CObjectVector<NWildcard::CCensorPath> &paths, QWidget *parent) {
    return _modal ? _modal(source, paths, parent) : E_NOTIMPL;
}
void QtShellArchiveServices::GetUserCodecs(CCodecs &codecs,
                                           CObjectVector<CCodecInfoUser> &userCodecs) {
    if (&codecs != &_metadata || !_catalog || _catalog != _backend.codecCatalog())
        throw CSystemException(E_ABORT);
    _catalog->getUserCodecs(userCodecs);
}
CCodecs &QtShellArchiveServices::Codecs() {
    if (!_catalog || _catalog->result() != S_OK)
        throw CSystemException(E_FAIL);
    return _metadata;
}
HRESULT QtShellArchiveServices::LoadHashProviders(CExternalCodecs &providers) {
    return _backend.loadHashProviders(providers);
}
HRESULT QtShellArchiveServices::ExtractGUI(CCodecs &codecs, const CObjectVector<COpenType> &types,
                                           UStringVector &paths, UStringVector &fullPaths,
                                           const NWildcard::CCensorNode &censor,
                                           CExtractOptions &options, bool showDialog,
                                           bool &messageWasDisplayed,
                                           CExtractCallbackImp &callback) {
    if (_backend.busy() || _backend.codecCatalog() != _catalog)
        return E_FAIL;
    QtArchiveBatchOpen open(_backend);
    if (types.Size() > 1)
        return E_NOTIMPL;
    if (!types.IsEmpty()) {
        const auto &type = types[0];
        if (type.IsHashType)
            open.SetOpenType(UString(L"hash"));
        else if (type.FormatIndex >= 0 && unsigned(type.FormatIndex) < codecs.Formats.Size())
            open.SetOpenType(codecs.Formats[unsigned(type.FormatIndex)].Name);
        else
            return E_NOTIMPL;
    }
    COutputPolicy policy(options);
    auto *previous = options.NtOptions.QtPathPolicy;
    const auto restore = qScopeGuard([&] { options.NtOptions.QtPathPolicy = previous; });
    options.NtOptions.QtPathPolicy = &policy;
    const HRESULT result =
        ::ExtractGUI(&codecs, types, CIntVector(), paths, fullPaths, censor, options, nullptr,
                     showDialog, messageWasDisplayed, &callback, &open, &open, _parent);
    return result;
}
HRESULT QtShellArchiveServices::HashCalcGUI(const CExternalCodecs *providers,
                                            const NWildcard::CCensor &censor,
                                            const CHashOptions &options,
                                            bool &messageWasDisplayed) {
    return ::HashCalcGUI(providers, censor, options, messageWasDisplayed);
}
void QtShellArchiveServices::Open(const UString &path, const UString &type) {
    if (_open)
        _result = _open(path, type);
    else
        ErrorMessageHRESULT(E_NOTIMPL);
}
void QtShellArchiveServices::ExtractArchives(const UStringVector &paths, const UString &folder,
                                             bool showDialog, bool elimDup, UInt32 zone) {
    _result = ::ExtractArchives(paths, folder, showDialog, elimDup, zone, *this);
}
void QtShellArchiveServices::TestArchives(const UStringVector &paths, bool hashMode) {
    _result = ::TestArchives(paths, hashMode, *this);
}
void QtShellArchiveServices::CompressFiles(const UString &folder, const UString &name,
                                           const UString &type, bool addExtension,
                                           const UStringVector &paths, bool email, bool showDialog,
                                           bool waitFinish) {
    if (_compress)
        _compress(folder, name, type, addExtension, paths, email, showDialog, waitFinish);
    else {
        const HRESULT result = ::CompressFiles(folder, name, type, addExtension, paths, email,
                                               showDialog, waitFinish, *this);
        if (result != S_OK)
            _result = result;
    }
}

HRESULT QtShellArchiveServices::UpdateGUI(CCodecs &codecs, const CObjectVector<COpenType> &types,
                                          const UString &path, NWildcard::CCensor &censor,
                                          CUpdateOptions &options, bool showDialog,
                                          bool &messageWasDisplayed, CUpdateCallbackGUI &callback) {
    if (&codecs != &_metadata || _backend.busy() || _catalog != _backend.codecCatalog())
        return _result = E_ABORT;
    // Unix has no upstream MAPI consumer; do not silently report email success.
#ifndef _WIN32
    if (options.EMailMode)
        return _result = E_NOTIMPL;
#endif
    _result = ::UpdateGUI(&codecs, types, path, censor, options, showDialog, messageWasDisplayed,
                          &callback, _parent, *this);
    return _result;
}

HRESULT QtShellArchiveServices::UpdateArchive(CCodecs *codecs,
                                              const CObjectVector<COpenType> &types,
                                              const UString &path, NWildcard::CCensor &censor,
                                              CUpdateOptions &options, CUpdateErrorInfo &error,
                                              IOpenCallbackUI *openCallback,
                                              IUpdateCallbackUI2 *callback, bool needSetPath) {
    if (codecs != &_metadata)
        return E_INVALIDARG;
    return _backend.updateSourceArchive(path, types, censor, options, error, openCallback, callback,
                                        needSetPath, _catalog);
}
void QtShellArchiveServices::CalcChecksum(const UStringVector &paths, const UString &method,
                                          const UString &folder, const UString &generateName) {
    const HRESULT result = ::CalcChecksum(paths, method, folder, generateName, *this);
    if (result != S_OK)
        _result = result;
}
void QtShellArchiveServices::ShowErrorMessage(const UString &message) {
    _result = E_FAIL;
    if (_error)
        _error(E_FAIL, message);
    else
        QMessageBox::warning(_parent, QStringLiteral("7-Zip"), QString::fromWCharArray(message));
}
void QtShellArchiveServices::ShowErrorMessageRes(UInt32 id) { ShowErrorMessage(LangString(id)); }
void QtShellArchiveServices::ErrorMessageHRESULT(HRESULT result) {
    _result = result;
    if (_error)
        _error(result, HResultToMessage(result));
    else
        ShowErrorMessage(HResultToMessage(result));
    _result = result;
}
