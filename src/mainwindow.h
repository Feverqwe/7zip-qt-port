#pragma once

#include "archivebackend.h"

#include <QAction>
#include <QFileDialog>
#include <QFileInfo>
#include <QHeaderView>
#include <QKeySequence>
#include <QLabel>
#include <QMainWindow>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
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
        path_ = new QLabel("Откройте архив .7z или .zip");
        path_->setObjectName("archivePath");
        path_->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
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
        connect(backend_, &ArchiveBackend::progress, this, [this](int percent, const QString &) {
            if (progress_)
                progress_->setValue(percent);
        });
        connect(backend_, &ArchiveBackend::finished, this, [this](bool ok, const QString &message) {
            if (progress_) {
                QProgressDialog *completed = progress_;
                progress_ = nullptr;
                completed->disconnect(backend_);
                completed->close();
                completed->deleteLater();
            }
            if (state_ == State::Loading && !ok)
                setState(State::Error);
            else if (state_ == State::Operating)
                setState(previousState_ == State::Viewing ? State::Viewing
                                                          : (ok ? State::NoArchive : State::Error));
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
        entries_.clear();
        table_->setRowCount(0);
        path_->setText(archive_);
        setState(State::Loading);
        begin("Открытие архива");
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
        begin("Извлечение");
        backend_->extract(archive_, destination, names);
    }

  private:
    enum class State { NoArchive, Loading, Viewing, Operating, Error };
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
    }
    void begin(const QString &label) {
        progress_ = new QProgressDialog(label, "Отмена", 0, 100, this);
        progress_->setWindowModality(Qt::WindowModal);
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
        begin("Создание архива");
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
    State state_ = State::NoArchive;
    State previousState_ = State::NoArchive;
    QList<ArchiveEntry> entries_;
    QLabel *path_ = nullptr;
    QTableWidget *table_ = nullptr;
    QProgressDialog *progress_ = nullptr;
    QAction *open_ = nullptr;
    QAction *add_ = nullptr;
    QAction *extract_ = nullptr;
    QAction *selected_ = nullptr;
};
