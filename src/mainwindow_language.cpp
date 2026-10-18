#include "7zip/UI/FileManager/resource.h"
#include "CPP/7zip/UI/FileManager/FormatUtils.h"
#include "CPP/7zip/UI/FileManager/MyLoadMenu.h"
#include "CPP/7zip/UI/FileManager/PropertyName.h"
#include "CPP/7zip/UI/FileManager/RootFolder.h"
#include "Windows/PropVariant.h"
#include "filetable.h"
#include "mainwindow.h"
#include "qtlangcontrols.h"
#include "qtpaneladdresscombo.h"
#include <QEvent>
#include <QToolButton>

void MainWindow::reloadLangItems() {
    if (app_.QtOperations().closed())
        return;
    app_.ReloadLangItems();
    MyLoadMenu(*menuBar(), true);
    refreshBookmarks();
    QtReloadLangTexts(*this);
    refreshToolbarTexts();
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
            if (auto *header = files->horizontalHeaderItem(i)) {
                const CPropColumn *column = files->sourceColumn(i);
                header->setText(QString::fromWCharArray(GetNameOfProperty(
                    files->propertyId(i), column ? column->Name.Ptr() : nullptr)));
            }
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
        auto &panel = app_.Panels[i];
        if (!app_.QtOperations().blocking() && !sourceArchiveProgress_ &&
            panel.ShouldProcessNotifications() && panel.GetFolder() && panel.IsFSDrivesFolder())
            if (auto *address = dynamic_cast<QtPanelAddressCombo *>(panelUi_[i].pathCombo))
                address->ReloadCurrentPath();
        if (panelUi_[i].statusLabels[0])
            app_.Panels[i].Refresh_StatusBar();
    }
}

void MainWindow::refreshToolbarTexts() {
    auto *toolbar = findChild<QToolBar *>("archiveToolbar");
    if (!toolbar)
        return;
    // Original AddButton and WM_NOTIFY/TTN_GETDISPINFO share SetButtonText.
    // QAction keeps its File menu caption; Qt supplies button/tooltip lifetime.
    for (QAction *action : {toolbarAdd_, extract_, test_, copy_, move_, remove_, infoAction_})
        if (auto *button = qobject_cast<QToolButton *>(toolbar->widgetForAction(action))) {
            UString text;
            SetButtonText(action->property("nativeCommandID").toInt(), text);
            const QString caption = QString::fromWCharArray(text);
            button->setText(caption);
            button->setToolTip(caption);
        }
}

void MainWindow::changeEvent(QEvent *event) {
    if (event->type() == QEvent::LanguageChange && menuBar() && findChild<QMenu *>("fileMenu"))
        reloadLangItems();
    QMainWindow::changeEvent(event);
}
