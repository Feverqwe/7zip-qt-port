#include "filetable.h"
#include "layout_inspection.h"
#include "window_state_support.h"

#include <QCheckBox>
#include <QMouseEvent>
#include <QScrollBar>

namespace {
struct CurrentDirectoryGuard {
    const QString previous = QDir::currentPath();
    ~CurrentDirectoryGuard() { QDir::setCurrent(previous); }
};

bool writeFile(const QString &path) {
    QFile file(path);
    return file.open(QIODevice::WriteOnly) && file.write("file") == 4;
}

QString screenshotPath(const QString &name) {
    const QDir report(qEnvironmentVariable("SEVENZIP_UI_REPORT_DIR"));
    const QString folder = report.filePath(QStringLiteral("../ui-screenshots"));
    QDir().mkpath(folder);
    return QDir(folder).filePath(name);
}
} // namespace

class ListSettingsRuntimeTests final : public QObject, protected MainWindowTestSupport {
    Q_OBJECT

  private slots:
    void init() { QFile::remove(qEnvironmentVariable("SEVENZIP_SETTINGS_PATH")); }

    void optionsParentRowAndContextMenu() {
        QTemporaryDir temp;
        QVERIFY(temp.isValid());
        CurrentDirectoryGuard directoryGuard;
        QVERIFY(QDir::setCurrent(temp.path()));
        QVERIFY(writeFile(temp.filePath(QStringLiteral("child.txt"))));
        MainWindow window;
        window.show();
        auto *files = static_cast<FileTable *>(table(window));
        QCOMPARE(files->rowCount(), 1);

        QTimer::singleShot(0, [&] {
            auto *dialog = window.findChild<QDialog *>("settingsDialog");
            QVERIFY(dialog);
            for (const char *name : {"showDotsOption", "fullRowOption", "showSystemMenuOption"})
                dialog->findChild<QCheckBox *>(name)->setChecked(true);
            dialog->accept();
        });
        action(window, "settingsAction")->trigger();
        QCOMPARE(files->rowCount(), 2);
        QCOMPARE(files->item(0, 0)->text(), QStringLiteral(".."));
        QCOMPARE(files->item(0, 0)->data(Qt::UserRole + 6).toUInt(), quint32(-1));
        QCOMPARE(files->item(0, 0)->toolTip(), QStringLiteral("Родительская папка"));
        QCOMPARE(files->accessibleName(), QStringLiteral("Список файлов"));
        QCOMPARE(files->item(1, 0)->toolTip(),
                 QFileInfo(temp.filePath(QStringLiteral("child.txt"))).canonicalFilePath());
        files->sortItems(0, Qt::DescendingOrder);
        QCOMPARE(files->item(0, 0)->text(), QStringLiteral(".."));
        files->sortItems(0, Qt::AscendingOrder);
        QVERIFY(files->fullRowHighlight());
        QSettings settings(qEnvironmentVariable("SEVENZIP_SETTINGS_PATH"), QSettings::IniFormat);
        QVERIFY(settings.value("FM/ShowDots").toBool());
        QVERIFY(settings.value("FM/FullRow").toBool());
        QVERIFY(settings.value("FM/ShowSystemMenu").toBool());

        files->selectRow(1);
        const QRect selectedSizeCell = files->visualItemRect(files->item(1, 2));
        const QPoint selectedPixel(selectedSizeCell.left() + 5, selectedSizeCell.center().y());
        const QColor fullRowColor = files->viewport()->grab().toImage().pixelColor(selectedPixel);
        if (qEnvironmentVariableIntValue("SEVENZIP_CAPTURE_SCREENSHOTS") == 1)
            QVERIFY(files->viewport()->grab().save(
                screenshotPath(QStringLiteral("list-settings-full-row.png"))));
        QTimer::singleShot(0, [&] {
            auto *menu = qobject_cast<QMenu *>(QApplication::activePopupWidget());
            QVERIFY(menu);
            bool found = false;
            for (QAction *item : menu->actions())
                found |= item->text() == QStringLiteral("Открыть в системном файловом менеджере");
            QVERIFY(found);
            menu->close();
        });
        QVERIFY(QMetaObject::invokeMethod(
            files, "customContextMenuRequested", Qt::DirectConnection,
            Q_ARG(QPoint, files->visualItemRect(files->item(1, 0)).center())));

        MainWindow reopened;
        QCOMPARE(table(reopened)->rowCount(), 2);
        reopened.show();
        for (int width : {980, 640}) {
            reopened.resize(width, width == 980 ? 620 : 400);
            QApplication::processEvents();
            QVERIFY(UiInspection::fullyVisible(*table(reopened), reopened));
            QVERIFY(
                UiInspection::writeReport(reopened, QStringLiteral("list-settings-%1").arg(width)));
        }

        QTimer::singleShot(0, [&] {
            auto *dialog = window.findChild<QDialog *>("settingsDialog");
            QVERIFY(dialog);
            for (const char *name : {"showDotsOption", "fullRowOption", "showSystemMenuOption"})
                dialog->findChild<QCheckBox *>(name)->setChecked(false);
            dialog->accept();
        });
        action(window, "settingsAction")->trigger();
        QCOMPARE(files->rowCount(), 1);
        QVERIFY(!files->fullRowHighlight());
        files->selectRow(0);
        const QRect nameOnlySizeCell = files->visualItemRect(files->item(0, 2));
        const QPoint nameOnlyPixel(nameOnlySizeCell.left() + 5, nameOnlySizeCell.center().y());
        const QColor nameOnlyColor = files->viewport()->grab().toImage().pixelColor(nameOnlyPixel);
        QVERIFY(fullRowColor != nameOnlyColor);
        if (qEnvironmentVariableIntValue("SEVENZIP_CAPTURE_SCREENSHOTS") == 1)
            QVERIFY(files->viewport()->grab().save(
                screenshotPath(QStringLiteral("list-settings-name-only.png"))));
        QCOMPARE(settings.value("FM/ShowDots").toBool(), false);
        QCOMPARE(settings.value("FM/FullRow").toBool(), false);
        QCOMPARE(settings.value("FM/ShowSystemMenu").toBool(), false);
    }

