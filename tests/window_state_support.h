#pragma once

#include "mainwindow.h"

#include <QAbstractButton>
#include <QAction>
#include <QApplication>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QDir>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QImage>
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMenu>
#include <QMessageBox>
#include <QMimeData>
#include <QPlainTextEdit>
#include <QProgressDialog>
#include <QPushButton>
#include <QScopedPointer>
#include <QSettings>
#include <QSpinBox>
#include <QStatusBar>
#include <QSysInfo>
#include <QTableWidget>
#include <QTemporaryDir>
#include <QTimer>
#include <QUrl>
#include <QtTest>

class ControlledBackend : public ArchiveBackend {
  public:
    ControlledBackend() : ArchiveBackend(nullptr, "/missing/7zz") {}
    bool busy() const override { return busy_; }
    void list(const QString &archive, const QString & = {}) override {
        ++listCalls;
        lastArchive = archive;
        busy_ = true;
    }
    void extract(const QString &, const QString &, const QStringList &selected,
                 const QString &password = {}, const ExtractionOptions &options = {}) override {
        ++extractCalls;
        lastSelected = selected;
        lastExtractPassword = password;
        lastExtractionOptions = options;
        busy_ = true;
    }
    void test(const QString &, const QString &password = {}) override {
        ++testCalls;
        lastTestPassword = password;
        busy_ = true;
    }
    void create(const QString &target, const QStringList &sources, const QString & = {},
                bool = false, const CompressionOptions &options = {}) override {
        ++createCalls;
        lastCreateTarget = target;
        lastCreateSources = sources;
        lastCompressionOptions = options;
        busy_ = true;
    }
    void add(const QString &target, const QStringList &sources) override {
        ++addCalls;
        lastAddTarget = target;
        lastAddSources = sources;
        busy_ = true;
    }
    void remove(const QString &, const QStringList &paths) override {
        ++removeCalls;
        lastRemoved = paths;
        busy_ = true;
    }
    void renameEntry(const QString &, const QString &oldPath, const QString &newPath) override {
        ++renameCalls;
        lastRenameOld = oldPath;
        lastRenameNew = newPath;
        busy_ = true;
    }
    void cancel() override {
        ++cancelCalls;
        busy_ = false;
        emit finished(false, QStringLiteral("Операция отменена."));
    }
    void completeList(const QList<ArchiveEntry> &entries) {
        busy_ = false;
        emit entriesReady(entries);
        emit finished(true, QStringLiteral("Архив открыт."));
    }
    void fail(const QString &message) {
        busy_ = false;
        emit finished(false, message);
    }
    void completeCreate() {
        busy_ = false;
        emit finished(true, QStringLiteral("Архив создан."));
    }
    void completeAdd() {
        busy_ = false;
        emit finished(true, QStringLiteral("Файлы добавлены."));
    }
    void report(int percent, const QString &detail, const QString &line) {
        emit outputReceived(line);
        emit progress(percent, detail);
    }
    int listCalls = 0;
    int extractCalls = 0;
    int testCalls = 0;
    int createCalls = 0;
    int addCalls = 0;
    int removeCalls = 0;
    int renameCalls = 0;
    int cancelCalls = 0;
    CompressionOptions lastCompressionOptions;
    QString lastArchive;
    QStringList lastSelected;
    QString lastExtractPassword;
    ExtractionOptions lastExtractionOptions;
    QString lastTestPassword;
    QString lastCreateTarget;
    QStringList lastCreateSources;
    QString lastAddTarget;
    QStringList lastAddSources;
    QStringList lastRemoved;
    QString lastRenameOld;
    QString lastRenameNew;

  private:
    bool busy_ = false;
};

class MainWindowTestSupport {
  protected:
    static QAction *action(MainWindow &window, const char *name) {
        auto *result = window.findChild<QAction *>(name);
        Q_ASSERT(result);
        return result;
    }
    static QTableWidget *table(MainWindow &window) {
        auto *result = window.findChild<QTableWidget *>("archiveTable");
        Q_ASSERT(result);
        return result;
    }
    static int rowWithPath(MainWindow &window, const QString &path) {
        for (int row = 0; row < table(window)->rowCount(); ++row) {
            if (table(window)->item(row, 0)->data(Qt::UserRole).toString() == path)
                return row;
        }
        return -1;
    }
    static void openRow(MainWindow &window, int row) {
        QVERIFY(row >= 0);
        QVERIFY(QMetaObject::invokeMethod(table(window), "cellDoubleClicked", Qt::DirectConnection,
                                          Q_ARG(int, row), Q_ARG(int, 0)));
    }
    static void dismissWarnings() {
        for (QWidget *widget : QApplication::topLevelWidgets()) {
            if (auto *box = qobject_cast<QMessageBox *>(widget))
                box->accept();
        }
    }
    static void pressSelectAll(QTableWidget *view) {
        const QKeyCombination key = QKeySequence(QKeySequence::SelectAll)[0];
        QTest::keyClick(view, key.key(), key.keyboardModifiers());
    }
    static int filesystemRow(MainWindow &window, const QString &name) {
        for (int row = 0; row < table(window)->rowCount(); ++row) {
            if (table(window)->item(row, 0)->text() == name)
                return row;
        }
        return -1;
    }

  protected:
};
