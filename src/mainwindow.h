#pragma once

#include "archivebackend.h"
#include "createarchivedialog.h"

#include <QAction>
#include <QApplication>
#include <QDialog>
#include <QDir>
#include <QEvent>
#include <QFileDialog>
#include <QFileInfo>
#include <QHeaderView>
#include <QKeyEvent>
#include <QKeySequence>
#include <QLineEdit>
#include <QMainWindow>
#include <QMap>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QProgressDialog>
#include <QSizePolicy>
#include <QStatusBar>
#include <QStyle>
#include <QTableWidget>
#include <QToolBar>
#include <QVBoxLayout>
#include <QWidget>

class ArchiveTableItem : public QTableWidgetItem {
  public:
    using QTableWidgetItem::QTableWidgetItem;
    bool operator<(const QTableWidgetItem &other) const override {
        if (data(Qt::UserRole + 1).toBool() != other.data(Qt::UserRole + 1).toBool())
            return data(Qt::UserRole + 1).toBool();
        if (data(Qt::UserRole + 2).isValid() && other.data(Qt::UserRole + 2).isValid()) {
            const qint64 left = data(Qt::UserRole + 2).toLongLong();
            const qint64 right = other.data(Qt::UserRole + 2).toLongLong();
            if (left != right)
                return left < right;
        }
        return QString::localeAwareCompare(text(), other.text()) < 0;
    }
};

