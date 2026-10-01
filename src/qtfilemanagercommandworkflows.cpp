#include "7zip/PropID.h"
#include "7zip/UI/FileManager/resource.h"
#include "Windows/PropVariant.h"
#include "Windows/PropVariantConv.h"
#include "mainwindow.h"
#include "qtfilemanagercommands.h"
#include <QScopedValueRollback>

bool QtFileManagerCommands::IsShiftDown() const {
    return QApplication::keyboardModifiers().testFlag(Qt::ShiftModifier);
}

void QtFileManagerCommands::OpenNative(bool internal, bool external, const wchar_t *type) {
    auto &w = window_;
    auto &panel = w.app_.GetFocusedPanel();
    CSelectedState selection;
    panel.SaveSelectedState(selection);
    const bool wasFilesystem = panel.IsFSFolder();
    const bool wasRoot = panel.IsRootFolder();
    CMyComPtr<IFolderFolder> previousFolder = panel.GetFolder();
    const UString previousPrefix = panel.GetCurrentFolderPrefix();
    const auto previousState = w.state_;
    const QString previousDirectory = w.directory_;
    if (wasFilesystem)
        w.returnDirectory_ = previousDirectory;
    w.setState(MainWindow::State::Operating);
    {
        QScopedValueRollback<bool> source(w.sourceArchiveProgress_, true);
        if (internal && !external)
            panel.OpenFocusedItemAsInternal(type);
        else
            panel.OpenSelectedItems(internal);
    }
    if (wasFilesystem && panel.IsFSFolder() &&
        QDir::cleanPath(ToQString(GetFolderPath(panel.GetFolder()))) != previousDirectory)
        w.history_.append(previousDirectory);
    if (wasRoot && panel.IsFSFolder())
        w.history_.append(QString()); // Empty native root prefix in Qt Back history.
    if ((IFolderFolder *)previousFolder == panel.GetFolder() &&
        previousPrefix == panel.GetCurrentFolderPrefix())
        w.setState(previousState);
    else
        w.renderBoundFolder();
}

void QtFileManagerCommands::OpenItem() { OpenNative(true, true, nullptr); }
void QtFileManagerCommands::OpenItemInside(const wchar_t *type) { OpenNative(true, false, type); }
void QtFileManagerCommands::OpenItemOutside() { OpenNative(false, true, nullptr); }
void QtFileManagerCommands::EditItem(bool editor) {
    auto &panel = window_.app_.GetFocusedPanel();
    CSelectedState selection;
    panel.SaveSelectedState(selection);
    panel.EditItem(editor);
}
void QtFileManagerCommands::DiffFiles() { window_.app_.DiffFiles(); }
void QtFileManagerCommands::Rename() {
    if (window_.app_.GetFocusedPanel().CheckBeforeUpdate(IDS_ERROR_RENAMING))
        window_.renameSelected();
}
void QtFileManagerCommands::CopyTo() {
    window_.app_.OnCopy(false, false, window_.app_.GetFocusedPanelIndex());
}
void QtFileManagerCommands::MoveTo() {
    window_.app_.OnCopy(true, false, window_.app_.GetFocusedPanelIndex());
}
void QtFileManagerCommands::Delete(bool toRecycleBin) {
    if (window_.app_.GetFocusedPanel().CheckBeforeUpdate(IDS_ERROR_DELETING))
        window_.deleteSelected(toRecycleBin);
}
void QtFileManagerCommands::CalculateCrc(const char *method) {
    window_.calculateHash(QString::fromLatin1(method));
}
void QtFileManagerCommands::Split() { window_.startVolume(VolumeOperation::Mode::Split); }
void QtFileManagerCommands::Combine() { window_.startVolume(VolumeOperation::Mode::Combine); }
void QtFileManagerCommands::Properties() { window_.showSelectedProperties(); }
void QtFileManagerCommands::CreateFolder() {
    if (window_.app_.GetFocusedPanel().CheckBeforeUpdate(IDS_CREATE_FOLDER_ERROR))
        window_.createFolder();
}
void QtFileManagerCommands::CreateFile() {
    if (window_.app_.GetFocusedPanel().CheckBeforeUpdate(IDS_CREATE_FILE_ERROR))
        window_.createFile();
}
void QtFileManagerCommands::AddToArchive() {
    if (window_.app_.GetFocusedPanel().IsFSFolder())
        window_.createDialog();
    else
        window_.addDialog();
}
void QtFileManagerCommands::ExtractArchives() { window_.extract(false); }
void QtFileManagerCommands::TestArchives() { window_.testArchive(); }
bool QtFileManagerCommands::ExecuteExtensionCommand(unsigned id) {
    switch (id) {
    case OpenDialog:
        window_.openDialog();
        break;
    case CreateArchive:
        window_.createDialog();
        break;
    case AddSources:
        window_.addDialog();
        break;
    case ExtractSelected:
        window_.extract(true);
        break;
    case ArchiveInformation:
        window_.showInfo();
        break;
    case Back:
        window_.goBack();
        break;
    case Journal:
        window_.showJournal();
        break;
    case TextView:
    case TextEdit:
        window_.viewSelectedFile(id == TextEdit);
        break;
    default:
        return false;
    }
    return true;
}
void QtFileManagerCommands::CloseWindow() { window_.close(); }
void QtFileManagerCommands::SelectAll(bool select) { window_.SelectAll(select); }
void QtFileManagerCommands::InvertSelection() { window_.InvertSelection(); }
void QtFileManagerCommands::SelectSpec(bool select) { window_.SelectSpec(select); }
void QtFileManagerCommands::SelectByType(bool select) { window_.SelectByType(select); }
void QtFileManagerCommands::Refresh_StatusBar() { window_.refreshActions(); }
void QtFileManagerCommands::SetListViewMode(unsigned index) { window_.setViewMode(index); }
void QtFileManagerCommands::SortItemsWithPropID(unsigned propID) {
    const auto mode = propID == kpidExtension    ? MainWindow::SortMode::Type
                      : propID == kpidMTime      ? MainWindow::SortMode::Date
                      : propID == kpidSize       ? MainWindow::SortMode::Size
                      : propID == kpidNoProperty ? MainWindow::SortMode::Unsorted
                                                 : MainWindow::SortMode::Name;
    window_.setSortMode(mode);
}
void QtFileManagerCommands::OpenRootFolder() { window_.goRoot(); }
void QtFileManagerCommands::OpenParentFolder() { window_.goParent(); }
void QtFileManagerCommands::FoldersHistory() { window_.showFoldersHistory(); }
void QtFileManagerCommands::ChangeFlatMode() {
    window_.applyFlatView(
        !(window_.state_ == MainWindow::State::Viewing ? window_.flatArchive_ : window_.flatDisk_));
}
void QtFileManagerCommands::RefreshView() { window_.refreshCurrent(); }
void QtFileManagerCommands::SwitchStandardToolbar() { window_.app_.SwitchStandardToolbar(); }
void QtFileManagerCommands::SwitchArchiveToolbar() { window_.app_.SwitchArchiveToolbar(); }
void QtFileManagerCommands::SwitchButtonsLables() { window_.app_.SwitchButtonsLables(); }
void QtFileManagerCommands::SwitchLargeButtons() { window_.app_.SwitchLargeButtons(); }
bool &QtFileManagerCommands::TimestampShowUTC() { return g_Timestamp_Show_UTC; }
void QtFileManagerCommands::OptionsDialog() { window_.showSettings(); }
void QtFileManagerCommands::OpenBookmark(unsigned index) { window_.openBookmark(index); }
void QtFileManagerCommands::SetBookmark(unsigned index) { window_.saveBookmark(index); }

