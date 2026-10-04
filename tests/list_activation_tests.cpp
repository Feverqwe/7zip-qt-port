#include "CPP/7zip/UI/FileManager/ListViewDialog.h"
#include "CPP/7zip/UI/FileManager/RegistryUtils.h"
#include "CPP/7zip/UI/FileManager/ViewSettings.h"
#include "qtlistview.h"
#include <QApplication>
#include <QMouseEvent>
#include <QPlainTextEdit>
#include <QTemporaryDir>
#include <QTimer>
#include <QTreeWidget>
#include <QWheelEvent>
#include <QtTest>
#include <memory>

class ListActivationTests : public QObject {
    Q_OBJECT
    QTemporaryDir _dir;
    void prepare(CListViewDialog &dialog, bool singleClick, unsigned columns = 1) {
        dialog.SettingsPath = _dir.filePath("настройки списка.ini");
        CFmSettings settings;
        settings.Load(dialog.SettingsPath);
        settings.SingleClick = singleClick;
        settings.Save(dialog.SettingsPath);
        dialog.NumColumns = columns;
        dialog.SelectFirst = true;
        for (int i = 0; i < 3; ++i) {
            dialog.Strings.Add(ToUString(QString("путь %1").arg(i)));
            dialog.Values.Add(ToUString(QString("значение %1\nполный текст").arg(i)));
        }
        dialog.OnInit();
        dialog.show();
        QApplication::processEvents();
    }
    static QPoint position(QTreeWidget *list, int row) {
        return list->visualItemRect(list->topLevelItem(row)).center();
    }
    static void move(QTreeWidget *list, int row, Qt::KeyboardModifiers modifiers = Qt::NoModifier) {
        const QPoint pos = position(list, row);
        if (modifiers == Qt::NoModifier) {
            auto *window = list->window();
            QTest::mouseMove(
                window->windowHandle(),
                list->viewport()->mapTo(window, QPoint(1, list->viewport()->height() - 2)));
            QTest::mouseMove(window->windowHandle(), list->viewport()->mapTo(window, pos));
            return;
        }
        QMouseEvent event(QEvent::MouseMove, QPointF(pos),
                          QPointF(list->viewport()->mapToGlobal(pos)), Qt::NoButton, Qt::NoButton,
                          modifiers);
        QApplication::sendEvent(list->viewport(), &event);
    }

