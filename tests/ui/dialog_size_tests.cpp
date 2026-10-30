#include "fileops/native/Dialog.h"
#include "ui/dialogs/qtdialogscreenservices.h"
#include <QDialog>
#include <QScreen>
#include <QtTest>

namespace {
class Screen final : public IQtDialogScreenServices {
  public:
    QRect Area;
    QSize GetDialogBaseUnits() const override { return {8, 16}; }
    QRect GetWorkAreaRect() const override { return Area; }
};
} // namespace

class DialogSizeTests final : public QObject {
    Q_OBJECT
  private slots:
    void sourcePositionPolicy_data() {
        QTest::addColumn<QRect>("initial");
        QTest::addColumn<QRect>("parentRect");
        QTest::addColumn<QRect>("parentArea");
        QTest::addColumn<QRect>("expected");
        const QRect work(0, 0, 400, 300);
        QTest::newRow("inside-no-parent")
            << QRect(10, 20, 100, 80) << QRect() << work << QRect(10, 20, 100, 80);
        QTest::newRow("outside-no-parent")
            << QRect(500, 400, 100, 80) << QRect() << work << QRect(500, 400, 100, 80);
        QTest::newRow("partial-no-parent")
            << QRect(350, 250, 100, 80) << QRect() << work << QRect(150, 110, 100, 80);
        QTest::newRow("too-large")
            << QRect(-20, -20, 500, 400) << QRect() << work << QRect(-20, -20, 500, 400);
        QTest::newRow("inside-parent")
            << QRect(10, 20, 100, 80) << QRect(40, 30, 200, 160) << work << QRect(90, 70, 100, 80);
        QTest::newRow("outside-child-inside-parent")
            << QRect(500, 400, 100, 80) << QRect(40, 30, 200, 160) << work
            << QRect(90, 70, 100, 80);
        QTest::newRow("small-parent")
            << QRect(10, 20, 100, 80) << QRect(40, 30, 50, 40) << work << QRect(10, 20, 100, 80);
        QTest::newRow("partial-parent") << QRect(350, 250, 100, 80) << QRect(300, 200, 200, 160)
                                        << work << QRect(150, 110, 100, 80);
        QTest::newRow("other-monitor-parent")
            << QRect(350, 250, 100, 80) << QRect(440, 30, 200, 160) << QRect(400, 0, 400, 300)
            << QRect(150, 110, 100, 80);
        QTest::newRow("invalid-parent-area") << QRect(350, 250, 100, 80) << QRect(40, 30, 200, 160)
                                             << QRect() << QRect(350, 250, 100, 80);
    }
    void sourcePositionPolicy() {
        QFETCH(QRect, initial);
        QFETCH(QRect, parentRect);
        QFETCH(QRect, parentArea);
        QFETCH(QRect, expected);
        QWidget parent(nullptr, Qt::Window | Qt::FramelessWindowHint);
        parent.setGeometry(parentRect);
        QWidget window(parentRect.isValid() ? &parent : nullptr,
                       Qt::Window | Qt::FramelessWindowHint);
        window.setGeometry(initial);
        Screen screen, parentScreen;
        screen.Area = QRect(0, 0, 400, 300);
        parentScreen.Area = parentArea;
        NWindows::NControl::CDialog::NormalizePosition(window, screen, parentScreen);
        QCOMPARE(window.geometry(), expected);
        QVERIFY(!window.isVisible());
    }
    void sourceWorkAreaPolicy_data() {
        QTest::addColumn<QRect>("initial");
        QTest::addColumn<QRect>("area");
        QTest::addColumn<QRect>("reduced");
        QTest::addColumn<bool>("full");
        QTest::addColumn<bool>("oversized");
        const QRect work(0, 0, 200, 160);
        struct Case {
            const char *Name;
            QRect Initial, Area, Reduced;
            bool Oversized;
        };
        const Case cases[] = {
            {"invalid-area", {10, 10, 300, 240}, {}, {10, 10, 300, 240}, false},
            {"outside", {300, 300, 300, 240}, work, {300, 300, 300, 240}, false},
            {"fits", {150, 100, 100, 50}, work, {150, 100, 100, 50}, false},
            {"equal-size", {30, 30, 200, 160}, work, {30, 30, 200, 160}, false},
            {"wide", {-20, 10, 300, 100}, work, {0, 30, 200, 100}, true},
            {"tall", {50, 20, 100, 240}, work, {50, 0, 100, 160}, true},
            {"both", {20, 20, 300, 240}, work, {0, 0, 200, 160}, true},
        };
        for (const auto &entry : cases)
            for (bool full : {false, true})
                QTest::newRow(qPrintable(QString("%1-%2").arg(entry.Name).arg(full)))
                    << entry.Initial << entry.Area << entry.Reduced << full << entry.Oversized;
    }
    void sourceWorkAreaPolicy() {
        QFETCH(QRect, initial);
        QFETCH(QRect, area);
        QFETCH(QRect, reduced);
        QFETCH(bool, full);
        QFETCH(bool, oversized);
        QWidget window(nullptr, Qt::Window | Qt::FramelessWindowHint);
        window.setGeometry(initial);
        Screen screen;
        screen.Area = area;
        NWindows::NControl::CDialog::NormalizeSize(window, screen, full);
        QCOMPARE(window.isMaximized(), full && oversized);
        QVERIFY(!window.isVisible()); // Source OnInit must not expose a modeless view.
        if (!full || !oversized)
            QCOMPARE(window.geometry(), reduced);
    }
    void firstShowNormalizesWithoutOverwritingLaterUserResize() {
        QDialog dialog;
        const QSize work = dialog.screen()->availableGeometry().size();
        // Exercise reduction without filling both screen dimensions: Cocoa
        // reports a work-area-sized ordinary window as zoomed/maximized.
        dialog.resize(work.width() * 2, qMin(300, work.height() / 2));
        QtNormalizeDialogSize(dialog);
        QVERIFY(!dialog.isVisible());
        QVERIFY(!dialog.isMaximized());
        dialog.show();
        QVERIFY(QTest::qWaitForWindowExposed(&dialog));
        QTRY_VERIFY(dialog.width() <= work.width() && dialog.height() <= work.height());
        QVERIFY(!dialog.isMaximized());
        dialog.resize(200, 100);
        QTRY_COMPARE(dialog.size(), QSize(200, 100));
        const QSize userSize = dialog.size();
        dialog.hide();
        dialog.show();
        QTRY_VERIFY(!dialog.isMaximized());
        QTRY_COMPARE(dialog.size(), userSize);
    }
};

QTEST_MAIN(DialogSizeTests)
#include "dialog_size_tests.moc"
