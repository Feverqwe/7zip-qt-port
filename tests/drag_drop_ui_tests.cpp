#include "CPP/7zip/UI/FileManager/Panel.h"
#include "filetable.h"
#include "layout_inspection.h"
#include "qtownedtempregistry.h"
#include "qtpaneldragtransport.h"
#include "window_state_support.h"
#include <QContextMenuEvent>
#include <QDrag>
#include <QDragLeaveEvent>
#include <QMouseEvent>
#include <QStackedWidget>

namespace {
int Row(QTableWidget &table, const QString &name) {
    for (int row = 0; row < table.rowCount(); ++row)
        if (table.item(row, 0)->text().normalized(QString::NormalizationForm_C) ==
            name.normalized(QString::NormalizationForm_C))
            return row;
    return -1;
}
void Navigate(QWidget &panel, const QString &path) {
    auto *address = panel.findChild<QLineEdit *>("archivePath");
    address->setFocus();
    address->setText(path);
    QTest::keyClick(address, Qt::Key_Return);
}
} // namespace

class DragDropUiTests final : public QObject {
    Q_OBJECT
  private slots:
    void init() { QFile::remove(qEnvironmentVariable("SEVENZIP_SETTINGS_PATH")); }

    void archiveSourceCancelsThroughLiveController_data() {
        QTest::addColumn<bool>("icons");
        QTest::addColumn<bool>("right");
        QTest::newRow("table") << false << false;
        QTest::newRow("icons") << true << false;
        QTest::newRow("right-gesture") << false << true;
    }
    void archiveSourceCancelsThroughLiveController() {
        QFETCH(bool, icons);
        QFETCH(bool, right);
        QSettings settings(qEnvironmentVariable("SEVENZIP_SETTINGS_PATH"), QSettings::IniFormat);
        settings.setValue("FM/SingleClick", 1);
        settings.sync();
        QTemporaryDir root;
        const QString name = QStringLiteral("исходный с пробелом.txt");
        QFile input(root.filePath(name));
        QVERIFY(input.open(QIODevice::WriteOnly));
        QCOMPARE(input.write("controller bytes"), qint64(16));
        input.close();
        const QString archive = root.filePath("source.zip");
        ArchiveBackend backend;
        QSignalSpy finished(&backend, &ArchiveBackend::finished);
        backend.create(archive, {input.fileName()});
        QVERIFY(finished.wait(30000));
        QVERIFY(finished.takeFirst().first().toBool());
        QFile original(archive);
        QVERIFY(original.open(QIODevice::ReadOnly));
        const QByteArray originalBytes = original.readAll();
        original.close();
        MainWindow window(&backend);
        window.show();
        window.openArchive(archive);
        auto *table = dynamic_cast<FileTable *>(window.findChild<QTableWidget *>("archiveTable"));
        QVERIFY(table && table->dragStarter);
        QTRY_VERIFY(table->isEnabled() && Row(*table, name) >= 0);
        if (icons)
            window.findChild<QAction *>("viewMode0Action")->trigger();
        table->selectRow(Row(*table, name));
        table->setCurrentCell(Row(*table, name), 0);
        finished.clear();
        bool extracted = false, closeRefused = false, protectedActiveRoot = false;
        QStringList dragRoots;
        connect(&backend, &ArchiveBackend::engineStarted, &window, [&] {
            if (extracted)
                return;
            extracted = true;
            closeRefused = !window.close();
            QMetaObject::invokeMethod(table, "cellClicked", Qt::DirectConnection,
                                      Q_ARG(int, Row(*table, name)), Q_ARG(int, 0));
            QtOwnedTempRegistry registry;
            for (const auto &entry : registry.Entries()) {
                if (QFileInfo(entry.path).fileName().startsWith("7zE")) {
                    dragRoots.append(entry.path);
                    protectedActiveRoot |= entry.active && !registry.ClaimForDeletion(entry);
                }
            }
        });
        if (right) {
            QWidget *viewport = table->viewport();
            const QPoint point =
                table->visualRect(table->model()->index(Row(*table, name), 0)).center();
            const QPoint global = viewport->mapToGlobal(point);
            QMouseEvent press(QEvent::MouseButtonPress, point, global, Qt::RightButton,
                              Qt::RightButton, Qt::NoModifier);
            QCoreApplication::sendEvent(viewport, &press);
            QContextMenuEvent context(QContextMenuEvent::Mouse, point, global);
            QCoreApplication::sendEvent(viewport, &context);
            QVERIFY(!QApplication::activePopupWidget());
            const QPoint moved = point + QPoint(QApplication::startDragDistance() + 1, 0);
            QMouseEvent move(QEvent::MouseMove, moved, viewport->mapToGlobal(moved), Qt::NoButton,
                             Qt::RightButton, Qt::NoModifier);
            QCoreApplication::sendEvent(viewport, &move);
            QMouseEvent release(QEvent::MouseButtonRelease, moved, viewport->mapToGlobal(moved),
                                Qt::RightButton, Qt::NoButton, Qt::NoModifier);
            QCoreApplication::sendEvent(viewport, &release);
        } else if (icons) {
            auto *list = dynamic_cast<FileIconList *>(window.findChild<QListView *>("iconList"));
            QVERIFY(list && list->isVisible() && list->dragEnabled() && list->dragStarter);
            list->dragStarter();
        } else
            table->dragStarter();
        if (right) {
            QVERIFY(!extracted);
            QVERIFY(dragRoots.isEmpty());
        } else
            QVERIFY(extracted && closeRefused && protectedActiveRoot);
        QVERIFY(window.isVisible());
        QVERIFY(!backend.busy());
        QTRY_VERIFY(Row(*table, name) >= 0 && table->item(Row(*table, name), 0)->isSelected());
        for (const QString &path : dragRoots)
            QVERIFY(!QFileInfo::exists(path));
        QVERIFY(original.open(QIODevice::ReadOnly));
        QCOMPARE(original.readAll(), originalBytes);
    }

