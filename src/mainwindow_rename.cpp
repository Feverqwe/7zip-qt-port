#include "CPP/7zip/UI/FileManager/Panel.h"
#include "mainwindow.h"
#include "qtlangcontrols.h"
#include <QApplication>
#include <QDir>
#include <QFileInfo>
#include <QLineEdit>
#include <QMessageBox>

namespace {
// Qt equivalent of rejecting the native LV_DISPINFO edit without a reload.
void rejectLabelEdit(CPanel &panel, QTableWidget &view, UInt32 sourceIndex) {
    for (int row = 0; row < view.rowCount(); ++row)
        if (view.item(row, 0)->data(Qt::UserRole + 6).toUInt() == sourceIndex) {
            view.item(row, 0)->setData(Qt::EditRole, ToQString(panel.GetItemName(sourceIndex)));
            break;
        }
    CSelectedState state;
    panel.SaveSelectedState(state);
    panel.RestoreLoadedSelection(state);
}

} // namespace

void MainWindow::beginRename(int row) {
    auto &panel = app_.GetFocusedPanel();
    const bool streams = panel.GetFolder() && panel.IsAltStreamsFolder();
    if (state_ != State::Filesystem && state_ != State::Viewing && !streams)
        return;
    const bool archive = state_ == State::Viewing;
    if (!rename_->isEnabled())
        return;
    if (row < 0 || row >= table_->rowCount() || panel.OnBeginLabelEdit(row))
        return;
    auto *item = table_->item(row, 0);
    if (!item || item->data(Qt::UserRole + 6).isNull())
        return;
    renameEditingItem_ = item;
    renameSourceIndex_ = item->data(Qt::UserRole + 6).toUInt();
    renameOriginalPath_ = ToQString(archive ? panel.GetItemRelPath2(renameSourceIndex_)
                                            : panel.GetItemFullPath(renameSourceIndex_));
    renameActivationPending_ = true;
    const QString nameHint =
        archive || streams ? ToQString(panel.GetItemName(renameSourceIndex_)) : QString();
    if (viewStack_->currentWidget() == iconList_) {
        const QModelIndex modelIndex = table_->model()->index(row, 0);
        iconList_->scrollTo(modelIndex);
        iconList_->edit(modelIndex);
        if (archive || streams)
            if (auto *editor = iconList_->findChild<QLineEdit *>()) {
                editor->setText(nameHint);
                editor->selectAll();
            }
    } else {
        table_->scrollToItem(item);
        table_->editItem(item);
        if (archive || streams)
            if (auto *editor = table_->findChild<QLineEdit *>()) {
                editor->setText(nameHint);
                editor->selectAll();
            }
    }
}

void MainWindow::finishFilesystemRename(quint32 sourceIndex, const QString &originalPath,
                                        const QString &name) {
    const bool archive = state_ == State::Viewing;
    const auto &panel = app_.GetFocusedPanel();
    const bool streams = panel.GetFolder() && panel.IsAltStreamsFolder();
    if (state_ != State::Filesystem && !archive && !streams)
        return;
    if (!archive && !streams && QFileInfo(originalPath).absolutePath() != directory_ && !flatDisk_)
        return;
    if (archive && (name.isEmpty() || name == "." || name == ".." || name.contains('/') ||
                    name.contains('\\'))) {
        rejectLabelEdit(app_.GetFocusedPanel(), *table_, sourceIndex);
        refreshActions();
        QMessageBox::warning(this, QStringLiteral("7-Zip"),
                             QtLocalText("Invalid file name.", "Недопустимое имя файла."));
        return;
    }
    const HRESULT result = app_.GetFocusedPanel().OnEndLabelEdit(sourceIndex, ToUString(name));
    const auto selection = app_.GetFocusedPanel().TakeRefreshSelection();
    if (!selection) {
        // The source validation return rejects the edit without reloading.
        if (result != S_OK && result != E_ABORT)
            QMessageBox::warning(
                this, "Sevenzip Desktop",
                QtLocalText("Could not rename the item.", "Не удалось переименовать объект."));
        rejectLabelEdit(app_.GetFocusedPanel(), *table_, sourceIndex);
        refreshActions();
        return;
    }
    // The source worker owns operation errors. Its posted reload ignores the
    // publication result and must not add a second rename diagnostic.
    if (renderBoundFolder() == S_OK)
        restoreListSelection(*selection);
}
