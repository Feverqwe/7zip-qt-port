#include "7zip/PropID.h"
#include "CPP/7zip/UI/FileManager/PanelSort.h"
#include "CPP/7zip/UI/FileManager/PanelSplitFile.h"
#include "Common/Defs.h"
#include "Windows/PropVariant.h"
#include "Windows/PropVariantConv.h"
#include "mainwindow.h"
#include "qtfilemanagercommands.h"
#include "qtlocationfolder.h"

void MainWindow::renderArchiveFolder() {
    auto &panel = app_.GetFocusedPanel();
    CPanel::CDisableTimerProcessing disableTimerProcessing(panel);
    CPanel::CDisableNotify disableNotify(panel);
    CMyComPtr<IFolderFolder> folderOwner = panel.GetFolder();
    if (!folderOwner)
        return;
    auto *archiveFolder = static_cast<CAgentFolder *>((IFolderFolder *)folderOwner);
    const bool flat = flatView_->isChecked();
    archiveFolder->SetFlatMode(BoolToInt(flat));
    if (archiveFolder->LoadItems() != S_OK)
        return;
    static_cast<FileTable *>(table_)->filesystem = false;
    QSettings settings(toolbarSettingsPath(), QSettings::IniFormat);
    table_->setColumnHidden(3, !settings.value("columns/3", true).toBool());
    if (!isNestedArchive())
        recordVisitedPath(archiveDisplayPath());
    const QString prefix = archiveFolder_.isEmpty() ? QString() : archiveFolder_ + '/';
    table_->setSortingEnabled(false);
    app_.GetFocusedPanel().SetNewFolder(folderOwner);
    app_.GetFocusedPanel().LoadFullPath();
    for (unsigned index = 0; index < archiveFolder->GetNumberOfItems(); ++index)
        appendArchiveEntry(*archiveFolder, index, prefix, flat);
    table_->setSortingEnabled(true);
    applySortMode();
    path_->setText(archiveDisplayPath());
    statusBar()->showMessage(QStringLiteral("%1 объектов").arg(archiveFolder->GetNumberOfItems()));
    refreshActions();
}

void MainWindow::appendArchiveEntry(CAgentFolder &folder, unsigned index, const QString &prefix,
                                    bool flat) {
    const CProxyArc &proxy = folder.GetProxy();
    const CProxyItem proxyItem = folder.GetItem(index);
    const CProxyDir &parent = proxy.Dirs[proxyItem.DirIndex];
    const bool isDir = proxyItem.Index < parent.SubDirs.Size();
    const int realIndex = proxy.GetRealIndex(proxyItem.DirIndex, proxyItem.Index);
    const unsigned childDir = isDir ? parent.SubDirs[proxyItem.Index] : 0;
    ArchiveEntry entry;
    entry.directory = isDir;
    const QString name = QString::fromWCharArray(isDir ? proxy.Dirs[childDir].Name
                                                       : proxy.Files[unsigned(realIndex)].Name);
    const QString panelPrefix =
        QDir::fromNativeSeparators(ToQString(proxy.GetDirPath_as_Prefix(proxyItem.DirIndex)));
    if (isDir) {
        entry.path = QDir::fromNativeSeparators(ToQString(proxy.GetDirPath_as_Prefix(childDir)));
        entry.path.chop(1);
    } else
        entry.path = panelPrefix + name;
    const QString displayName = flat ? panelPrefix.mid(prefix.size()) + name : name;
    const int row = table_->rowCount();
    table_->insertRow(row);
    const int sourceOrder = int(index);
    NWindows::NCOM::CPropVariant size, packedSize, modified;
    folder.GetProperty(index, kpidSize, &size);
    folder.GetProperty(index, kpidPackSize, &packedSize);
    folder.GetProperty(index, kpidMTime, &modified);
    if (modified.vt == VT_FILETIME) {
        char text[64];
        if (ConvertUtcFileTimeToString(modified.filetime, text))
            entry.modified = QString::fromLatin1(text);
    }
    const QVariant sizeValue =
        size.vt == VT_UI8 ? QVariant::fromValue(quint64(size.uhVal.QuadPart)) : QVariant();
    const QVariant packedValue = packedSize.vt == VT_UI8
                                     ? QVariant::fromValue(quint64(packedSize.uhVal.QuadPart))
                                     : QVariant();
    const QStringList values = {displayName,
                                entry.path,
                                entry.directory ? QString() : sizeValue.toString(),
                                entry.directory ? QString() : packedValue.toString(),
                                entry.modified,
                                GetExtensionPtr(QStringView(name)).toString(),
                                QString::number(sourceOrder)};
    for (int col = 0; col < values.size(); ++col) {
        auto *item = new ArchiveTableItem(values[col]);
        item->setData(Qt::UserRole, entry.path);
        item->setData(Qt::UserRole + 1, entry.directory);
        item->setData(Qt::UserRole + 3, name);
        item->setData(Qt::UserRole + 5, flat ? panelPrefix.mid(prefix.size()) : QString());
        item->setData(Qt::UserRole + 6, sourceOrder);
        if (isDir)
            item->setData(Qt::UserRole + 4, childDir);
        item->setToolTip(entry.path);
        if (col == 0)
            item->setIcon(QApplication::style()->standardIcon(
                entry.directory ? QStyle::SP_DirIcon : QStyle::SP_FileIcon));
        if (!entry.directory && (col == 2 || col == 3))
            item->setData(Qt::UserRole + 2, col == 2 ? sizeValue : packedValue);
        if (col == 6)
            item->setData(Qt::UserRole + 2, sourceOrder);
        if (col == 2 || col == 3)
            item->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        table_->setItem(row, col, item);
    }
}

void MainWindow::setState(State state) {
    const auto kind = state == State::Loading     ? QtAppOperationState::Kind::Open
                      : state == State::Operating ? QtAppOperationState::Kind::Operation
                      : scanningDisk_             ? QtAppOperationState::Kind::Scan
                                                  : QtAppOperationState::Kind::Idle;
    if (!app_.QtOperations().Begin(kind))
        return;
    state_ = state;
    refreshActions();
}

void MainWindow::refreshActions() {
    app_.RefreshTitle();
    const bool navigating = !app_.QtOperations().closed() && !app_.QtOperations().blocking();
    const bool idle = navigating && !scanningDisk_;
    refreshBookmarks();
    if (commands_)
        commands_->RefreshActions();
    table_->setEnabled(idle);
    iconList_->setEnabled(idle);
    table_->setDragEnabled(idle && state_ == State::Filesystem);
    iconList_->setDragEnabled(idle && state_ == State::Filesystem);
    path_->setReadOnly(!navigating);
}