  private slots:
    void historyActivation_data() {
        QTest::addColumn<bool>("singleClick");
        QTest::newRow("single") << true;
        QTest::newRow("double") << false;
    }
    void historyActivation() {
        QFETCH(bool, singleClick);
        CListViewDialog dialog;
        prepare(dialog, singleClick);
        auto *list = dialog.findChild<QTreeWidget *>();
        QTest::mouseClick(list->viewport(), Qt::LeftButton, Qt::NoModifier, position(list, 1));
        if (!singleClick) {
            QVERIFY(dialog.isVisible());
            QTest::mouseDClick(list->viewport(), Qt::LeftButton, Qt::NoModifier, position(list, 1));
        }
        QCOMPARE(dialog.result(), int(QDialog::Accepted));
        QCOMPARE(dialog.FocusedItemIndex, 1);
    }
    void modifiedClickOnlySelects() {
        CListViewDialog dialog;
        prepare(dialog, true);
        auto *list = dialog.findChild<QTreeWidget *>();
        QTest::mouseClick(list->viewport(), Qt::LeftButton, Qt::ControlModifier, position(list, 2));
        QCOMPARE(list->selectedItems().size(), 2);
        QVERIFY(dialog.isVisible());
        QTest::mouseClick(list->viewport(), Qt::LeftButton, Qt::ShiftModifier, position(list, 0));
        QVERIFY(dialog.isVisible());
        dialog.reject();
        QCOMPARE(dialog.FocusedItemIndex, -1);
    }
    void singleClickDetail_data() {
        QTest::addColumn<unsigned>("columns");
        QTest::addColumn<Qt::KeyboardModifiers>("modifiers");
        QTest::newRow("properties") << 2u << Qt::KeyboardModifiers(Qt::NoModifier);
        QTest::newRow("alt-history") << 1u << Qt::KeyboardModifiers(Qt::AltModifier);
    }
    void singleClickDetail() {
        QFETCH(unsigned, columns);
        QFETCH(Qt::KeyboardModifiers, modifiers);
        CListViewDialog dialog;
        prepare(dialog, true, columns);
        auto *list = dialog.findChild<QTreeWidget *>();
        int opened = 0;
        QTimer::singleShot(0, &dialog, [&] {
            auto *detail = dialog.findChild<QDialog *>("listItemInfoDialog");
            QVERIFY(detail);
            QCOMPARE(detail->findChild<QPlainTextEdit *>()->toPlainText(),
                     columns == 1 ? QString("путь 1") : QString("значение 1\nполный текст"));
            ++opened;
            detail->reject();
        });
        QTest::mouseClick(list->viewport(), Qt::LeftButton, modifiers, position(list, 1));
        if (modifiers.testFlag(Qt::AltModifier))
            QTest::keyRelease(list, Qt::Key_Alt);
        QCOMPARE(opened, 1);
        QVERIFY(dialog.isVisible());
        QCOMPARE(dialog.FocusedItemIndex, -1);
    }
    void hoverSelectsWithoutActivating_data() { historyActivation_data(); }
    void hoverSelectsWithoutActivating() {
        QFETCH(bool, singleClick);
        CListViewDialog dialog;
        prepare(dialog, singleClick);
        auto *list = dialog.findChild<QTreeWidget *>();
        move(list, 1);
        if (singleClick)
            QTRY_COMPARE(list->currentItem(), list->topLevelItem(1));
        else {
            QTest::qWait(450);
            QCOMPARE(list->currentItem(), list->topLevelItem(0));
        }
        QVERIFY(dialog.isVisible());
        QCOMPARE(dialog.FocusedItemIndex, -1);
    }
    void hoverStopsOnLeaveModifiersAndRemovedRows() {
        CListViewDialog dialog;
        prepare(dialog, true);
        auto *list = dialog.findChild<QTreeWidget *>();
        move(list, 1, Qt::ShiftModifier);
        QTest::qWait(450);
        QCOMPARE(list->currentItem(), list->topLevelItem(0));
        QTest::keyRelease(list, Qt::Key_Shift);
        move(list, 1);
        QEvent leave(QEvent::Leave);
        QApplication::sendEvent(list->viewport(), &leave);
        QTest::qWait(450);
        QCOMPARE(list->currentItem(), list->topLevelItem(0));
        move(list, 1);
        delete list->takeTopLevelItem(1);
        QTest::qWait(450);
        QCOMPARE(list->currentItem(), list->topLevelItem(0));
        QVERIFY(dialog.isVisible());
    }
    void hoverCancelsBeforeDelayedSelection_data() {
        QTest::addColumn<QString>("action");
        for (const char *action : {"wheel", "disable", "hide-show", "close"})
            QTest::newRow(action) << QString::fromLatin1(action);
    }
    void hoverCancelsBeforeDelayedSelection() {
        QFETCH(QString, action);
        auto dialog = std::make_unique<CListViewDialog>();
        prepare(*dialog, true);
        auto *list = dynamic_cast<CListView *>(dialog->findChild<QTreeWidget *>());
        QVERIFY(list);
        move(list, 1);
        if (action == "wheel") {
            const QPoint pos = position(list, 1);
            QWheelEvent wheel(QPointF(pos), QPointF(list->viewport()->mapToGlobal(pos)), QPoint(),
                              QPoint(0, 120), Qt::NoButton, Qt::NoModifier, Qt::NoScrollPhase,
                              false);
            QApplication::sendEvent(list->viewport(), &wheel);
        } else if (action == "disable") {
            list->SetSingleClickActivation(false);
        } else if (action == "hide-show") {
            list->hide();
            list->show();
        } else {
            QPointer<QTreeWidget> lifetime(list);
            dialog.reset();
            QVERIFY(lifetime.isNull());
            QTest::qWait(450);
            return;
        }
        QTest::qWait(450);
        QCOMPARE(list->currentItem(), list->topLevelItem(0));
        QCOMPARE(dialog->FocusedItemIndex, -1);
        QVERIFY(dialog->isVisible());
        move(list, 2);
        if (action != "disable")
            QTRY_COMPARE_WITH_TIMEOUT(list->currentItem(), list->topLevelItem(2), 1200);
        else {
            QTest::qWait(450);
            QCOMPARE(list->currentItem(), list->topLevelItem(0));
        }
    }
};
QTEST_MAIN(ListActivationTests)
#include "list_activation_tests.moc"
