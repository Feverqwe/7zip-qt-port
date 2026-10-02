#pragma once
#include "qtfilemanagercommandservices.h"
#include <QHash>
#include <QList>

class QAction;
class MainWindow;

// Qt presentation and existing transport consumers for CApp's native entry.
// Native menu policy is recomputed for execution, never read from QAction state.
class QtFileManagerCommands final : public IQtFileManagerMenuCommandServices {
  public:
    enum Extension : unsigned {
        OpenDialog = 1000,
        CreateArchive,
        AddSources,
        ExtractSelected,
        ArchiveInformation,
        Back,
        Journal,
        TextView,
        TextEdit
    };
    explicit QtFileManagerCommands(MainWindow &window);
    void AttachActions();
    void RefreshActions();
    bool CanExecuteCommand(unsigned id) override;
    bool IsShiftDown() const override;
    void InvokePluginCommand(unsigned id) override;
    void ReleasePluginMenus() override;
    void OpenItem() override;
    void OpenItemInside(const wchar_t *type) override;
    void OpenItemOutside() override;
    void EditItem(bool useEditor) override;
    void Rename() override;
    void CopyTo() override;
    void MoveTo() override;
    void Delete(bool toRecycleBin) override;
    void CalculateCrc(const char *methodName) override;
    void DiffFiles() override;
    void VerCtrl(unsigned id) override;
    void Split() override;
    void Combine() override;
    void Properties() override;
    void Comment() override;
    void CreateFolder() override;
    void CreateFile() override;
    void Link() override;
    void OpenAltStreams() override;
    void AddToArchive() override;
    void ExtractArchives() override;
    void TestArchives() override;
    bool ExecuteExtensionCommand(unsigned id) override;
    void CloseWindow() override;
    void SelectAll(bool select) override;
    void InvertSelection() override;
    void SelectSpec(bool select) override;
    void SelectByType(bool select) override;
    void Refresh_StatusBar() override;
    void SetListViewMode(unsigned index) override;
    void SortItemsWithPropID(unsigned propID) override;
    void OpenRootFolder() override;
    void OpenParentFolder() override;
    void FoldersHistory() override;
    void ChangeFlatMode() override;
    void RefreshView() override;
    void Change_AutoRefresh_Mode() override;
    void SwitchOnOffOnePanel() override;
    void SwitchStandardToolbar() override;
    void SwitchArchiveToolbar() override;
    void SwitchButtonsLables() override;
    void SwitchLargeButtons() override;
    bool &TimestampShowUTC() override;
    void RedrawListItems_InPanels() override;
    unsigned TimestampLevelCount() const override;
    int TimestampLevel(unsigned index) const override;
    void SetTimestampLevel(int level) override;
    void OptionsDialog() override;
    void MyBenchmark(bool totalMode) override;
    void ShowHelpWindow(const char *topic) override;
    void AboutDialog() override;
    void MyBrowseForTempFolder() override;
    void OpenBookmark(unsigned index) override;
    void SetBookmark(unsigned index) override;

  private:
    struct TransportCapabilities {
        bool filesystem = false, archive = false, writable = false, mutationWritable = false;
        bool nested = false, root = false;
        bool comment = false;
        bool selected = false, one = false, oneFile = false, operated = false, focused = false;
        bool focusedFile = false, folderSize = false;
        bool split = false, combine = false, entries = false, rows = false, foldersHistory = false;
    };
    TransportCapabilities ReadTransportCapabilities();
    bool TransportAllows(unsigned id, const TransportCapabilities &capabilities) const;
    QHash<unsigned, bool> Availability();
    void AttachFileActions();
    void AttachPanelActions();
    void AttachViewActions();
    void Bind(QAction *action, unsigned id);
    void BindToggle(QAction *action, unsigned id, bool CApp::*state);
    void OpenNative(bool internal, bool external, const wchar_t *type);
    MainWindow &window_;
    QHash<unsigned, QList<QAction *>> actions_;
};