    void hoverSelectsWithoutOpeningAndIgnoresScroll() {
        QTemporaryDir temp;
        QVERIFY(temp.isValid());
        CurrentDirectoryGuard directoryGuard;
        QVERIFY(QDir::setCurrent(temp.path()));
        for (int i = 0; i < 60; ++i)
            QVERIFY(
                writeFile(temp.filePath(QStringLiteral("file-%1.txt").arg(i, 3, 10, QChar('0')))));
        QSettings settings(qEnvironmentVariable("SEVENZIP_SETTINGS_PATH"), QSettings::IniFormat);
        settings.setValue("FM/SingleClick", true);
        settings.sync();
        MainWindow window;
        window.resize(640, 400);
        window.show();
        auto *files = table(window);
        auto *icons = window.findChild<QListView *>("iconList");
        QCOMPARE(icons->accessibleName(), QStringLiteral("Список файлов"));
        const QString path = window.findChild<QLineEdit *>("archivePath")->text();
        for (int mode : {3, 0}) {
            action(window, mode == 3 ? "viewMode3Action" : "viewMode0Action")->trigger();
            QAbstractItemView *view = mode == 3 ? static_cast<QAbstractItemView *>(files)
                                                : static_cast<QAbstractItemView *>(icons);
            const QModelIndex target = files->model()->index(4, 0);
            view->scrollTo(target);
            QTest::mouseMove(view->viewport(), view->visualRect(target).center());
            QTRY_COMPARE_WITH_TIMEOUT(files->currentRow(), 4, 1200);
            QCOMPARE(window.findChild<QLineEdit *>("archivePath")->text(), path);
            const QModelIndex delayed = files->model()->index(12, 0);
            view->scrollTo(delayed);
            QTest::mouseMove(view->viewport(), view->visualRect(delayed).center());
            view->verticalScrollBar()->setValue(view->verticalScrollBar()->maximum());
            QTest::qWait(550);
            QCOMPARE(files->currentRow(), 4);

            for (const auto gesture : {Qt::ControlModifier, Qt::ShiftModifier}) {
                view->scrollTo(delayed);
                const QPoint position = view->visualRect(delayed).center();
                QMouseEvent move(QEvent::MouseMove, QPointF(position),
                                 QPointF(view->viewport()->mapToGlobal(position)), Qt::NoButton,
                                 Qt::NoButton, gesture);
                QApplication::sendEvent(view->viewport(), &move);
                QTest::qWait(450);
                QCOMPARE(files->currentRow(), 4);
            }
            view->scrollTo(delayed);
            const QPoint dragPosition = view->visualRect(delayed).center();
            QMouseEvent dragging(QEvent::MouseMove, QPointF(dragPosition),
                                 QPointF(view->viewport()->mapToGlobal(dragPosition)), Qt::NoButton,
                                 Qt::LeftButton, Qt::NoModifier);
            QApplication::sendEvent(view->viewport(), &dragging);
            files->setCurrentCell(4, 0);
            QTest::qWait(450);
            QCOMPARE(files->currentRow(), 4);
            view->scrollTo(delayed);
            QTest::mouseMove(view->viewport(), view->visualRect(delayed).center());
            QEvent leave(QEvent::Leave);
            QApplication::sendEvent(view->viewport(), &leave);
            QTest::qWait(450);
            QCOMPARE(files->currentRow(), 4);
        }

        const QString focusedName = files->item(files->currentRow(), 0)->text();
        const QModelIndex removed = files->model()->index(12, 0);
        icons->scrollTo(removed);
        QTest::mouseMove(icons->viewport(), icons->visualRect(removed).center());
        QVERIFY(QFile::remove(files->item(12, 1)->text()));
        action(window, "refreshAction")->trigger();
        QCOMPARE(files->rowCount(), 59);
        QTest::qWait(500);
        QCOMPARE(files->item(files->currentRow(), 0)->text(), focusedName);
    }