// These IDs have no attached/available Qt consumer. Keep their original native
// switch branches while the mandatory owners port the workflow; policy rejects
// them before dispatch. Never expose a functioning-looking placeholder action.
void QtFileManagerCommands::InvokePluginCommand(unsigned) { Q_UNREACHABLE(); }
void QtFileManagerCommands::ReleasePluginMenus() { Q_UNREACHABLE(); }
void QtFileManagerCommands::VerCtrl(unsigned id) {
    auto &w = window_;
    CSelectedState selection;
    w.app_.GetFocusedPanel().SaveSelectedState(selection);
    w.app_.VerCtrl(id);
    w.showDirectory(w.directory_, false);
    if (w.scanningDisk_)
        w.scanSelectedState_ = selection;
    else
        w.app_.GetFocusedPanel().RefreshListCtrl(selection);
    w.refreshActions();
}
void QtFileManagerCommands::Comment() { Q_UNREACHABLE(); }
void QtFileManagerCommands::Link() { Q_UNREACHABLE(); }
void QtFileManagerCommands::OpenAltStreams() { Q_UNREACHABLE(); }
void QtFileManagerCommands::Change_AutoRefresh_Mode() {
    window_.app_.Change_AutoRefresh_Mode();
    window_.autoRefreshAction_->setChecked(window_.app_.Get_AutoRefresh_Mode());
    QSettings settings(MainWindow::toolbarSettingsPath(), QSettings::IniFormat);
    settings.setValue(QStringLiteral("view/autoRefresh"), window_.app_.Get_AutoRefresh_Mode());
}
void QtFileManagerCommands::SwitchOnOffOnePanel() { Q_UNREACHABLE(); }
void QtFileManagerCommands::RedrawListItems_InPanels() {
    QSettings settings(MainWindow::toolbarSettingsPath(), QSettings::IniFormat);
    settings.setValue(QStringLiteral("view/timestampUTC"), g_Timestamp_Show_UTC);
    window_.redrawTimestampColumns();
}
unsigned QtFileManagerCommands::TimestampLevelCount() const {
    return window_.app_._timestampLevels.Size();
}
int QtFileManagerCommands::TimestampLevel(unsigned index) const {
    return window_.app_._timestampLevels[index];
}
void QtFileManagerCommands::SetTimestampLevel(int level) {
    window_.app_.SetTimestampLevel(level);
    QSettings settings(MainWindow::toolbarSettingsPath(), QSettings::IniFormat);
    settings.setValue(QStringLiteral("view/timestampLevel"), level);
    window_.redrawTimestampColumns();
}
void QtFileManagerCommands::MyBenchmark(bool) { Q_UNREACHABLE(); }
void QtFileManagerCommands::ShowHelpWindow(const char *) { Q_UNREACHABLE(); }
void QtFileManagerCommands::AboutDialog() { Q_UNREACHABLE(); }
void QtFileManagerCommands::MyBrowseForTempFolder() { Q_UNREACHABLE(); }