class MainWindow : public QMainWindow {
  public:
    explicit MainWindow(ArchiveBackend *backend = nullptr) {
        backend_ = backend ? backend : new ArchiveBackend(this);
        setWindowTitle("Sevenzip Desktop");
        resize(980, 620);
        auto *menu = menuBar()->addMenu("Файл");
        auto *toolbar = addToolBar("Архив");
        toolbar->setObjectName("archiveToolbar");
        toolbar->setMovable(false);
        back_ = toolbar->addAction("Назад", this, [this] { goBack(); });
        back_->setObjectName("backAction");
        up_ = toolbar->addAction("Вверх", this, [this] { goUp(); });
        up_->setObjectName("upAction");
        auto *open = menu->addAction("Открыть…", this, [this] { openDialog(); });
        open->setObjectName("openArchiveAction");
        open->setShortcut(QKeySequence::Open);
        create_ = toolbar->addAction("Создать…", this, [this] { createDialog(); });
        create_->setObjectName("createArchiveAction");
        auto *addExisting = menu->addAction("Добавить в открытый архив…");
        addExisting->setObjectName("addArchiveAction");
        addExisting->setEnabled(false);
        extract_ = toolbar->addAction("Извлечь", this, [this] { extract(false); });
        extract_->setObjectName("extractArchiveAction");
        selected_ = menu->addAction("Извлечь выбранное…", this, [this] { extract(true); });
        selected_->setObjectName("extractSelectedAction");
        journalAction_ =
            menu->addAction("Журнал последней операции…", this, [this] { showJournal(); });
        journalAction_->setObjectName("operationJournalAction");
        journalAction_->setEnabled(false);
        toolbar->addSeparator();
        for (const QString &title : {"Тест", "Копировать", "Переместить", "Удалить", "Информация"})
            toolbar->addAction(title)->setEnabled(false);
        for (const QString &title :
             {"Пароль", "Изменить архив", "Интеграция с файловым менеджером"})
            menu->addAction(title)->setEnabled(false);
        toolbar->addSeparator();
        toolbar->addAction(open);
        open_ = open;

        auto *central = new QWidget(this);
        auto *layout = new QVBoxLayout(central);
        layout->setContentsMargins(4, 4, 4, 4);
        path_ = new QLineEdit;
        path_->setObjectName("archivePath");
        path_->setPlaceholderText("Путь к папке или архиву");
        layout->addWidget(path_);
        table_ = new QTableWidget(0, 5, central);
        table_->setObjectName("archiveTable");
        table_->setHorizontalHeaderLabels({"Имя", "Путь", "Размер", "Сжатый", "Изменён"});
        table_->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
        table_->setSelectionBehavior(QAbstractItemView::SelectRows);
        table_->setSelectionMode(QAbstractItemView::ExtendedSelection);
        table_->setEditTriggers(QAbstractItemView::NoEditTriggers);
        table_->setSortingEnabled(true);
        table_->sortItems(0, Qt::AscendingOrder);
        table_->installEventFilter(this);
        table_->viewport()->installEventFilter(this);
        openItem_ = new QAction(table_);
        openItem_->setObjectName("openItemAction");
        table_->addAction(openItem_);
        connect(openItem_, &QAction::triggered, this,
                [this] { activateRow(table_->currentRow()); });
        parent_ = new QAction(table_);
        parent_->setObjectName("parentFolderAction");
        table_->addAction(parent_);
        connect(parent_, &QAction::triggered, this, [this] { goParent(); });
        selectAll_ = new QAction(table_);
        selectAll_->setObjectName("selectAllAction");
        selectAll_->setShortcut(QKeySequence::SelectAll);
        selectAll_->setShortcutContext(Qt::WidgetShortcut);
        table_->addAction(selectAll_);
        connect(selectAll_, &QAction::triggered, table_, &QTableWidget::selectAll);
        layout->addWidget(table_);
        setCentralWidget(central);
        statusBar()->showMessage("Готово");
        connect(path_, &QLineEdit::returnPressed, this, [this] { navigateTo(path_->text()); });
        connect(table_, &QTableWidget::cellDoubleClicked, this,
                [this](int row, int) { activateRow(row); });
        showDirectory(QDir::currentPath(), false);
        refreshActions();
        connect(table_->selectionModel(), &QItemSelectionModel::selectionChanged, this,
                [this] { refreshActions(); });
        connect(table_, &QTableWidget::currentCellChanged, this, [this] { refreshActions(); });

        connect(backend_, &ArchiveBackend::entriesReady, this,
                [this](const QList<ArchiveEntry> &entries) {
                    if (state_ != State::Loading)
                        return;
                    entries_ = entries;
                    showArchiveFolder({});
                    setState(State::Viewing);
                });
        connect(backend_, &ArchiveBackend::outputReceived, this,
                [this](const QString &line) { appendJournal(line); });
        connect(backend_, &ArchiveBackend::progress, this,
                [this](int percent, const QString &detail) {
                    if (!progress_)
                        return;
                    if (percent >= 0)
                        progress_->setValue(percent);
                    if (!detail.isEmpty()) {
                        currentDetail_ = detail;
                        progress_->setLabelText(operationName_ + "\n" + currentDetail_);
                        statusBar()->showMessage(operationName_ + ": " + currentDetail_);
                    }
                });
        connect(backend_, &ArchiveBackend::finished, this, [this](bool ok, const QString &message) {
            appendJournal((ok ? QStringLiteral("Результат: ") : QStringLiteral("Ошибка: ")) +
                          message);
            if (progress_) {
                QProgressDialog *completed = progress_;
                progress_ = nullptr;
                completed->disconnect(backend_);
                completed->close();
                completed->deleteLater();
            }
            if (state_ == State::Loading && !ok)
                showDirectory(returnDirectory_, false);
            else if (state_ == State::Operating) {
                if (creating_)
                    showDirectory(directory_, false);
                else
                    setState(previousState_ == State::Viewing ? State::Viewing : State::Filesystem);
                creating_ = false;
            }
            statusBar()->showMessage(message);
            if (!ok && message != "Операция отменена.")
                QMessageBox::warning(this, "Sevenzip Desktop", message);
        });
    }

    void openArchive(const QString &path) {
        if (state_ == State::Loading || state_ == State::Operating)
            return;
        const QString suffix = QFileInfo(path).suffix().toLower();
        if (suffix != "7z" && suffix != "zip") {
            QMessageBox::warning(this, "Sevenzip Desktop",
                                 "Поддерживаются только архивы .7z и .zip.");
            return;
        }
        archive_ = QFileInfo(path).absoluteFilePath();
        if (state_ == State::Filesystem)
            returnDirectory_ = directory_;
        else if (returnDirectory_.isEmpty())
            returnDirectory_ = QFileInfo(archive_).absolutePath();
        entries_.clear();
        archiveFolder_.clear();
        table_->setRowCount(0);
        path_->setText(archive_);
        setState(State::Loading);
        begin("Открытие архива", archive_);
        backend_->list(archive_);
    }

