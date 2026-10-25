#include "qtarchivemountmanager.h"
#include "qtarchivemountsession.h"
#include <QApplication>
#include <QClipboard>
#include <QDesktopServices>
#include <QDialogButtonBox>
#include <QEvent>
#include <QHeaderView>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QTreeWidget>
#include <QVBoxLayout>

QtArchiveMountManager::QtArchiveMountManager(QWidget *owner) : QDialog(owner), owner_(owner) {
    setObjectName("archiveMountManager");
    setWindowTitle(tr("Read-only mounts"));
    setModal(false);
    auto *layout = new QVBoxLayout(this);
    list_ = new QTreeWidget(this);
    list_->setObjectName("mountList");
    list_->setHeaderLabels({tr("Source"), tr("Mount path"), tr("Status")});
    list_->setRootIsDecorated(false);
    list_->setSelectionMode(QAbstractItemView::SingleSelection);
    list_->header()->setSectionResizeMode(QHeaderView::ResizeToContents);
    list_->header()->setStretchLastSection(true);
    layout->addWidget(list_, 2);
    details_ = new QPlainTextEdit(this);
    details_->setObjectName("mountDetails");
    details_->setReadOnly(true);
    details_->setAccessibleName(tr("Mount details"));
    layout->addWidget(details_, 1);
    auto *buttons = new QDialogButtonBox(this);
    open_ = buttons->addButton(tr("Open Folder"), QDialogButtonBox::ActionRole);
    copy_ = buttons->addButton(tr("Copy Path"), QDialogButtonBox::ActionRole);
    auto *disconnectButtons = new QDialogButtonBox(this);
    unmount_ = disconnectButtons->addButton(tr("Unmount"), QDialogButtonBox::ActionRole);
    force_ = disconnectButtons->addButton(tr("Force Unmount…"), QDialogButtonBox::ActionRole);
    force_->setObjectName("forceUnmountButton");
    unmount_->setObjectName("unmountButton");
    copy_->setObjectName("copyMountPathButton");
    open_->setObjectName("openMountFolderButton");
    for (auto *button : {open_, copy_, unmount_, force_})
        button->setAutoDefault(false);
    layout->addWidget(buttons);
    layout->addWidget(disconnectButtons);
    auto *footer = new QDialogButtonBox(QDialogButtonBox::Close, this);
    remove_ = footer->addButton(tr("Remove from List"), QDialogButtonBox::ActionRole);
    remove_->setObjectName("removeMountButton");
    remove_->setAutoDefault(false);
    layout->addWidget(footer);
    connect(remove_, &QPushButton::clicked, this, &QtArchiveMountManager::removeSelected);
    connect(footer, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(list_, &QTreeWidget::itemSelectionChanged, this, &QtArchiveMountManager::updateDetails);
    connect(open_, &QPushButton::clicked, this, [this] {
        if (const auto *session = selected(); session && !session->path().isEmpty())
            QDesktopServices::openUrl(QUrl::fromLocalFile(session->path()));
    });
    connect(copy_, &QPushButton::clicked, this, [this] {
        if (const auto *session = selected(); session && !session->path().isEmpty())
            QApplication::clipboard()->setText(session->path());
    });
    connect(unmount_, &QPushButton::clicked, this, [this] { unmountSelected(false); });
    connect(force_, &QPushButton::clicked, this, [this] { unmountSelected(true); });
    resize(780, 420);
    updateDetails();
}
void QtArchiveMountManager::retranslate() {
    setWindowTitle(tr("Read-only mounts"));
    list_->setHeaderLabels({tr("Source"), tr("Mount path"), tr("Status")});
    details_->setAccessibleName(tr("Mount details"));
    open_->setText(tr("Open Folder"));
    copy_->setText(tr("Copy Path"));
    unmount_->setText(tr("Unmount"));
    force_->setText(tr("Force Unmount…"));
    remove_->setText(tr("Remove from List"));
    refresh();
}
void QtArchiveMountManager::changeEvent(QEvent *event) {
    QDialog::changeEvent(event);
    if (event->type() == QEvent::LanguageChange)
        retranslate();
}
QtArchiveMountSession *QtArchiveMountManager::selected() const {
    const auto *item = list_->currentItem();
    const int row = item ? list_->indexOfTopLevelItem(const_cast<QTreeWidgetItem *>(item)) : -1;
    return row >= 0 && row < sessions_.size() ? sessions_.at(row) : nullptr;
}
void QtArchiveMountManager::updateDetails() {
    const auto *session = selected();
    const auto status = session ? session->status() : QtArchiveMountSession::Status::Stopped;
    const bool mounted = status == QtArchiveMountSession::Status::Mounted;
    open_->setEnabled(mounted && session && !session->path().isEmpty());
    copy_->setEnabled(open_->isEnabled());
    unmount_->setEnabled(session &&
                         (mounted || status == QtArchiveMountSession::Status::Preparing ||
                          status == QtArchiveMountSession::Status::Mounting));
    force_->setVisible(mounted && session && !session->error().isEmpty());
    remove_->setEnabled(session && status == QtArchiveMountSession::Status::Stopped);
    QStringList details;
    if (session) {
        details << session->label() << session->path() << tr("Read-only access");
        if (!session->limitations().isEmpty())
            details << session->limitations();
        if (!session->error().isEmpty())
            details << session->error();
        details << issues_.value(const_cast<QtArchiveMountSession *>(session));
    }
    details_->setPlainText(details.join(u'\n'));
}
void QtArchiveMountManager::unmountSelected(bool force) {
    auto *session = selected();
    if (!session)
        return;
    if (force) {
        QMessageBox prompt(QMessageBox::Warning, tr("Force unmount"),
                           tr("Disconnect this mount even if another application is using it? "
                              "Open files may stop working."),
                           QMessageBox::Cancel, this);
        auto *confirm = prompt.addButton(tr("Force Unmount"), QMessageBox::AcceptRole);
        prompt.setDefaultButton(QMessageBox::Cancel);
        prompt.exec();
        if (prompt.clickedButton() != confirm)
            return;
    }
    session->unmount(force);
}
void QtArchiveMountManager::removeSelected() {
    auto *session = selected();
    if (!session || session->status() != QtArchiveMountSession::Status::Stopped)
        return;
    const int row = sessions_.indexOf(session);
    sessions_.removeAt(row);
    issues_.remove(session);
    disconnect(session, nullptr, this, nullptr);
    delete list_->takeTopLevelItem(row);
    session->deleteLater();
    if (!sessions_.isEmpty())
        list_->setCurrentItem(list_->topLevelItem(qMin(row, int(sessions_.size()) - 1)));
    refresh();
}
void QtArchiveMountManager::showManager() {
    show();
    raise();
    activateWindow();
}
