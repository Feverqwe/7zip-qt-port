#pragma once

#include "archivebackend.h"

#include <QAction>
#include <QDialog>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QHeaderView>
#include <QKeySequence>
#include <QLineEdit>
#include <QMainWindow>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QProgressDialog>
#include <QSizePolicy>
#include <QStatusBar>
#include <QTableWidget>
#include <QToolBar>
#include <QVBoxLayout>
#include <QWidget>

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
        auto *open = menu->addAction("Открыть…", this, [this] { openDialog(); });
        open->setObjectName("openArchiveAction");
        open->setShortcut(QKeySequence::Open);
        add_ = toolbar->addAction("Добавить");
        add_->setObjectName("addArchiveAction");
        auto *addMenu = new QMenu(this);
        addMenu->addAction("Файлы…", this, [this] { addFiles(); });
        addMenu->addAction("Папку…", this, [this] { addFolder(); });
        add_->setMenu(addMenu);
        connect(add_, &QAction::triggered, this, [this] { addFiles(); });
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

        connect(backend_, &ArchiveBackend::entriesReady, this,
                [this](const QList<ArchiveEntry> &entries) {
                    if (state_ != State::Loading)
                        return;
                    entries_ = entries;
                    table_->setRowCount(entries.size());
                    for (int row = 0; row < entries.size(); ++row) {
                        const auto &entry = entries[row];
                        const QStringList values = {QFileInfo(entry.path).fileName(), entry.path,
                                                    QString::number(entry.size),
                                                    QString::number(entry.packedSize),
                                                    entry.modified};
                        for (int col = 0; col < values.size(); ++col)
                            table_->setItem(row, col, new QTableWidgetItem(values[col]));
                    }
                    setState(State::Viewing);
                    statusBar()->showMessage(QStringLiteral("%1 объектов").arg(entries.size()));
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
            else if (state_ == State::Operating)
                setState(previousState_ == State::Viewing ? State::Viewing : State::Filesystem);
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
            for (const QModelIndex &index : table_->selectionModel()->selectedRows())
                names << entries_[index.row()].path;
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
    void showDirectory(const QString &path, bool remember) {
        const QFileInfo info(path);
        if (!info.isDir()) {
            statusBar()->showMessage("Папка не найдена: " + path);
            path_->setText(state_ == State::Viewing ? archive_ : directory_);
            return;
        }
        const QString next = info.canonicalFilePath();
        if (remember && !directory_.isEmpty() && directory_ != next)
            history_.append(directory_);
        directory_ = next;
        archive_.clear();
        entries_.clear();
        table_->clearSelection();
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
            for (int col = 0; col < values.size(); ++col)
                table_->setItem(row, col, new QTableWidgetItem(values[col]));
        }
        setState(State::Filesystem);
        statusBar()->showMessage(QStringLiteral("%1 объектов").arg(files.size()));
    }
    void navigateTo(const QString &path) {
        if (state_ == State::Loading || state_ == State::Operating)
            return;
        const QFileInfo info(path);
        if (info.isDir())
            showDirectory(path, true);
        else if (info.isFile() && (info.suffix().compare("7z", Qt::CaseInsensitive) == 0 ||
                                   info.suffix().compare("zip", Qt::CaseInsensitive) == 0))
            openArchive(path);
        else {
            statusBar()->showMessage("Путь не найден или тип файла не поддерживается: " + path);
            path_->setText(state_ == State::Viewing ? archive_ : directory_);
        }
    }
    void activateRow(int row) {
        if (state_ != State::Filesystem || row < 0 || row >= table_->rowCount())
            return;
        navigateTo(table_->item(row, 1)->text());
    }
    void goBack() {
        if (state_ == State::Loading || state_ == State::Operating)
            return;
        if (state_ == State::Viewing) {
            showDirectory(returnDirectory_, false);
        } else if (!history_.isEmpty()) {
            showDirectory(history_.takeLast(), false);
        }
    }
    void setState(State state) {
        state_ = state;
        refreshActions();
    }
    void refreshActions() {
        const bool idle = state_ != State::Loading && state_ != State::Operating;
        open_->setEnabled(idle);
        add_->setEnabled(idle);
        extract_->setEnabled(idle && state_ == State::Viewing && !entries_.isEmpty());
        selected_->setEnabled(idle && state_ == State::Viewing && !entries_.isEmpty() &&
                              !table_->selectionModel()->selectedRows().isEmpty());
        back_->setEnabled(idle && (state_ == State::Viewing || !history_.isEmpty()));
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
    void createArchive(const QStringList &sources) {
        if (state_ == State::Loading || state_ == State::Operating || sources.isEmpty())
            return;
        const QString target =
            QFileDialog::getSaveFileName(this, "Создать архив", {}, "7z (*.7z);;ZIP (*.zip)");
        if (target.isEmpty())
            return;
        previousState_ = state_;
        setState(State::Operating);
        begin("Создание архива", target);
        backend_->create(target, sources);
    }
    void addFiles() { createArchive(QFileDialog::getOpenFileNames(this, "Добавить файлы")); }
    void addFolder() {
        const QString folder = QFileDialog::getExistingDirectory(this, "Добавить папку");
        if (!folder.isEmpty())
            createArchive({folder});
    }

    ArchiveBackend *backend_ = nullptr;
    QString archive_;
    State state_ = State::Filesystem;
    State previousState_ = State::Filesystem;
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
    QAction *add_ = nullptr;
    QAction *extract_ = nullptr;
    QAction *selected_ = nullptr;
    QAction *back_ = nullptr;
};
