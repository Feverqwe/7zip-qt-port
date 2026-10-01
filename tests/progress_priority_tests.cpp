#include "CPP/7zip/UI/FileManager/ProgressDialog2.h"
#include "layout_inspection.h"
#include "processpriority.h"
#include <QPushButton>
#include <QtTest>

namespace {
class CRestorePriority {
    bool _background;

  public:
    explicit CRestorePriority(bool background) : _background(background) {}
    ~CRestorePriority() { NProcessPriority::SetBackground(0, _background); }
};
QPushButton *Background(CProgressDialog &dialog) {
    return dialog.findChild<QPushButton *>("IDB_PROGRESS_BACKGROUND");
}
bool NativeBackground() {
    bool background = false;
    return NProcessPriority::GetBackground(0, background) == S_OK && background;
}
} // namespace

class ProgressPriorityTests : public QObject {
    Q_OBJECT
  private slots:
    void sourceTransitionDefaultFocusAndGeometry_data() {
        QTest::addColumn<QSize>("size");
        QTest::newRow("normal") << QSize(760, 450);
        QTest::newRow("compact") << QSize(640, 380);
    }
    void sourceTransitionDefaultFocusAndGeometry() {
        QFETCH(QSize, size);
        bool original;
        QCOMPARE(NProcessPriority::GetBackground(0, original), S_OK);
        CRestorePriority restore(original);
        QCOMPARE(NProcessPriority::SetBackground(0, false), S_OK);
        QWidget main;
        CProgressDialog dialog(nullptr, L"Обновление архива с пробелом");
        dialog.MainWindow = &main;
        dialog.MainTitle = L"7-Zip";
        dialog.resize(size);
        dialog.show();
        QApplication::processEvents();
        auto *background = Background(dialog);
        auto *pause = dialog.findChild<QPushButton *>("IDB_PAUSE");
        QVERIFY(background->isEnabled());
        QVERIFY(background->isDefault());
        QCOMPARE(dialog.focusWidget(), background);
        QCOMPARE(background->text(), QStringLiteral("&Background"));
        QTest::keyClick(background, Qt::Key_Return);
        QVERIFY(NativeBackground());
        QCOMPARE(background->text(), QStringLiteral("&Foreground"));
        QVERIFY(dialog.windowTitle().contains(QStringLiteral("Background")));
        QVERIFY(main.windowTitle().contains(QStringLiteral("Background")));
        QVERIFY(background->sizeHint().width() <= background->width());
        QVERIFY(UiInspection::fullyVisible(*background, dialog));
        QVERIFY(UiInspection::writeReport(
            dialog, QString("progress-priority-%1-background").arg(size.width())));
        pause->setFocus();
        QTest::keyClick(pause, Qt::Key_Return);
        QVERIFY(dialog.Sync.Get_Paused());
        QVERIFY(NativeBackground());
        QCOMPARE(background->text(), QStringLiteral("&Foreground"));
        QTest::keyClick(pause, Qt::Key_Return);
        QVERIFY(!dialog.Sync.Get_Paused());
        background->setFocus();
        QTest::keyClick(background, Qt::Key_Return);
        QVERIFY(!NativeBackground());
        QCOMPARE(background->text(), QStringLiteral("&Background"));
        QVERIFY(!dialog.windowTitle().contains(QStringLiteral("Background")));
        QVERIFY(!main.windowTitle().contains(QStringLiteral("Background")));
        QCOMPARE(dialog.Sync.Messages.Size(), 0u);
        QVERIFY(UiInspection::writeReport(
            dialog, QString("progress-priority-%1-foreground").arg(size.width())));
    }
};

QTEST_MAIN(ProgressPriorityTests)
#include "progress_priority_tests.moc"
