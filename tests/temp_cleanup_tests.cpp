#include "CPP/7zip/UI/FileManager/BrowseDialog2.h"
#include "CPP/7zip/UI/FileManager/LangUtils.h"
#include "CPP/7zip/UI/FileManager/RegistryUtils.h"
#include "layout_inspection.h"
#include "qtlangcontrols.h"
#include "qtownedtempdir.h"
#include <QComboBox>
#include <QContextMenuEvent>
#include <QDesktopServices>
#include <QFile>
#include <QFileInfo>
#include <QHeaderView>
#include <QLineEdit>
#include <QMenu>
#include <QMessageBox>
#include <QPushButton>
#include <QTemporaryDir>
#include <QTemporaryFile>
#include <QTextBrowser>
#include <QTimer>
#include <QTreeWidget>
#include <QtTest>

class TempCleanupTests : public QObject {
    Q_OBJECT
    QUrl opened;
    static QTreeWidgetItem *row(CBrowseDialog2 &dialog, const QString &path) {
        auto *list = dialog.findChild<QTreeWidget *>("tempList");
        const QString name = QFileInfo(path).fileName().normalized(QString::NormalizationForm_C);
        for (int i = 0; i < list->topLevelItemCount(); ++i)
            if (list->topLevelItem(i)->text(0).normalized(QString::NormalizationForm_C) == name)
                return list->topLevelItem(i);
        return nullptr;
    }
    static void select(CBrowseDialog2 &dialog, const QString &path) {
        auto *list = dialog.findChild<QTreeWidget *>("tempList");
        list->clearSelection();
        if (auto *item = row(dialog, path)) {
            list->setCurrentItem(item);
            item->setSelected(true);
        }
    }
    static bool file(const QString &path) {
        QFile output(path);
        return output.open(QIODevice::WriteOnly) && output.write("fixture") == 7;
    }
    static bool inactive(const QString &path) {
        auto lease = QtOwnedTempRegistry().RegisterCreated(path);
        return bool(lease);
    }
  public slots:
    void captureFile(const QUrl &url) { opened = url; }
  private slots:
    void init() {
        QFile::remove(qEnvironmentVariable("SEVENZIP_SETTINGS_PATH"));
        ReloadLang(qEnvironmentVariable("SEVENZIP_SETTINGS_PATH"));
    }
    void ownedFileCleanupAndLinkNavigationNeverReachUserTargets() {
        QTemporaryDir root;
        QTemporaryDir outside;
        QTemporaryFile temporary(QDir::tempPath() + QStringLiteral("/cleanup файл-XXXXXX.txt"));
        QVERIFY(root.isValid() && outside.isValid());
        QVERIFY(temporary.open());
        temporary.close();
        QVERIFY(file(outside.filePath("keep.txt")));
        const QString link = root.filePath("external link");
        if (!QFile::link(outside.path(), link) || !QFileInfo(link).isSymLink())
            QSKIP("OS does not provide a directory symlink for this fixture");
        QVERIFY(inactive(root.path()) && inactive(temporary.fileName()));
        bool blocked = false, deleted = false, fileSelected = false, asked = false;
        QStringList visibleNames;
        QTimer::singleShot(0, qApp, [&] {
            auto *dialog = dynamic_cast<CBrowseDialog2 *>(QApplication::activeModalWidget());
            if (!dialog)
                return;
            auto *list = dialog->findChild<QTreeWidget *>("tempList");
            select(*dialog, root.path());
            QTest::keyClick(list, Qt::Key_Return);
            QTimer::singleShot(0, dialog, [&] {
                if (auto *box = qobject_cast<QMessageBox *>(QApplication::activeModalWidget())) {
                    blocked = box->text().contains("blocked", Qt::CaseInsensitive);
                    box->button(QMessageBox::Ok)->click();
                }
            });
            QTest::keyClick(list, Qt::Key_Return);
            QTest::keyClick(list, Qt::Key_Backspace);
            for (int i = 0; i < list->topLevelItemCount(); ++i)
                visibleNames.append(list->topLevelItem(i)->text(0));
            select(*dialog, temporary.fileName());
            fileSelected = row(*dialog, temporary.fileName()) &&
                           dialog->findChild<QPushButton *>("tempDelete")->isEnabled();
            QTimer::singleShot(0, dialog, [&] {
                if (auto *box = qobject_cast<QMessageBox *>(QApplication::activeModalWidget())) {
                    asked = box->standardButtons().testFlag(QMessageBox::Yes);
                    box->button(QMessageBox::Yes)->click();
                }
            });
            dialog->findChild<QPushButton *>("tempDelete")->click();
            deleted = !QFileInfo::exists(temporary.fileName());
            dialog->reject();
        });
        MyBrowseForTempFolder(nullptr);
        QVERIFY(blocked);
        QVERIFY2(fileSelected, qPrintable(visibleNames.join('|')));
        QVERIFY(asked);
        QVERIFY(deleted);
        QVERIFY(QFileInfo::exists(outside.filePath("keep.txt")));
        QVERIFY(QFileInfo(link).isSymLink());
    }
    void sourceShowDotsAndSingleClickRetainExternalProtection() {
        QTemporaryDir root;
        QVERIFY(root.isValid());
        QVERIFY(file(root.filePath("inside.txt")));
        QVERIFY(inactive(root.path()));
        CFmSettings prefs;
        prefs.Load(qEnvironmentVariable("SEVENZIP_SETTINGS_PATH"));
        prefs.ShowDots = prefs.SingleClick = prefs.FullRow = prefs.ShowGrid = true;
        prefs.Save(qEnvironmentVariable("SEVENZIP_SETTINGS_PATH"));
        opened = QUrl();
        QDesktopServices::setUrlHandler("file", this, "captureFile");
        bool dots = false, launchProtected = false;
        QTimer::singleShot(0, qApp, [&] {
            auto *dialog = dynamic_cast<CBrowseDialog2 *>(QApplication::activeModalWidget());
            if (!dialog)
                return;
            auto *list = dialog->findChild<QTreeWidget *>("tempList");
            if (auto *item = row(*dialog, root.path()))
                QTest::mouseClick(list->viewport(), Qt::LeftButton, Qt::NoModifier,
                                  list->visualItemRect(item).center());
            dots = !list->findItems("..", Qt::MatchExactly).isEmpty();
            if (auto *item = row(*dialog, root.filePath("inside.txt")))
                QTest::mouseClick(list->viewport(), Qt::LeftButton, Qt::NoModifier,
                                  list->visualItemRect(item).center());
            for (const auto &entry : QtOwnedTempRegistry().Entries())
                if (entry.path == root.path())
                    launchProtected = entry.externallyProtected && !entry.active;
            dialog->reject();
        });
        MyBrowseForTempFolder(nullptr);
        QDesktopServices::unsetUrlHandler("file");
        QVERIFY(dots && launchProtected);
        QCOMPARE(QFileInfo(opened.toLocalFile()).canonicalFilePath(),
                 QFileInfo(root.filePath("inside.txt")).canonicalFilePath());
        QVERIFY(QFileInfo::exists(root.filePath("inside.txt")));
    }
    void sourceContextPropertiesKeepCommandOrderAndNativeDetails() {
        QTemporaryDir root;
        QVERIFY(root.isValid());
        QVERIFY(file(root.filePath("details.txt")));
        QVERIFY(inactive(root.path()));
        QList<unsigned> commands;
        bool properties = false;
        QTimer::singleShot(0, qApp, [&] {
            auto *dialog = dynamic_cast<CBrowseDialog2 *>(QApplication::activeModalWidget());
            if (!dialog)
                return;
            select(*dialog, root.path());
            auto *list = dialog->findChild<QTreeWidget *>("tempList");
            QTimer choose;
            connect(&choose, &QTimer::timeout, dialog, [&] {
                if (auto *menu = qobject_cast<QMenu *>(QApplication::activePopupWidget())) {
                    QAction *detail = nullptr;
                    for (auto *action : menu->actions())
                        if (!action->isSeparator()) {
                            commands.append(action->data().toUInt());
                            if (action->data().toUInt() == 4)
                                detail = action;
                        }
                    if (detail) {
                        menu->setActiveAction(detail);
                        QTest::keyClick(menu, Qt::Key_Return);
                    } else
                        menu->close();
                } else if (auto *box =
                               qobject_cast<QMessageBox *>(QApplication::activeModalWidget())) {
                    properties =
                        box->text().contains("details.txt") && box->text().contains("Size");
                    box->button(QMessageBox::Ok)->click();
                }
            });
            choose.start(5);
            const QPoint point = list->visualItemRect(list->currentItem()).center();
            QContextMenuEvent context(QContextMenuEvent::Mouse, point,
                                      list->viewport()->mapToGlobal(point));
            QApplication::sendEvent(list->viewport(), &context);
            choose.stop();
            dialog->reject();
        });
        MyBrowseForTempFolder(nullptr);
        QCOMPARE(commands, QList<unsigned>({1, 2, 3, 4}));
        QVERIFY(properties);
        QVERIFY(QFileInfo::exists(root.filePath("details.txt")));
    }
    void confirmationControlsTheActualNativeDeleteLoop_data() {
        QTest::addColumn<int>("answer");
        QTest::newRow("yes") << int(QMessageBox::Yes);
        QTest::newRow("no") << int(QMessageBox::No);
        QTest::newRow("cancel") << int(QMessageBox::Cancel);
    }
    void confirmationControlsTheActualNativeDeleteLoop() {
        QFETCH(int, answer);
        QTemporaryDir owned(QDir::tempPath() + QStringLiteral("/cleanup имя с пробелом-XXXXXX"));
        QTemporaryDir unowned(QDir::tempPath() + "/7zO12345678-XXXXXX");
        QVERIFY(owned.isValid() && unowned.isValid());
        QVERIFY(file(owned.filePath(QStringLiteral("данные с пробелом.txt"))));
        QVERIFY(file(unowned.filePath("keep.txt")));
        QVERIFY(inactive(owned.path()));
        bool present = false, excluded = false, confirmed = false, enabled = false;
        QTimer::singleShot(0, qApp, [&] {
            auto *dialog = dynamic_cast<CBrowseDialog2 *>(QApplication::activeModalWidget());
            if (!dialog)
                return;
            present = row(*dialog, owned.path());
            excluded = !row(*dialog, unowned.path());
            select(*dialog, owned.path());
            auto *button = dialog->findChild<QPushButton *>("tempDelete");
            enabled = button->isEnabled();
            QTimer::singleShot(0, dialog, [&] {
                if (auto *question =
                        qobject_cast<QMessageBox *>(QApplication::activeModalWidget())) {
                    confirmed = question->standardButtons().testFlag(QMessageBox::Cancel) &&
                                question->text().contains(QFileInfo(owned.path()).fileName());
                    question->button(QMessageBox::StandardButton(answer))->click();
                }
            });
            button->click();
            dialog->reject();
        });
        MyBrowseForTempFolder(nullptr);
        QVERIFY(present && excluded && confirmed && enabled);
        QCOMPARE(QFileInfo::exists(owned.path()), answer != QMessageBox::Yes);
        QVERIFY(QFileInfo::exists(unowned.filePath("keep.txt")));
    }
    void activeAndUnknownExternalSessionsSurviveActualDeleteAttempts() {
        QtOwnedTempDir active;
        QTemporaryDir protectedCopy;
        QVERIFY(active.isValid() && protectedCopy.isValid());
        QVERIFY(file(active.filePath("active.txt")));
        QVERIFY(file(protectedCopy.filePath("external.txt")));
        auto protectedLease = QtOwnedTempRegistry().RegisterCreated(protectedCopy.path());
        QVERIFY(protectedLease && protectedLease->ProtectExternal());
        protectedLease.reset();
        bool activeDisabled = false, unknownDisabled = false;
        int messages = 0;
        QTimer::singleShot(0, qApp, [&] {
            auto *dialog = dynamic_cast<CBrowseDialog2 *>(QApplication::activeModalWidget());
            if (!dialog)
                return;
            select(*dialog, active.path());
            activeDisabled = row(*dialog, active.path()) &&
                             !dialog->findChild<QPushButton *>("tempDelete")->isEnabled();
            select(*dialog, protectedCopy.path());
            unknownDisabled = row(*dialog, protectedCopy.path()) &&
                              !dialog->findChild<QPushButton *>("tempDelete")->isEnabled();
            QTimer answers;
            connect(&answers, &QTimer::timeout, dialog, [&] {
                if (auto *box = qobject_cast<QMessageBox *>(QApplication::activeModalWidget())) {
                    ++messages;
                    box->button(box->standardButtons().testFlag(QMessageBox::Yes) ? QMessageBox::Yes
                                                                                  : QMessageBox::Ok)
                        ->click();
                }
            });
            answers.start(5);
            QTest::keyClick(dialog->findChild<QTreeWidget *>("tempList"), Qt::Key_Delete);
            answers.stop();
            dialog->reject();
        });
        MyBrowseForTempFolder(nullptr);
        QVERIFY(activeDisabled && unknownDisabled);
        QCOMPARE(messages, 2);
        QVERIFY(QFileInfo::exists(active.filePath("active.txt")));
        QVERIFY(QFileInfo::exists(protectedCopy.filePath("external.txt")));
    }
    void partialFailureStopsBeforeLaterSelectedItemsAndRefreshPreservesSelection() {
        QTemporaryDir first(QDir::tempPath() + "/cleanup-a-XXXXXX");
        QTemporaryDir second(QDir::tempPath() + "/cleanup-b-XXXXXX");
        QTemporaryDir third(QDir::tempPath() + "/cleanup-c-XXXXXX");
        QTemporaryDir backup;
        QVERIFY(first.isValid() && second.isValid() && third.isValid() && backup.isValid());
        QVERIFY(inactive(first.path()) && inactive(second.path()) && inactive(third.path()));
        const QString moved = backup.filePath("original second");
        bool selected = false, replaced = false, refreshed = false;
        int messages = 0;
        QTimer::singleShot(0, qApp, [&] {
            auto *dialog = dynamic_cast<CBrowseDialog2 *>(QApplication::activeModalWidget());
            if (!dialog)
                return;
            auto *list = dialog->findChild<QTreeWidget *>("tempList");
            list->header()->sectionClicked(0);
            list->clearSelection();
            for (const auto &path : {first.path(), second.path(), third.path()})
                if (auto *item = row(*dialog, path))
                    item->setSelected(true);
            selected = list->selectedItems().size() == 3;
            QTimer answers;
            connect(&answers, &QTimer::timeout, dialog, [&] {
                if (auto *box = qobject_cast<QMessageBox *>(QApplication::activeModalWidget())) {
                    ++messages;
                    if (box->standardButtons().testFlag(QMessageBox::Yes)) {
                        replaced =
                            QDir().rename(second.path(), moved) && QDir().mkpath(second.path());
                        box->button(QMessageBox::Yes)->click();
                    } else
                        box->button(QMessageBox::Ok)->click();
                }
            });
            answers.start(5);
            dialog->findChild<QPushButton *>("tempDelete")->click();
            answers.stop();
            dialog->findChild<QPushButton *>("tempRefresh")->click();
            auto *remaining = row(*dialog, third.path());
            refreshed = !row(*dialog, first.path()) && !row(*dialog, second.path()) && remaining &&
                        remaining->isSelected();
            dialog->reject();
        });
        MyBrowseForTempFolder(nullptr);
        QVERIFY(selected && replaced && refreshed);
        QCOMPARE(messages, 2);
        QVERIFY(!QFileInfo::exists(first.path()));
        QVERIFY(QFileInfo::exists(second.path()));
        QVERIFY(QFileInfo::exists(moved));
        QVERIFY(QFileInfo::exists(third.path()));
    }
    void sourceNavigationHelpAndNormalCompactGeometry() {
        QTemporaryDir owned;
        QTemporaryDir settings;
        QVERIFY(owned.isValid() && settings.isValid());
        QVERIFY(QDir().mkpath(owned.filePath(QStringLiteral("подпапка с пробелом"))));
        QVERIFY(file(owned.filePath(QStringLiteral("подпапка с пробелом/данные.txt"))));
        QVERIFY(inactive(owned.path()));
        bool navigated = true, geometry = true, reports = true, help = true;
        for (const char *language : {"-", "ru"}) {
            SaveRegLang(UString(language), settings.filePath("language.ini"));
            ReloadLang(settings.filePath("language.ini"));
            QTimer::singleShot(0, qApp, [&] {
                auto *dialog = dynamic_cast<CBrowseDialog2 *>(QApplication::activeModalWidget());
                if (!dialog) {
                    navigated = false;
                    return;
                }
                for (const QSize size : {QSize(900, 580), QSize(500, 320)}) {
                    dialog->resize(size);
                    QApplication::processEvents();
                    for (auto *button : dialog->findChildren<QPushButton *>())
                        geometry = geometry && UiInspection::fullyVisible(*button, *dialog);
                    for (const char *name : {"tempList", "tempPath", "tempFilter"})
                        geometry = geometry && UiInspection::fullyVisible(
                                                   *dialog->findChild<QWidget *>(name), *dialog);
                    reports =
                        reports &&
                        UiInspection::writeReport(
                            *dialog, QString("temp-cleanup-%1-%2").arg(language).arg(size.width()));
                }
                auto *list = dialog->findChild<QTreeWidget *>("tempList");
                select(*dialog, owned.path());
                QTest::keyClick(list, Qt::Key_Return);
                navigated = navigated && list->topLevelItemCount() == 1 &&
                            list->topLevelItem(0)->text(0) == QStringLiteral("подпапка с пробелом");
                QTest::keyClick(list, Qt::Key_Return);
                navigated = navigated && list->topLevelItemCount() == 1 &&
                            list->topLevelItem(0)->text(0) == QStringLiteral("данные.txt");
                QTest::keyClick(list, Qt::Key_Backspace);
                QTest::keyClick(list, Qt::Key_Backspace);
                navigated = navigated && row(*dialog, owned.path()) &&
                            row(*dialog, owned.path())->isSelected();
                dialog->findChild<QPushButton *>("tempHelp")->click();
                auto *helpWindow = dialog->findChild<QDialog *>("helpWindow");
                help = help && helpWindow && helpWindow->isVisible();
                if (helpWindow) {
                    help = help &&
                           helpWindow->findChild<QTextBrowser *>("helpBrowser")->source().path() ==
                               "/upstream/7zip/Help/fm/temp.htm";
                    helpWindow->close();
                    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
                    dialog->activateWindow();
                    list->setFocus();
                    QTest::qWait(10);
                    QTest::keyClick(list, Qt::Key_F1);
                    if (auto *again = dialog->findChild<QDialog *>("helpWindow")) {
                        help = help && again->isVisible();
                        again->close();
                    } else
                        help = false;
                }
                dialog->findChild<QPushButton *>("tempClose")->click();
            });
            MyBrowseForTempFolder(nullptr);
        }
        ReloadLang(settings.filePath("reset.ini"));
        QVERIFY(navigated && geometry && reports && help);
        QVERIFY(
            QFileInfo::exists(owned.filePath(QStringLiteral("подпапка с пробелом/данные.txt"))));
    }
};
QTEST_MAIN(TempCleanupTests)
#include "temp_cleanup_tests.moc"