    void externalDragSuspendsHoverWithoutOpeningFolder() {
        QTemporaryDir root;
        const QString folder = QStringLiteral("назначение");
        QVERIFY(QDir().mkpath(root.filePath(folder)));
        QFile file(root.filePath("selected.txt"));
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.close();
        QSettings settings(qEnvironmentVariable("SEVENZIP_SETTINGS_PATH"), QSettings::IniFormat);
        settings.setValue("FM/SingleClick", 1);
        settings.sync();
        SavePanelPath(0, ToUString(root.path()), qEnvironmentVariable("SEVENZIP_SETTINGS_PATH"));
        ControlledBackend backend;
        MainWindow window(&backend);
        window.show();
        Navigate(*window.findChild<QWidget *>("filePanel0"), root.path());
        auto *table = window.findChild<QTableWidget *>("archiveTable");
        QTRY_VERIFY(table->isEnabled() && Row(*table, folder) >= 0 &&
                    Row(*table, "selected.txt") >= 0);
        const int selected = Row(*table, "selected.txt");
        table->selectRow(selected);
        table->setCurrentCell(selected, 0);
        const QModelIndex destination = table->model()->index(Row(*table, folder), 0);
        const QPoint point = table->visualRect(destination).center();
        QMimeData mime;
        mime.setUrls({QUrl::fromLocalFile(file.fileName())});
        QDragEnterEvent enter(point, Qt::CopyAction, &mime, Qt::LeftButton, Qt::NoModifier);
        QCoreApplication::sendEvent(table->viewport(), &enter);
        QVERIFY(enter.isAccepted());
        QVERIFY(destination.data(Qt::UserRole + 19).toBool());
        QTest::mouseMove(table->viewport(), point);
        QTest::qWait(450);
        QCOMPARE(table->currentRow(), selected);
        QDragLeaveEvent leave;
        QCoreApplication::sendEvent(table->viewport(), &leave);
        QVERIFY(!destination.data(Qt::UserRole + 19).toBool());
        QTest::mouseMove(table->viewport(), point + QPoint(1, 0));
        QTRY_COMPARE(table->currentRow(), destination.row());
        QCOMPARE(
            QFileInfo(window.findChild<QLineEdit *>("archivePath")->text()).canonicalFilePath(),
            QFileInfo(root.path()).canonicalFilePath());
    }

