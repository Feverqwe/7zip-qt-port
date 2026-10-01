#include "7zip/UI/FileManager/resource.h"
#include "CPP/7zip/UI/FileManager/FormatUtils.h"
#include "CPP/7zip/UI/FileManager/MyLoadMenu.h"
#include "CPP/7zip/UI/FileManager/PropertyName.h"
#include "filetable.h"
#include "mainwindow.h"
#include "qtlangcontrols.h"
#include <QEvent>
#include <QToolButton>

void MainWindow::reloadLangItems() {
    app_.ReloadLangItems();
    MyLoadMenu(*menuBar(), true);
    QtReloadLangTexts(*this);
    // App.cpp g_ArchiveButtons/g_StandardButtons StringResIDs. QToolButton
    // text is independent of the shared QAction's longer File menu caption.
    auto *toolbar = findChild<QToolBar *>("archiveToolbar");
    const UInt32 ids[] = {IDS_ADD,         IDS_EXTRACT,       IDS_TEST,       IDS_BUTTON_COPY,
                          IDS_BUTTON_MOVE, IDS_BUTTON_DELETE, IDS_BUTTON_INFO};
    QAction *actions[] = {toolbarAdd_, extract_, test_, copy_, move_, remove_, infoAction_};
    if (toolbar)
        for (unsigned i = 0; i < 7; ++i)
            if (auto *button = qobject_cast<QToolButton *>(toolbar->widgetForAction(actions[i])))
                QtBindLangText(button, ids[i]);
    for (QTableWidget *table : findChildren<QTableWidget *>()) {
        auto *files = dynamic_cast<FileTable *>(table);
        if (!files)
            continue;
        for (int i = 0; i < files->columnCount(); i++) {
            if (files->isRawProperty(i) || files->propertyId(i) == kpidNoProperty)
                continue;
            if (auto *header = files->horizontalHeaderItem(i))
                header->setText(
                    QString::fromWCharArray(GetNameOfProperty(files->propertyId(i), nullptr)));
        }
    }
    for (unsigned i = 0; i < kNumPanelsMax; i++) {
        if (auto *label = panelUi_[i].statusLabels[0]) {
            const CPanelStatus status = app_.Panels[i].Refresh_StatusBar();
            label->setText(ToQString(
                MyFormatNew(app_.LangString_N_SELECTED_ITEMS, ToUString(status.selectedItems))));
        }
    }
}

void MainWindow::changeEvent(QEvent *event) {
    if (event->type() == QEvent::LanguageChange && menuBar() && findChild<QMenu *>("fileMenu"))
        reloadLangItems();
    QMainWindow::changeEvent(event);
}
