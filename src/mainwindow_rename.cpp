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

void MainWindow::renameSelected() {
    const auto &panel = app_.GetFocusedPanel();
    const bool streams = panel.GetFolder() && panel.IsAltStreamsFolder();
    if (state_ != State::Filesystem && state_ != State::Viewing && !streams)
        return;
    const bool archive = state_ == State::Viewing;
    if (!rename_->isEnabled())
        return;
    const QStringList paths = archive ? selectedArchiveItemsPaths() : selectedFilesystemPaths();
    if (paths.size() != 1)
        return;
    if (!archive && QFileInfo(paths.first()).isSymLink())
        return;
    const auto indices = sourceOperatedIndices();
    if (indices.Size() != 1)
        return;
    int row = -1;
    for (int i = 0; i < table_->rowCount(); ++i)
        if (table_->item(i, 0) &&
            table_->item(i, 0)->data(Qt::UserRole + 6).toUInt() == indices[0]) {
            row = i;
            break;
        }
    if (row < 0)
        return;
    auto *item = table_->item(row, 0);
    if (!item || item->data(Qt::UserRole + 6).isNull())
        return;
    renameEditingItem_ = item;
    renameOriginalPath_ = paths.first();
    renameSourceIndex_ = item->data(Qt::UserRole + 6).toUInt();
    renameActivationPending_ = true;
    const QString nameHint =
        archive || streams ? ToQString(app_.GetFocusedPanel().GetItemName(indices[0])) : QString();
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
    if (result != S_OK && result != E_ABORT)
        QMessageBox::warning(
            this, "Sevenzip Desktop",
            QtLocalText("Could not rename the item.", "Не удалось переименовать объект."));
    const auto selection = app_.GetFocusedPanel().TakeRefreshSelection();
    if (!selection) {
        // The source validation return rejects the edit without reloading.
        rejectLabelEdit(app_.GetFocusedPanel(), *table_, sourceIndex);
        refreshActions();
        return;
    }
    if (renderBoundFolder() == S_OK)
        restoreListSelection(*selection);
}