    void externalFilesUseArchiveConfirmation_data() {
        QTest::addColumn<bool>("icons");
        QTest::addColumn<int>("answer");
        QTest::newRow("table-yes") << false << int(QMessageBox::Yes);
        QTest::newRow("icons-yes") << true << int(QMessageBox::Yes);
        QTest::newRow("table-no") << false << int(QMessageBox::No);
        QTest::newRow("icons-cancel") << true << int(QMessageBox::Cancel);
    }
    void externalFilesUseArchiveConfirmation() {
        QFETCH(bool, icons);
        QFETCH(int, answer);
        QTemporaryDir root;
        const QString seed = root.filePath("original.txt");
        const QString source = root.filePath(QStringLiteral("добавлен с пробелом.txt"));
        for (const QString &path : {seed, source}) {
            QFile file(path);
            QVERIFY(file.open(QIODevice::WriteOnly));
            QCOMPARE(file.write("archive UI bytes"), qint64(16));
        }
        const QString archive = root.filePath("target.zip");
        ArchiveBackend backend;
        QSignalSpy created(&backend, &ArchiveBackend::finished);
        backend.create(archive, {seed});
        QVERIFY(created.wait(30000));
        QVERIFY(created.takeFirst().first().toBool());
        QFile before(archive);
        QVERIFY(before.open(QIODevice::ReadOnly));
        const QByteArray original = before.readAll();
        before.close();
        MainWindow window(&backend);
        window.show();
        window.openArchive(archive);
        auto *table = window.findChild<QTableWidget *>("archiveTable");
        QTRY_VERIFY(table->isEnabled() && Row(*table, "original.txt") >= 0);
        QVERIFY(table->dragEnabled());
        if (icons) {
            auto *action = window.findChild<QAction *>("viewMode0Action");
            QTRY_VERIFY(action->isEnabled());
            action->trigger();
        }
        auto *iconList = window.findChild<QListView *>("iconList");
        QAbstractItemView *view = icons ? static_cast<QAbstractItemView *>(iconList) : table;
        QVERIFY(view->isVisible());
        QVERIFY(view->dragEnabled());
        QMimeData mime;
        mime.setUrls({QUrl::fromLocalFile(source)});
        QDragEnterEvent enter(QPoint(5, 5), Qt::CopyAction, &mime, Qt::LeftButton, Qt::NoModifier);
        QCoreApplication::sendEvent(view->viewport(), &enter);
        QVERIFY(enter.isAccepted());
        bool questionShown = false;
        QString unexpectedError;
        QTimer respond;
        respond.setInterval(5);
        connect(&respond, &QTimer::timeout, &window, [&] {
            if (auto *question = qobject_cast<QMessageBox *>(QApplication::activeModalWidget())) {
                if (auto *button = question->button(QMessageBox::StandardButton(answer))) {
                    questionShown = true;
                    button->click();
                } else if (auto *button = question->button(QMessageBox::Ok)) {
                    unexpectedError = question->text();
                    button->click();
                }
            }
        });
        respond.start();
        QDropEvent drop(QPointF(5, 5), Qt::CopyAction, &mime, Qt::LeftButton, Qt::NoModifier);
        QCoreApplication::sendEvent(view->viewport(), &drop);
        respond.stop();
        QVERIFY2(unexpectedError.isEmpty(), qPrintable(unexpectedError));
        QVERIFY(questionShown);
        const bool success = answer == QMessageBox::Yes;
        QCOMPARE(drop.isAccepted(), success);
        QVERIFY(QFileInfo::exists(source));
        QVERIFY(!backend.busy());
        if (!success) {
            QVERIFY(before.open(QIODevice::ReadOnly));
            QCOMPARE(before.readAll(), original);
            return;
        }
        QTRY_VERIFY(Row(*table, QFileInfo(source).fileName()) >= 0);
        const QString destination = root.filePath("extracted");
        QVERIFY(QDir().mkpath(destination));
        ArchiveBackend verifier;
        QSignalSpy extracted(&verifier, &ArchiveBackend::finished);
        verifier.list(archive);
        QVERIFY(extracted.wait(30000));
        QVERIFY(extracted.takeFirst().first().toBool());
        verifier.extract(archive, destination, {QFileInfo(source).fileName()});
        QVERIFY(extracted.wait(30000));
        QVERIFY(extracted.takeFirst().first().toBool());
        QFile result(QDir(destination).filePath(QFileInfo(source).fileName()));
        QVERIFY(result.open(QIODevice::ReadOnly));
        QCOMPARE(result.readAll(), QByteArray("archive UI bytes"));
    }

