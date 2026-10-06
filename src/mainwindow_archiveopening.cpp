#include "7zip/PropID.h"
#include "7zip/UI/FileManager/resourceGui.h"
#include "7zip/UI/GUI/ExtractRes.h"
#include "CPP/7zip/UI/FileManager/FormatUtils.h"
#include "CPP/7zip/UI/FileManager/MessagesDialog.h"
#include "CPP/7zip/UI/FileManager/ProgressDialog2.h"
#include "Common/StringConvert.h"
#include "Windows/PropVariant.h"
#include "mainwindow.h"
#include "qtarchivefileresource.h"
#include "qtlangcontrols.h"
#include <QScopeGuard>
#include <QScopedValueRollback>

bool MainWindow::canHandleExternalUpdate() const {
    return !app_.QtOperations().active() && !app_.QtOperations().closed() &&
           !sourceArchiveProgress_ && !backend_->busy() && !QApplication::activeModalWidget();
}

bool MainWindow::runExternalArchiveUpdate(CPanel &panel, const std::function<bool()> &body) {
    if (!canHandleExternalUpdate())
        return false;
    CSelectedState selection;
    panel.SaveSelectedState(selection);
    bool result;
    {
        QScopedValueRollback<bool> owner(sourceArchiveProgress_, true);
        app_.QtOperations().Begin(QtAppOperationState::Kind::Operation);
        const auto finish = qScopeGuard([&] { app_.QtOperations().End(); });
        result = body();
    }
    if (result) {
        const unsigned previous = projectedPanel_;
        unsigned index = 0;
        while (index < kNumPanelsMax && &app_.Panels[index] != &panel)
            ++index;
        if (index < kNumPanelsMax) {
            activatePanelUi(index);
            renderBoundFolder();
            restoreListSelection(selection);
            activatePanelUi(previous);
        }
    }
    refreshActions();
    return result;
}

void MainWindow::openArchive(const QString &path) { openArchiveAs(path, {}); }

HRESULT MainWindow::openArchiveAs(const QString &path, const QString &type) {
    if (app_.QtOperations().closed())
        return E_ABORT;
    QtAppOperationState::CallScope call(app_.QtOperations());
    if (state_ == State::Loading || state_ == State::Operating)
        return E_ABORT;
    const QFileInfo file(path);
    if (!file.isFile() || file.isSymLink()) {
        statusBar()->showMessage(
            QtLocalText("File not found: ", QStringLiteral("Файл не найден: ")) + path);
        return E_INVALIDARG;
    }
    filesystemScanner_->cancel();
    listProjection_->cancel();
    scanningArchive_ = false;
    scanningDisk_ = false;
    scanSelectedState_.reset();
    scanSelectedPaths_.reset();
    if (state_ == State::Filesystem)
        returnDirectory_ = directory_;
    auto &panel = app_.GetFocusedPanel();
    operationName_ = QtLangText(IDS_OPENNING);
    QString message;
    const auto completion = connect(backend_, &ArchiveBackend::finished, this,
                                    [&message](bool, const QString &text) { message = text; });
    setState(State::Loading);
    HRESULT result;
    {
        QScopedValueRollback<bool> source(sourceArchiveProgress_, true);
        COpenResult opened;
        result = panel.BindToPath(ToUString(file.absoluteFilePath()), ToUString(type), opened);
        // BindToPath can successfully bind the containing filesystem folder
        // after its archive probe returns S_FALSE. An archive-only request
        // must retain that refusal rather than report "Archive opened".
        if (result == S_OK && !opened.ArchiveIsOpened) {
            result = S_FALSE;
            message = ToQString(MyFormatNew(opened.Encrypted ? IDS_CANT_OPEN_ENCRYPTED_ARCHIVE
                                                             : IDS_CANT_OPEN_ARCHIVE,
                                            ToUString(file.absoluteFilePath())));
        }
    }
    disconnect(completion);
    if (panel.IsArcFolder())
        renderBoundFolder();
    else
        showDirectory(returnDirectory_, false);
    if (result == E_ABORT)
        message = QtLocalText("Operation canceled.", QStringLiteral("Операция отменена."));
    else if (message.isEmpty())
        message = result == S_OK ? QtLocalText("Archive opened.", QStringLiteral("Архив открыт."))
                                 : ToQString(HResultToMessage(result));
    statusBar()->showMessage(message);
    if (result != S_OK && result != E_ABORT)
        QMessageBox::warning(this, QStringLiteral("Sevenzip Desktop"), message);
    return panel.IsArcFolder() ? result : result == S_OK ? S_FALSE : result;
}

bool MainWindow::isNestedArchive() const {
    return app_.Panels[app_.GetFocusedPanelIndex()].GetParentFolders().Size() > 1;
}

