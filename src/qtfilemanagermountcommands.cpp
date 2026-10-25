#include "CPP/7zip/UI/Agent/Agent.h"
#include "mainwindow.h"
#include "qtarchivefileresource.h"
#include "qtarchivemountmanager.h"
#include "qtfilemanagercommands.h"
#include "qtlangcontrols.h"
#include <QAction>
#include <QMenu>

void QtFileManagerCommands::AttachMountActions() {
    auto *menu = window_.findChild<QMenu *>("toolsMenu");
    auto *mount = menu->addAction(QtArchiveMountManager::tr("Mount read-only…"));
    mount->setObjectName("mountReadOnlyAction");
    QtBindLocalText(mount, "QtArchiveMountManager", "Mount read-only…");
    Bind(mount, MountReadOnly);
    auto *manage = menu->addAction(QtArchiveMountManager::tr("Read-only mounts…"));
    manage->setObjectName("manageMountsAction");
    QtBindLocalText(manage, "QtArchiveMountManager", "Read-only mounts…");
    Bind(manage, ManageMounts);
}
void QtFileManagerCommands::MountReadOnlyContainer() {
    auto *folder = dynamic_cast<CAgentFolder *>(window_.app_.GetFocusedPanel().GetFolder());
    auto *resource =
        folder ? dynamic_cast<QtArchiveFileResource *>(folder->GetEngineResource()) : nullptr;
    if (!resource || !resource->NativeReaderLease())
        return;
    if (!mounts_)
        mounts_ = new QtArchiveMountManager(&window_);
    const QString path = ToQString(GetFolderPath(folder));
    const QString label = path.isEmpty() ? QStringLiteral("/") : path;
    mounts_->mount(resource, label, window_.archivePassword_, path);
}
void QtFileManagerCommands::ShowMounts() {
    if (!mounts_)
        mounts_ = new QtArchiveMountManager(&window_);
    mounts_->show();
    mounts_->raise();
    mounts_->activateWindow();
}
bool QtFileManagerCommands::CanCloseMounts() { return !mounts_ || mounts_->canCloseOwner(); }
