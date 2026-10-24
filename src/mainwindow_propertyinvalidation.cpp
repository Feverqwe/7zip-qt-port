#include "Windows/PropVariant.h"
#include "mainwindow.h"

// Keep the numeric size role used by interaction in sync with source stats.
// Display text is requested from the source panel when Qt paints a cell.
void MainWindow::redrawSourceProperties() {
    auto &panel = app_.GetFocusedPanel();
    if (!table_ || !panel.GetFolder() || app_.QtOperations().closed())
        return;
    table_->panelModel()->invalidateText();
    if (table_->isSortingEnabled())
        table_->panelModel()->sort(table_->horizontalHeader()->sortIndicatorSection(),
                                   table_->horizontalHeader()->sortIndicatorOrder());
    table_->viewport()->update();
    refreshActions();
}
