#include "qtfilemanagerdrag.h"
#include "7zip/UI/FileManager/resource.h"
#include "CPP/7zip/UI/FileManager/LangUtils.h"
#include "archiveprogressoperation.h"
#include "filetable.h"
#include "mainwindow.h"
#include "qtfilelisthover.h"
#include "qtlangcontrols.h"
#include <QScopedValueRollback>
#include <QStorageInfo>

QtFileManagerDrag::QtFileManagerDrag(MainWindow &window) : QObject(&window), window_(window) {
    window.app_.DropTarget().AttachPresentation(this);
    window.setAcceptDrops(true);
    window.installEventFilter(this);
    for (unsigned i = 0; i < kNumPanelsMax; ++i)
        AttachPanel(i);
}
void QtFileManagerDrag::AttachPanel(unsigned i) {
    if (revoked_ || i >= kNumPanelsMax)
        return;
    window_.app_.Panels[i].AttachDragServices(this);
    const auto &ui = window_.panelUi_[i];
    if (auto *table = dynamic_cast<FileTable *>(ui.table))
        table->dragStarter = [this, i] { StartDrag(i); };
    if (auto *icons = dynamic_cast<FileIconList *>(ui.iconList))
        icons->dragStarter = [this, i] { StartDrag(i); };
    if (ui.table)
        ui.table->viewport()->installEventFilter(this);
    if (ui.iconList)
        ui.iconList->viewport()->installEventFilter(this);
}
QtFileManagerDrag::~QtFileManagerDrag() { Revoke(); }
void QtFileManagerDrag::Revoke() {
    if (revoked_)
        return;
    revoked_ = true;
    window_.app_.DropTarget().ClearState();
    window_.app_.DropTarget().AttachPresentation(nullptr);
    window_.removeEventFilter(this);
    for (unsigned i = 0; i < kNumPanelsMax; ++i) {
        window_.app_.Panels[i].AttachDragServices(nullptr);
        const auto &ui = window_.panelUi_[i];
        if (auto *table = dynamic_cast<FileTable *>(ui.table))
            table->dragStarter = {};
        if (auto *icons = dynamic_cast<FileIconList *>(ui.iconList))
            icons->dragStarter = {};
        if (ui.table)
            ui.table->viewport()->removeEventFilter(this);
        if (ui.iconList)
            ui.iconList->viewport()->removeEventFilter(this);
    }
}
void QtFileManagerDrag::SuspendHover(bool suspended) {
    for (const auto &ui : window_.panelUi_) {
        if (ui.tableHover)
            ui.tableHover->SetSuspended(suspended);
        if (ui.iconHover)
            ui.iconHover->SetSuspended(suspended);
    }
}
void QtFileManagerDrag::RefreshPanels() {
    if (window_.app_.QtOperations().closed())
        return;
    const unsigned original = window_.projectedPanel_;
    for (unsigned i = 0; i < kNumPanelsMax; ++i) {
        if (!window_.panelUi_[i].container || !window_.app_.IsPanelVisible(i))
            continue;
        window_.activatePanelUi(i);
        window_.refreshCurrent();
    }
    window_.activatePanelUi(original);
}
void QtFileManagerDrag::StartDrag(unsigned panel, bool rightButton) {
    if (revoked_ || active_ || panel >= kNumPanelsMax || window_.backend_->busy() ||
        window_.sourceArchiveProgress_ || window_.app_.QtOperations().closed() ||
        QApplication::activeModalWidget())
        return;
    window_.activatePanelUi(panel);
    if (rightButton && !window_.app_.Panels[panel].IsFSFolder())
        return; // Native OnDrag permits right-button sources only in the filesystem.
    QScopedValueRollback<bool> active(active_, true);
    SuspendHover(true);
    {
        QScopedValueRollback<bool> source(window_.sourceArchiveProgress_, true);
        window_.app_.Panels[panel].OnDrag(rightButton);
    }
    window_.app_.DropTarget().ClearState();
    SuspendHover(false);
    RefreshPanels();
}
HRESULT QtFileManagerDrag::DoDragDrop(CDataObject &data, CDropSource &source, DWORD allowed,
                                      DWORD &effect) {
    const int i = window_.app_.DropTarget().SrcPanelIndex;
    if (i < 0 || i >= int(kNumPanelsMax))
        return E_ABORT;
    const auto &ui = window_.panelUi_[i];
    QWidget *view = ui.viewStack ? ui.viewStack->currentWidget() : ui.table;
    QtPanelDragTransport transport(view);
    return transport.DoDragDrop(data, source, allowed, effect);
}
int QtFileManagerDrag::PanelAtPoint(int x, int y) const {
    for (unsigned i = 0; i < kNumPanelsMax; ++i) {
        QWidget *container = window_.panelUi_[i].container;
        if (container && container->isVisible() &&
            container->rect().contains(container->mapFromGlobal(QPoint(x, y))))
            return int(i);
    }
    return -1;
}
bool QtFileManagerDrag::PanelEnabled(unsigned i) const {
    const auto &ui = window_.panelUi_[i];
    return ui.container && ui.container->isEnabled() && ui.table && ui.table->isEnabled();
}
int QtFileManagerDrag::FolderAtPoint(unsigned i, int x, int y, UString &name) const {
    const auto &ui = window_.panelUi_[i];
    auto *view =
        ui.viewStack ? qobject_cast<QAbstractItemView *>(ui.viewStack->currentWidget()) : nullptr;
    if (!view || !view->viewport()->rect().contains(view->viewport()->mapFromGlobal(QPoint(x, y))))
        return -1;
    const QModelIndex index =
        view->indexAt(view->viewport()->mapFromGlobal(QPoint(x, y))).siblingAtColumn(0);
    if (!index.isValid())
        return -1;
    const UInt32 real = index.data(Qt::UserRole + 6).toUInt();
    if (real == UInt32(-1) || !window_.app_.Panels[i].IsItem_Folder(real))
        return -1;
    name = window_.app_.Panels[i].GetItemName(real);
    return index.row();
}
void QtFileManagerDrag::Highlight(unsigned i, int item, bool enabled) {
    const auto &ui = window_.panelUi_[i];
    if (!ui.table || item < 0 || item >= ui.table->rowCount())
        return;
    for (int column = 0; column < ui.table->columnCount(); ++column)
        ui.table->model()->setData(ui.table->model()->index(item, column), enabled,
                                   Qt::UserRole + 19);
}
bool QtFileManagerDrag::IsSameDrive(const UString &destination,
                                    const UStringVector &sources) const {
    if (sources.IsEmpty())
        return false;
    const QStorageInfo target(ToQString(destination));
    if (!target.isValid() || !target.isReady())
        return false;
    FOR_VECTOR(i, sources) {
        const QStorageInfo source(ToQString(sources[i]));
        if (!source.isValid() || !source.isReady() || source.device() != target.device() ||
            source.rootPath() != target.rootPath())
            return false;
    }
    return true;
}
bool QtFileManagerDrag::eventFilter(QObject *object, QEvent *event) {
    const bool dragEvent = event->type() == QEvent::DragEnter ||
                           event->type() == QEvent::DragMove ||
                           event->type() == QEvent::DragLeave || event->type() == QEvent::Drop;
    if (!dragEvent) {
        if (auto *widget = qobject_cast<QWidget *>(object))
            return HandleRightDrag(*widget, *event);
        return false;
    }
    if (auto *widget = qobject_cast<QWidget *>(object))
        return HandleDropEvent(*widget, *event);
    return false;
}
CDataObject *QtFileManagerDrag::ReadData(const QMimeData &mime) {
    if (auto *internal = dynamic_cast<const QtPanelDragMime *>(&mime))
        return &internal->Data();
    external_.Names.Clear();
    for (const QUrl &url : mime.urls()) {
        if (!url.isLocalFile())
            return nullptr;
        external_.Names.Add(ToUString(url.toLocalFile()));
    }
    return external_.Names.IsEmpty() ? nullptr : &external_;
}
static DWORD DropKeys(const QDropEvent &drop) {
    DWORD keys = 0;
    if (drop.modifiers() & Qt::ControlModifier)
        keys |= MK_CONTROL;
    if (drop.modifiers() & Qt::ShiftModifier)
        keys |= MK_SHIFT;
    if (drop.modifiers() & Qt::AltModifier)
        keys |= MK_ALT;
    if (drop.buttons() & Qt::RightButton)
        keys |= MK_RBUTTON;
    return keys;
}
bool QtFileManagerDrag::HandleDropEvent(QWidget &widget, QEvent &event) {
    if (event.type() == QEvent::DragLeave) {
        window_.app_.DropTarget().DragLeave();
        SuspendHover(active_);
        return true;
    }
    auto &drop = static_cast<QDropEvent &>(event);
    const unsigned panel = window_.panelIndexFor(&widget);
    if (revoked_ || window_.app_.QtOperations().closed() || window_.backend_->busy() ||
        QApplication::activeModalWidget() || (window_.sourceArchiveProgress_ && !active_) ||
        (panel < kNumPanelsMax && !PanelEnabled(panel))) {
        window_.app_.DropTarget().ClearState();
        SuspendHover(active_);
        drop.ignore();
        return true;
    }
    CDataObject *data = ReadData(*drop.mimeData());
    if (!data) {
        window_.app_.DropTarget().ClearState();
        SuspendHover(active_);
        drop.ignore();
        return true;
    }
    const QPoint position = widget.mapToGlobal(drop.position().toPoint());
    const DWORD keys = DropKeys(drop);
    DWORD effect = 0;
    if (drop.possibleActions() & Qt::CopyAction)
        effect |= DROPEFFECT_COPY;
    if (drop.possibleActions() & Qt::MoveAction)
        effect |= DROPEFFECT_MOVE;
    HRESULT result;
    auto &target = window_.app_.DropTarget();
    SuspendHover(true);
    if (event.type() == QEvent::DragMove)
        result = target.DragOver(keys, position.x(), position.y(), &effect);
    else if (event.type() == QEvent::DragEnter)
        result = target.DragEnter(data, keys, position.x(), position.y(), &effect);
    else {
        // Qt can deliver a synthetic Drop without an accepted DragEnter, or
        // after the target became disabled. Revalidate before running a command.
        result = target.DragOver(keys, position.x(), position.y(), &effect);
        if (result == S_OK && effect) {
            QScopedValueRollback<bool> source(window_.sourceArchiveProgress_, true);
            result = target.Drop(data, keys, position.x(), position.y(), &effect);
        } else
            target.ClearState();
    }
    if (result == S_OK && effect) {
        drop.setDropAction(effect == DROPEFFECT_MOVE ? Qt::MoveAction : Qt::CopyAction);
        drop.accept();
    } else
        drop.ignore();
    if (event.type() == QEvent::Drop) {
        SuspendHover(active_);
        if (!active_ && result == S_OK && effect)
            RefreshPanels();
    }
    return true;
}
