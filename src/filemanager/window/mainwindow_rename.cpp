#include "filemanager/panels/Panel.h"
#include "filemanager/window/mainwindow.h"
#include <QAbstractItemDelegate>
#include <QDir>
#include <QFileInfo>
#include <QLineEdit>
#include <QScopedValueRollback>

namespace {
// Qt equivalent of rejecting the native LV_DISPINFO edit without a reload.
void rejectLabelEdit(CPanel &panel, QTableView &view, UInt32 sourceIndex) {
    for (int row = 0; row < view.model()->rowCount(); ++row)
        if (view.model()->index(row, 0).data(Qt::UserRole + 6).toUInt() == sourceIndex) {
            view.model()->setData(view.model()->index(row, 0),
                                  ToQString(panel.GetItemName(sourceIndex)), Qt::EditRole);
            break;
        }
    CSelectedState state;
    panel.SaveSelectedState(state);
    panel.RestoreLoadedSelection(state);
}

} // namespace

void MainWindow::connectRenameEditor(QAbstractItemView &view) {
    const unsigned owner = projectedPanel_;
    if (auto *table = qobject_cast<QTableView *>(&view))
        connect(
            table->model(), &QAbstractItemModel::dataChanged, this,
            [this, owner](const QModelIndex &first, const QModelIndex &last,
                          const QList<int> &roles) {
                if (!renameEdit_ || renameEdit_->Panel != owner || !renameEdit_->Item.isValid() ||
                    renameEdit_->Item.model() != first.model() ||
                    renameEdit_->Item.row() < first.row() || renameEdit_->Item.row() > last.row() ||
                    renameEdit_->Item.column() < first.column() ||
                    renameEdit_->Item.column() > last.column() ||
                    (!roles.isEmpty() && !roles.contains(Qt::EditRole) &&
                     !roles.contains(Qt::DisplayRole)))
                    return;
                const QString name = renameEdit_->Item.data(Qt::EditRole).toString();
                auto edit = std::move(*renameEdit_);
                renameEdit_.reset();
                QTimer::singleShot(0, this, [this, edit = std::move(edit), name] {
                    const QPointer<MainWindow> guard(this);
                    finishDeferredRename(edit, name);
                    if (guard && !renameEdit_)
                        renameActivationPending_ = false;
                });
            });
    connect(view.itemDelegate(), &QAbstractItemDelegate::closeEditor, this,
            [this, owner](QWidget *, QAbstractItemDelegate::EndEditHint) {
                if (renameEdit_ && renameEdit_->Panel == owner)
                    renameEdit_.reset();
                QTimer::singleShot(0, this, [this] {
                    if (!renameEdit_)
                        renameActivationPending_ = false;
                });
            });
}

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
    const QModelIndex item = table_->model()->index(row, 0);
    if (!item.isValid() || item.data(Qt::UserRole + 6).isNull())
        return;
    const quint32 sourceIndex = item.data(Qt::UserRole + 6).toUInt();
    renameEdit_ = RenameEdit{projectedPanel_, panel.GetFolder(), item, sourceIndex,
                             ToQString(archive ? panel.GetItemRelPath2(sourceIndex)
                                               : panel.GetItemFullPath(sourceIndex))};
    renameActivationPending_ = true;
    const QString nameHint =
        archive || streams ? ToQString(panel.GetItemName(sourceIndex)) : QString();
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
        table_->scrollTo(item);
        table_->edit(item);
        if (archive || streams)
            if (auto *editor = table_->findChild<QLineEdit *>()) {
                editor->setText(nameHint);
                editor->selectAll();
            }
    }
}

void MainWindow::finishDeferredRename(const RenameEdit &edit, const QString &name) {
    if (app_.QtOperations().closed() || app_.QtOperations().blocking() || sourceArchiveProgress_)
        return;
    QtAppOperationState::CallScope call(app_.QtOperations());
    withPanelProjection(edit.Panel, [&] {
        const auto &panel = app_.GetFocusedPanel();
        if (panel.GetFolder() != edit.Folder || !edit.Item.isValid() ||
            !panel.ShouldProcessNotifications() || edit.SourceIndex >= panel.GetLoadedItemCount())
            return;
        const bool archive = state_ == State::Viewing;
        const bool streams = panel.IsAltStreamsFolder();
        if (state_ != State::Filesystem && !archive && !streams)
            return;
        const QString path = ToQString(archive ? panel.GetItemRelPath2(edit.SourceIndex)
                                               : panel.GetItemFullPath(edit.SourceIndex));
        if (path != edit.Path)
            return;
        // Native callbacks address their sending panel directly. Keep the Qt
        // projection on that same owner through modal progress/focus restoration.
        QScopedValueRollback<unsigned> projection(panelProjectionDepth_, panelProjectionDepth_ + 1);
        CPanel::CDisableNotify peer(app_.Panels[1 - edit.Panel]);
        if (name == (archive ? path.section('/', -1) : QFileInfo(path).fileName())) {
            if (archive && flatView_->isChecked()) {
                renderBoundFolder();
                selectPaths({path});
            }
            return;
        }
        finishFilesystemRename(edit.SourceIndex, path, name);
    });
}

void MainWindow::finishFilesystemRename(quint32 sourceIndex, const QString &originalPath,
                                        const QString &name) {
    const bool archive = state_ == State::Viewing;
    const auto &panel = app_.GetFocusedPanel();
    const bool streams = panel.GetFolder() && panel.IsAltStreamsFolder();
    if (state_ != State::Filesystem && !archive && !streams)
        return;
    if (!archive && !streams && QFileInfo(originalPath).absolutePath() != directory_ &&
        !panel.GetFlatMode())
        return;
    {
        QScopedValueRollback<bool> source(sourceArchiveProgress_, true);
        app_.GetFocusedPanel().OnEndLabelEdit(sourceIndex, ToUString(name));
    }
    const auto selection = app_.GetFocusedPanel().TakeRefreshSelection();
    if (!selection) {
        // The source validation return rejects the edit without reloading.
        rejectLabelEdit(app_.GetFocusedPanel(), *table_, sourceIndex);
        refreshActions();
        return;
    }
    // The source worker owns operation errors. Its posted reload ignores the
    // publication result and must not add a second rename diagnostic.
    if (renderBoundFolder() == S_OK)
        restoreListSelection(*selection);
}