    void parentItemInArchiveAndVirtualRoot() {
        QTemporaryDir temp;
        QVERIFY(temp.isValid());
        CurrentDirectoryGuard directoryGuard;
        QVERIFY(QDir::setCurrent(temp.path()));
        QSettings settings(qEnvironmentVariable("SEVENZIP_SETTINGS_PATH"), QSettings::IniFormat);
        settings.setValue("FM/ShowDots", true);
        settings.sync();
        ControlledBackend backend;
        MainWindow window(&backend);
        openListedArchive(window, backend, temp.filePath("sample.zip"),
                          {ArchiveEntry{QStringLiteral("папка/имя.txt"), 4, 4}});
        QTRY_VERIFY_WITH_TIMEOUT(table(window)->rowCount() >= 2, 1500);
        for (int mode = 0; mode < 4; ++mode) {
            action(window, qPrintable(QStringLiteral("viewMode%1Action").arg(mode)))->trigger();
            QCOMPARE(table(window)->item(0, 0)->text(), QStringLiteral(".."));
        }
        openRow(window, 0);
        QTRY_COMPARE_WITH_TIMEOUT(window.findChild<QLineEdit *>("archivePath")->text(),
                                  QFileInfo(temp.path()).canonicalFilePath(), 1500);
        QCOMPARE(table(window)->item(0, 0)->text(), QStringLiteral(".."));
        action(window, "openRootAction")->trigger();
        QVERIFY(filesystemRow(window, QStringLiteral("..")) < 0);
    }
};

QTEST_MAIN(ListSettingsRuntimeTests)
#include "list_settings_runtime_tests.moc"
