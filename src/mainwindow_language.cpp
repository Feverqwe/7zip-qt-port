#include "7zip/UI/FileManager/resource.h"
#include "CPP/7zip/UI/FileManager/FormatUtils.h"
#include "CPP/7zip/UI/FileManager/MyLoadMenu.h"
#include "CPP/7zip/UI/FileManager/PropertyName.h"
#include "CPP/7zip/UI/FileManager/RootFolder.h"
#include "Windows/PropVariant.h"
#include "filetable.h"
#include "mainwindow.h"
#include "qtlangcontrols.h"
#include <QEvent>
#include <QToolButton>

void MainWindow::reloadLangItems() {
    app_.ReloadLangItems();
    MyLoadMenu(*menuBar(), true);
    refreshBookmarks();
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
            if (files->propertyId(i) == kpidNoProperty) {
                if (auto *header = files->horizontalHeaderItem(i))
                    header->setText(QtLocalText("Order", "Порядок"));
                continue;
            }
            if (files->isRawProperty(i))
                continue;
            if (auto *header = files->horizontalHeaderItem(i))
                header->setText(
                    QString::fromWCharArray(GetNameOfProperty(files->propertyId(i), nullptr)));
        }
        for (int row = 0; row < files->rowCount(); ++row)
            if (auto *item = files->item(row, 0);
                item && item->data(Qt::UserRole + 6).toUInt() == UInt32(-1))
                item->setToolTip(QtLocalText("Parent folder", "Родительская папка"));
    }
    for (unsigned i = 0; i < kNumPanelsMax; i++) {
        if (auto *root = dynamic_cast<CRootFolder *>(app_.Panels[i].GetFolder())) {
            root->Init();
            if (auto *table = panelUi_[i].table) {
                const bool sorting = table->isSortingEnabled();
                table->setSortingEnabled(false);
                for (int row = 0; row < table->rowCount(); ++row) {
                    auto *item = table->item(row, 0);
                    if (!item)
                        continue;
                    NWindows::NCOM::CPropVariant name;
                    if (static_cast<IFolderFolder *>(root)->GetProperty(
                            item->data(Qt::UserRole + 6).toUInt(), kpidName, &name) == S_OK &&
                        name.vt == VT_BSTR) {
                        const QString nativeName = QString::fromWCharArray(name.bstrVal);
                        item->setText(nativeName.normalized(QString::NormalizationForm_C));
                        item->setData(Qt::UserRole + 3, nativeName);
                    }
                }
                table->setSortingEnabled(sorting);
            }
        }
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