    void extractTo(const QString &destination, bool onlySelected = false) {
        if (state_ != State::Viewing || entries_.isEmpty() || destination.isEmpty())
            return;
        QStringList names;
        if (onlySelected) {
            for (const QModelIndex &index : table_->selectionModel()->selectedRows()) {
                const auto *item = table_->item(index.row(), 0);
                const QString path = item->data(Qt::UserRole).toString();
                if (item->data(Qt::UserRole + 1).toBool()) {
                    for (const ArchiveEntry &entry : entries_) {
                        QString normalized = entry.path;
                        if (normalized.endsWith('/'))
                            normalized.chop(1);
                        if (normalized == path || normalized.startsWith(path + '/'))
                            names << entry.path;
                    }
                } else
                    names << path;
            }
            names.removeDuplicates();
            if (names.isEmpty())
                return;
        }
        previousState_ = state_;
        setState(State::Operating);
        begin("Извлечение", destination);
        backend_->extract(archive_, destination, names);
    }

  private:
    enum class State { Filesystem, Loading, Viewing, Operating };
    bool eventFilter(QObject *watched, QEvent *event) override {
        if ((watched == table_ || watched == table_->viewport()) &&
            event->type() == QEvent::KeyPress) {
            auto *key = static_cast<QKeyEvent *>(event);
            if (!(key->modifiers() &
                  (Qt::ControlModifier | Qt::MetaModifier | Qt::AltModifier | Qt::ShiftModifier))) {
                if ((key->key() == Qt::Key_Return || key->key() == Qt::Key_Enter) &&
                    openItem_->isEnabled()) {
                    openItem_->trigger();
                    return true;
                }
                if (key->key() == Qt::Key_Backspace && parent_->isEnabled()) {
                    parent_->trigger();
                    return true;
                }
            }
        }
        return QMainWindow::eventFilter(watched, event);
    }
    void showDirectory(const QString &path, bool remember) {
        const QFileInfo info(path);
        if (!info.isDir()) {
            statusBar()->showMessage("Папка не найдена: " + path);
            path_->setText(state_ == State::Viewing ? archiveDisplayPath() : directory_);
            return;
        }
        const QString next = info.canonicalFilePath();
        if (remember && !directory_.isEmpty() && directory_ != next)
            history_.append(directory_);
        directory_ = next;
        archive_.clear();
        archiveFolder_.clear();
        entries_.clear();
        table_->clearSelection();
        table_->setSortingEnabled(false);
        table_->setRowCount(0);
        path_->setText(directory_);
        const QFileInfoList files =
            QDir(directory_)
                .entryInfoList(QDir::AllEntries | QDir::NoDotAndDotDot | QDir::Hidden |
                                   QDir::System,
                               QDir::DirsFirst | QDir::Name | QDir::IgnoreCase);
        for (const QFileInfo &file : files) {
            const int row = table_->rowCount();
            table_->insertRow(row);
            const QStringList values = {file.fileName(), file.absoluteFilePath(),
                                        file.isDir() ? QString() : QString::number(file.size()),
                                        QString(), file.lastModified().toString(Qt::ISODate)};
            for (int col = 0; col < values.size(); ++col) {
                auto *item = new ArchiveTableItem(values[col]);
                item->setData(Qt::UserRole + 1, file.isDir());
                if (col == 0)
                    item->setIcon(QApplication::style()->standardIcon(
                        file.isDir() ? QStyle::SP_DirIcon : QStyle::SP_FileIcon));
                if (col == 2 && !file.isDir())
                    item->setData(Qt::UserRole + 2, file.size());
                table_->setItem(row, col, item);
            }
        }
        table_->setSortingEnabled(true);
        setState(State::Filesystem);
        statusBar()->showMessage(QStringLiteral("%1 объектов").arg(files.size()));
    }
    void navigateTo(const QString &path) {
        if (state_ == State::Loading || state_ == State::Operating)
            return;
        if (state_ == State::Viewing && path == archiveDisplayPath())
            return;
        const QFileInfo info(path);
        if (info.isDir())
            showDirectory(path, true);
        else if (info.isFile() && (info.suffix().compare("7z", Qt::CaseInsensitive) == 0 ||
                                   info.suffix().compare("zip", Qt::CaseInsensitive) == 0))
            openArchive(path);
        else {
            statusBar()->showMessage("Путь не найден или тип файла не поддерживается: " + path);
            path_->setText(state_ == State::Viewing ? archiveDisplayPath() : directory_);
        }
    }
    void activateRow(int row) {
        if (state_ == State::Loading || state_ == State::Operating)
            return;
        if (row < 0 || row >= table_->rowCount())
            return;
        if (state_ == State::Filesystem)
            navigateTo(table_->item(row, 1)->text());
        else if (state_ == State::Viewing && table_->item(row, 0)->data(Qt::UserRole + 1).toBool())
            showArchiveFolder(table_->item(row, 0)->data(Qt::UserRole).toString());
    }
    void goBack() {
        if (state_ == State::Loading || state_ == State::Operating)
            return;
        if (state_ == State::Viewing) {
            if (archiveFolder_.isEmpty())
                showDirectory(returnDirectory_, false);
            else
                goUp();
        } else if (!history_.isEmpty()) {
            showDirectory(history_.takeLast(), false);
        }
    }
    void goUp() {
        if (state_ != State::Viewing || archiveFolder_.isEmpty())
            return;
        const int separator = archiveFolder_.lastIndexOf('/');
        showArchiveFolder(separator < 0 ? QString() : archiveFolder_.left(separator));
    }
    void goParent() {
        if (state_ == State::Viewing) {
            goBack();
        } else if (state_ == State::Filesystem) {
            QDir parent(directory_);
            if (parent.cdUp())
                showDirectory(parent.absolutePath(), true);
        }
    }
    QString archiveDisplayPath() const {
        return archive_ +
               (archiveFolder_.isEmpty() ? QString() : QStringLiteral(" / ") + archiveFolder_);
    }
    void showArchiveFolder(const QString &folder) {
        archiveFolder_ = folder;
        QMap<QString, ArchiveEntry> children;
        const QString prefix = folder.isEmpty() ? QString() : folder + '/';
        for (const ArchiveEntry &entry : entries_) {
            QString path = entry.path;
            if (path.endsWith('/'))
                path.chop(1);
            if (!path.startsWith(prefix) || path == folder)
                continue;
            const QString relative = path.mid(prefix.size());
            const int slash = relative.indexOf('/');
            const QString name = slash < 0 ? relative : relative.left(slash);
            if (name.isEmpty())
                continue;
            const QString childPath = prefix + name;
            if (slash >= 0) {
                if (!children.contains(name)) {
                    ArchiveEntry implicit;
                    implicit.path = childPath;
                    implicit.directory = true;
                    children.insert(name, implicit);
                }
            } else {
                ArchiveEntry visible = entry;
                visible.path = childPath;
                children.insert(name, visible);
            }
        }
        table_->setSortingEnabled(false);
        table_->clearSelection();
        table_->setRowCount(0);
        for (auto it = children.cbegin(); it != children.cend(); ++it) {
            const ArchiveEntry &entry = it.value();
            const int row = table_->rowCount();
            table_->insertRow(row);
            const QStringList values = {
                it.key(), entry.path, entry.directory ? QString() : QString::number(entry.size),
                entry.directory ? QString() : QString::number(entry.packedSize), entry.modified};
            for (int col = 0; col < values.size(); ++col) {
                auto *item = new ArchiveTableItem(values[col]);
                item->setData(Qt::UserRole, entry.path);
                item->setData(Qt::UserRole + 1, entry.directory);
                item->setToolTip(entry.path);
                if (col == 0)
                    item->setIcon(QApplication::style()->standardIcon(
                        entry.directory ? QStyle::SP_DirIcon : QStyle::SP_FileIcon));
                if (!entry.directory && (col == 2 || col == 3))
                    item->setData(Qt::UserRole + 2, col == 2 ? entry.size : entry.packedSize);
                table_->setItem(row, col, item);
            }
        }
        table_->setSortingEnabled(true);
        path_->setText(archiveDisplayPath());
        statusBar()->showMessage(QStringLiteral("%1 объектов").arg(children.size()));
        refreshActions();
    }
    void setState(State state) {
        state_ = state;
        refreshActions();
    }
    void refreshActions() {
        const bool idle = state_ != State::Loading && state_ != State::Operating;
        open_->setEnabled(idle);
        create_->setEnabled(idle && state_ == State::Filesystem);
        extract_->setEnabled(idle && state_ == State::Viewing && !entries_.isEmpty());
        selected_->setEnabled(idle && state_ == State::Viewing && !entries_.isEmpty() &&
                              !table_->selectionModel()->selectedRows().isEmpty());
        back_->setEnabled(idle && (state_ == State::Viewing || !history_.isEmpty()));
        up_->setEnabled(idle && state_ == State::Viewing && !archiveFolder_.isEmpty());
        bool canOpenItem = false;
        const int row = table_->currentRow();
        if (idle && row >= 0 && row < table_->rowCount()) {
            if (state_ == State::Viewing)
                canOpenItem = table_->item(row, 0)->data(Qt::UserRole + 1).toBool();
            else {
                const QFileInfo info(table_->item(row, 1)->text());
                canOpenItem =
                    info.isDir() ||
                    (info.isFile() && (info.suffix().compare("7z", Qt::CaseInsensitive) == 0 ||
                                       info.suffix().compare("zip", Qt::CaseInsensitive) == 0));
            }
        }
        openItem_->setEnabled(canOpenItem);
        parent_->setEnabled(idle && (state_ == State::Viewing ||
                                     (state_ == State::Filesystem && QDir(directory_).cdUp())));
        selectAll_->setEnabled(idle && table_->rowCount() > 0);
        table_->setEnabled(idle);
        path_->setReadOnly(!idle);
    }
    void appendJournal(const QString &line) {
        journalText_ += line + '\n';
        if (journalView_)
            journalView_->appendPlainText(line);
    }
    void showJournal() {
        if (!journalDialog_) {
            journalDialog_ = new QDialog(this);
            journalDialog_->setObjectName("operationJournalDialog");
            journalDialog_->setWindowTitle("Журнал последней операции");
            journalDialog_->resize(700, 440);
            auto *layout = new QVBoxLayout(journalDialog_);
            journalView_ = new QPlainTextEdit(journalDialog_);
            journalView_->setObjectName("operationJournalText");
            journalView_->setReadOnly(true);
            journalView_->setPlainText(journalText_);
            layout->addWidget(journalView_);
        }
        journalDialog_->show();
        journalDialog_->raise();
        journalDialog_->activateWindow();
    }
    void begin(const QString &label, const QString &detail) {
        operationName_ = label;
        currentDetail_ = detail;
        journalText_.clear();
        if (journalView_)
            journalView_->clear();
        appendJournal(label + ": " + detail);
        journalAction_->setEnabled(true);
        progress_ = new QProgressDialog(label + "\n" + detail, "Отмена", 0, 100, this);
        progress_->setWindowModality(Qt::NonModal);
        progress_->setMinimumDuration(0);
        progress_->setValue(0);
        connect(progress_, &QProgressDialog::canceled, backend_, &ArchiveBackend::cancel);
    }
    void openDialog() {
        const QString file =
            QFileDialog::getOpenFileName(this, "Открыть архив", {}, "Архивы (*.7z *.zip)");
        if (!file.isEmpty())
            openArchive(file);
    }
    void extract(bool onlySelected) {
        if (state_ != State::Viewing || entries_.isEmpty())
            return;
        if (onlySelected && table_->selectionModel()->selectedRows().isEmpty())
            return;
        const QString destination = QFileDialog::getExistingDirectory(this, "Папка для извлечения");
        extractTo(destination, onlySelected);
    }
    void createDialog() {
        if (state_ != State::Filesystem)
            return;
        QStringList selectedPaths;
        for (const QModelIndex &index : table_->selectionModel()->selectedRows())
            selectedPaths << table_->item(index.row(), 1)->text();
        CreateArchiveDialog dialog(directory_, selectedPaths, this);
        if (dialog.exec() != QDialog::Accepted)
            return;
        createArchive(dialog.target(), dialog.sources());
    }
    void createArchive(const QString &target, const QStringList &sources) {
        if (state_ != State::Filesystem || sources.isEmpty() || target.isEmpty())
            return;
        previousState_ = state_;
        creating_ = true;
        setState(State::Operating);
        begin("Создание архива", target);
        backend_->create(target, sources);
    }

    ArchiveBackend *backend_ = nullptr;
    QString archive_;
    QString archiveFolder_;
    State state_ = State::Filesystem;
    State previousState_ = State::Filesystem;
    bool creating_ = false;
    QList<ArchiveEntry> entries_;
    QLineEdit *path_ = nullptr;
    QString directory_;
    QString returnDirectory_;
    QStringList history_;
    QTableWidget *table_ = nullptr;
    QProgressDialog *progress_ = nullptr;
    QDialog *journalDialog_ = nullptr;
    QPlainTextEdit *journalView_ = nullptr;
    QAction *journalAction_ = nullptr;
    QString journalText_;
    QString operationName_;
    QString currentDetail_;
    QAction *open_ = nullptr;
    QAction *create_ = nullptr;
    QAction *extract_ = nullptr;
    QAction *selected_ = nullptr;
    QAction *back_ = nullptr;
    QAction *up_ = nullptr;
    QAction *openItem_ = nullptr;
    QAction *parent_ = nullptr;
    QAction *selectAll_ = nullptr;
};
