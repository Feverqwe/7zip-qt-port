#include "qtapppresentation.h"
#include "mainwindow.h"
#include "qtfilelisthover.h"

void QtAppPresentation::Initialize(CApp &app) {
    // Preserve the existing single-panel preferences when introducing the
    // original per-panel keys. Explicit source keys always take precedence.
    QSettings key(window_.toolbarSettingsPath(), QSettings::IniFormat);
    if (!key.contains("Toolbars") &&
        (key.contains("toolbar/text") || key.contains("toolbar/large") ||
         key.contains("toolbar/standard") || key.contains("toolbar/archive"))) {
        UInt32 mask = 0;
        if (key.value("toolbar/text", true).toBool())
            mask |= 1;
        if (key.value("toolbar/large", false).toBool())
            mask |= 2;
        if (key.value("toolbar/standard", true).toBool())
            mask |= 4;
        if (key.value("toolbar/archive", true).toBool())
            mask |= 8;
        SaveToolbarsMask(mask, window_.toolbarSettingsPath());
    }
    if (!key.contains("ListMode")) {
        CListMode modes;
        bool ok = false;
        const int mode = key.value("view/mode", 3).toInt(&ok);
        if (ok && mode >= 0 && mode < 4)
            modes.Panels[app.LastFocusedPanel < kNumPanelsMax ? app.LastFocusedPanel : 0] = mode;
        modes.Save(window_.toolbarSettingsPath());
    }
    const QString flatKey =
        QStringLiteral("FM/FlatViewArc%1")
            .arg(app.LastFocusedPanel < kNumPanelsMax ? app.LastFocusedPanel : 0);
    if (!key.contains(flatKey)) {
        key.setValue(flatKey, key.value("view/flatArchive", false));
        key.sync();
    }
    editMenu_ = window_.createMenusAndToolbar();
}

bool QtAppPresentation::CreatePanel(CPanel &panel, unsigned index) {
    if (index >= kNumPanelsMax || window_.panelUi_[index].container)
        return false;
    const unsigned previous = window_.projectedPanel_;
    const bool hadPrevious = window_.panelUi_[previous].container;
    if (hadPrevious)
        window_.storePanelUi(previous);
    window_.projectedPanel_ = index;
    window_.restorePanelUi(index);
    window_.app_.SetFocusedPanel(index);
    panel.LoadFullPath();
    window_.flatArchive_ = panel.GetFlatModeForArc();
    panel.SetFlatModeForDisk(window_.flatDisk_);
    window_.createFilePanel(editMenu_, index);
    panel.AttachSelectionView(window_.table_);
    window_.storePanelUi(index);
    if (hadPrevious && index != previous) {
        window_.restorePanelUi(previous);
        window_.projectedPanel_ = previous;
        window_.app_.SetFocusedPanel(previous);
    }
    return true;
}

CPanelCallback *QtAppPresentation::PanelCallback(unsigned) {
    // Commands/title use CApp's native entry. Two-panel/drop integration remains
    // mandatory P0-08. Native focus callbacks
    // are owned by CApp::m_PanelCallbackImp, not this presentation service.
    return nullptr;
}

void QtAppPresentation::SetListSettings(unsigned index, const CFmSettings &settings) {
    if (index >= kNumPanelsMax)
        return;
    window_.fileListSettings_ = settings;
    const auto &ui = window_.panelUi_[index];
    if (ui.table) {
        ui.table->setShowGrid(settings.ShowGrid);
        static_cast<FileTable *>(ui.table)->setFullRowHighlight(settings.FullRow);
    }
    if (ui.tableHover)
        ui.tableHover->SetEnabled(settings.SingleClick);
    if (ui.iconHover)
        ui.iconHover->SetEnabled(settings.SingleClick);
    if (ui.iconList)
        ui.iconList->setSelectionMode(settings.AlternativeSelection
                                          ? QAbstractItemView::SingleSelection
                                          : QAbstractItemView::ExtendedSelection);
}

void QtAppPresentation::ReloadToolbars(CApp &) { window_.updateToolbar(); }
void QtAppPresentation::ShowPanel(unsigned index, bool visible) {
    if (index < kNumPanelsMax && window_.panelUi_[index].container)
        window_.panelUi_[index].container->setVisible(visible);
}
void QtAppPresentation::MoveSubWindows() {
    if (window_.layout())
        window_.layout()->activate();
}