HRESULT MainWindow::renderBoundFolder(bool updateAddress) {
    if (app_.QtOperations().closed())
        return E_ABORT;
    QtAppOperationState::CallScope call(app_.QtOperations());
    auto &panel = app_.GetFocusedPanel();
    const auto selection = panel.TakeRefreshSelection();
    panel.LoadFullPath();
    if (panel.IsRootFolder() || panel.IsFSDrivesFolder() || panel.IsNetFolder() ||
        panel.IsAltStreamsFolder()) {
        const HRESULT loaded = renderRootFolder(updateAddress);
        if (loaded != S_OK)
            return loaded;
        if (selection) {
            panel.RefreshListCtrl(*selection);
            refreshActions();
        }
        return S_OK;
    }
    if (!panel.IsArcFolder()) {
        const QString directory = panel.GetFolder()
                                      ? QDir::cleanPath(ToQString(GetFolderPath(panel.GetFolder())))
                                      : returnDirectory_;
        const HRESULT loaded =
            showDirectory(directory.isEmpty() ? returnDirectory_ : directory, false,
                          panel.GetFolder() && panel.IsFSFolder(), updateAddress);
        if (loaded == S_OK && selection)
            restoreListSelection(*selection);
        refreshActions();
        return loaded;
    }
    auto *folder = dynamic_cast<CAgentFolder *>(panel.GetFolder());
    if (!folder)
        return E_NOTIMPL;
    auto *resource = dynamic_cast<QtArchiveFileResource *>(folder->GetEngineResource());
    if (!resource)
        return E_NOTIMPL;
    archive_ = resource->EnginePath;
    info_ = resource->Info;
    const auto &parents = panel.GetParentFolders();
    archivePassword_ = parents.IsEmpty() ? QString() : ToQString(parents.Back().Password);
    archiveFolder_ = QDir::fromNativeSeparators(ToQString(GetFolderPath(folder)));
    if (archiveFolder_.endsWith('/'))
        archiveFolder_.chop(1);
    displayArchive_ = QDir::fromNativeSeparators(ToQString(panel.GetCurrentFolderPrefix()));
    displayArchive_.chop(archiveFolder_.size() +
                         (archiveFolder_.isEmpty() || archiveFolder_.endsWith(':') ? 1 : 2));
    entries_.clear();
    IInArchive *metadata = folder->GetArchiveProperties();
    UInt32 count = 0;
    if (metadata && metadata->GetNumberOfItems(&count) == S_OK) {
        for (UInt32 index = 0; index < count; ++index) {
            NWindows::NCOM::CPropVariant path, directory;
            metadata->GetProperty(index, kpidPath, &path);
            metadata->GetProperty(index, kpidIsDir, &directory);
            ArchiveEntry entry;
            if (path.vt == VT_BSTR)
                entry.path = QDir::fromNativeSeparators(QString::fromWCharArray(path.bstrVal));
            entry.directory = directory.vt == VT_BOOL && directory.boolVal != VARIANT_FALSE;
            entries_.append(entry);
        }
    }
    {
        QSignalBlocker block(flatView_);
        flatView_->setChecked(flatArchive_);
    }
    // Keep operation guards until rows and native selection agree. The enum
    // here selects Qt presentation; the app owner still blocks commands.
    state_ = State::Viewing;
    const HRESULT loaded = renderArchiveFolder(updateAddress);
    if (loaded == S_OK && selection)
        restoreListSelection(*selection);
    setState(State::Viewing);
    return loaded;
}

void MainWindow::showArchiveFolder(const QString &folder) {
    if (app_.QtOperations().closed())
        return;
    QtAppOperationState::CallScope call(app_.QtOperations());
    auto &panel = app_.GetFocusedPanel();
    if (panel.GetParentFolders().IsEmpty())
        return;
    const UString path = panel.GetParentFolders().Back().VirtualPath + L"/" + ToUString(folder);
    {
        QScopedValueRollback<bool> source(sourceArchiveProgress_, true);
        COpenResult result;
        if (panel.BindToPath(path, UString(), result) != S_OK)
            return;
    }
    renderBoundFolder();
}

void MainWindow::openNestedArchive(const QString &entry) {
    if (app_.QtOperations().closed())
        return;
    QtAppOperationState::CallScope call(app_.QtOperations());
    auto &panel = app_.GetFocusedPanel();
    UInt32 count = 0;
    if (!panel.GetFolder() || panel.GetFolder()->GetNumberOfItems(&count) != S_OK)
        return;
    for (UInt32 index = 0; index < count; ++index) {
        const QString path = QDir::fromNativeSeparators(
            ToQString(GetFolderPath(panel.GetFolder()) + panel.GetItemRelPath(index)));
        if (path != entry)
            continue;
        setState(State::Operating);
        {
            QScopedValueRollback<bool> source(sourceArchiveProgress_, true);
            panel.LoadFullPath();
            panel.OpenItemInArchive(index, true, false, false, false);
        }
        renderBoundFolder();
        return;
    }
}

void MainWindow::goBack() {
    if (state_ == State::Loading || state_ == State::Operating)
        return;
    if (state_ == State::Viewing ||
        (state_ == State::Namespace && !app_.GetFocusedPanel().IsRootFolder()))
        goParent();
    else if (!history_.isEmpty()) {
        const QString previous = history_.takeLast();
        if (previous.isEmpty())
            goRoot(false);
        else
            navigateTo(previous, true);
    }
}

void MainWindow::goUp() {
    if (state_ == State::Viewing && !archiveFolder_.isEmpty())
        goParent();
}

void MainWindow::goParent() {
    if (app_.QtOperations().closed())
        return;
    QtAppOperationState::CallScope call(app_.QtOperations());
    auto &panel = app_.GetFocusedPanel();
    if (state_ != State::Viewing && state_ != State::Filesystem &&
        (state_ != State::Namespace || panel.IsRootFolder()))
        return;
    if (!panel.GetFolder())
        return;
    setState(State::Operating);
    {
        QScopedValueRollback<bool> source(sourceArchiveProgress_, true);
        panel.OpenParentFolder();
    }
    renderBoundFolder();
    if (panel.IsRootFolder() && !history_.isEmpty() && history_.last().isEmpty())
        history_.removeLast(); // Parent consumed the virtual-root Back location.
}

QString MainWindow::archiveDisplayPath() const {
    if (displayArchive_.isEmpty())
        return archiveFolder_;
    return displayArchive_ +
           (archiveFolder_.isEmpty() ? QString() : QStringLiteral(" / ") + archiveFolder_);
}