    void nativeSourceUsesActualTargetView_data() {
        QTest::addColumn<bool>("icons");
        QTest::addColumn<bool>("move");
        QTest::addColumn<int>("sourceIndex");
        QTest::addColumn<bool>("right");
        QTest::addColumn<bool>("cancel");
        QTest::newRow("left-table-copy") << false << false << 0 << false << false;
        QTest::newRow("right-table-move") << false << true << 1 << false << false;
        QTest::newRow("left-icons-move") << true << true << 0 << false << false;
        QTest::newRow("right-icons-copy") << true << false << 1 << false << false;
        QTest::newRow("right-button-copy") << false << false << 0 << true << false;
        QTest::newRow("right-button-cancel") << true << false << 1 << true << true;
    }
    void nativeSourceUsesActualTargetView() {
        QFETCH(bool, icons);
        QFETCH(bool, move);
        QFETCH(int, sourceIndex);
        QFETCH(bool, right);
        QFETCH(bool, cancel);
        QTemporaryDir root;
        const QString name = QStringLiteral("перенос с пробелом.txt");
        const QString source = root.filePath("source");
        const QString target = root.filePath("target");
        const QString subfolder = QStringLiteral("назначение с пробелом");
        QVERIFY(QDir().mkpath(source));
        QVERIFY(QDir().mkpath(QDir(target).filePath(subfolder)));
        QFile input(QDir(source).filePath(name));
        QVERIFY(input.open(QIODevice::WriteOnly));
        QCOMPARE(input.write("UI drag bytes"), qint64(13));
        input.close();
        SavePanelPath(sourceIndex, ToUString(source),
                      qEnvironmentVariable("SEVENZIP_SETTINGS_PATH"));
        SavePanelPath(1 - sourceIndex, ToUString(target),
                      qEnvironmentVariable("SEVENZIP_SETTINGS_PATH"));
        ControlledBackend backend;
        MainWindow window(&backend);
        window.resize(980, 620);
        window.show();
        auto *two = window.findChild<QAction *>("twoPanelsAction");
        QTRY_VERIFY(two->isEnabled());
        if (!two->isChecked())
            two->trigger();
        auto *sourcePanel = window.findChild<QWidget *>(QString("filePanel%1").arg(sourceIndex));
        auto *targetPanel =
            window.findChild<QWidget *>(QString("filePanel%1").arg(1 - sourceIndex));
        QVERIFY(sourcePanel && targetPanel && targetPanel->isVisible());
        auto *sourceTable =
            dynamic_cast<FileTable *>(sourcePanel->findChild<QTableWidget *>("archiveTable"));
        auto *targetTable = targetPanel->findChild<QTableWidget *>("archiveTable");
        QVERIFY(sourceTable && sourceTable->dragStarter && targetTable);
        Navigate(*targetPanel, target);
        QTRY_VERIFY(targetTable->isEnabled() && Row(*targetTable, subfolder) >= 0);
        if (icons)
            window.findChild<QAction *>("viewMode0Action")->trigger();
        Navigate(*sourcePanel, source);
        QTRY_VERIFY(sourceTable->isEnabled() && Row(*sourceTable, name) >= 0);
        QVERIFY(sourceTable->sourcePanel);
        const int sourceRow = Row(*sourceTable, name);
        sourceTable->selectRow(sourceRow);
        sourceTable->setCurrentCell(sourceRow, 0);
        sourceTable->setFocus();
        QCoreApplication::processEvents();
        auto *targetIcons = targetPanel->findChild<QListView *>("iconList");
        QAbstractItemView *view =
            icons ? static_cast<QAbstractItemView *>(targetIcons) : targetTable;
        QVERIFY(view && view->isVisible());
        const int folderRow = Row(*targetTable, subfolder);
        const QModelIndex folder = targetTable->model()->index(folderRow, 0);
        const QPoint point = view->visualRect(folder).center();
        QVERIFY(view->viewport()->rect().contains(point));
        const auto selection = targetTable->selectionModel()->selectedRows();
        bool entered = false, dropped = false, highlighted = false, sourceRefused = false;
        bool menuShown = false;
        QTimer chooseMenu;
        chooseMenu.setInterval(5);
        connect(&chooseMenu, &QTimer::timeout, &window, [&] {
            for (QWidget *widget : QApplication::topLevelWidgets()) {
                auto *menu = qobject_cast<QMenu *>(widget);
                if (!menu || menu->objectName() != "dragContextMenu" || !menu->isVisible())
                    continue;
                const UInt32 command =
                    cancel ? UInt32(NDragMenu::k_Cancel)
                           : UInt32(NDragMenu::k_Copy_Base | NDragMenu::k_MenuFlag_Copy);
                for (QAction *action : menu->actions()) {
                    if (!action->isSeparator() && action->data().toUInt() == command) {
                        menuShown = true;
                        menu->setActiveAction(action);
                        QTest::keyClick(menu, Qt::Key_Return);
                        return;
                    }
                }
            }
        });
        if (right)
            chooseMenu.start();
        QtPanelDragTransport transport(sourceTable, [&](QDrag &drag, Qt::DropActions) {
            const auto modifiers = move ? Qt::ShiftModifier : Qt::ControlModifier;
            const auto button = right ? Qt::RightButton : Qt::LeftButton;
            QDragEnterEvent self(QPoint(5, 5), Qt::CopyAction | Qt::MoveAction, drag.mimeData(),
                                 button, modifiers);
            QCoreApplication::sendEvent(sourceTable->viewport(), &self);
            sourceRefused = !self.isAccepted();
            QDragLeaveEvent leave;
            QCoreApplication::sendEvent(sourceTable->viewport(), &leave);
            QDragEnterEvent enter(point, Qt::CopyAction | Qt::MoveAction, drag.mimeData(), button,
                                  modifiers);
            QCoreApplication::sendEvent(view->viewport(), &enter);
            entered = enter.isAccepted();
            highlighted = folder.data(Qt::UserRole + 19).toBool();
            QDropEvent drop(point, Qt::CopyAction | Qt::MoveAction, drag.mimeData(), button,
                            modifiers);
            QCoreApplication::sendEvent(view->viewport(), &drop);
            dropped = drop.isAccepted();
            return dropped ? drop.dropAction() : Qt::IgnoreAction;
        });
        sourceTable->sourcePanel->AttachDragServices(&transport);
        sourceTable->sourcePanel->OnDrag(right);
        sourceTable->sourcePanel->AttachDragServices(nullptr);
        chooseMenu.stop();
        QVERIFY(sourceRefused);
        QVERIFY(entered && highlighted);
        QCOMPARE(dropped, !cancel);
        QCOMPARE(menuShown, right);
        QVERIFY(!folder.data(Qt::UserRole + 19).toBool());
        QCOMPARE(targetTable->selectionModel()->selectedRows(), selection);
        if (cancel) {
            QVERIFY(sourceTable->item(sourceRow, 0)->isSelected());
            QVERIFY(QFileInfo::exists(input.fileName()));
            QVERIFY(!QFileInfo::exists(QDir(QDir(target).filePath(subfolder)).filePath(name)));
            return;
        }
        QFile result(QDir(QDir(target).filePath(subfolder)).filePath(name));
        QVERIFY(result.open(QIODevice::ReadOnly));
        QCOMPARE(result.readAll(), QByteArray("UI drag bytes"));
        QCOMPARE(QFileInfo::exists(input.fileName()), !move);
        for (const QSize size : {QSize(980, 620), QSize(640, 400)}) {
            window.resize(size);
            QCoreApplication::processEvents();
            QVERIFY(UiInspection::fullyVisible(*view, window));
            QVERIFY(UiInspection::writeReport(window, QString("drag-%1-%2-%3x%4")
                                                          .arg(icons ? "icons" : "table")
                                                          .arg(sourceIndex)
                                                          .arg(size.width())
                                                          .arg(size.height())));
        }
    }
};
QTEST_MAIN(DragDropUiTests)
#include "drag_drop_ui_tests.moc"
