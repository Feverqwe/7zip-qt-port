#include "CPP/7zip/UI/FileManager/ProgressDialog2.h"
#include "fixtures/dock_observer.h"
#include <QPushButton>
#include <QtTest>

namespace {
class CRestoreBadge {
    QString _original = NativeDockBadge();

  public:
    ~CRestoreBadge() { SetNativeDockBadge(_original); }
};
QPushButton *Pause(CProgressDialog &dialog) { return dialog.findChild<QPushButton *>("IDB_PAUSE"); }
} // namespace

class ProgressTaskbarTests : public QObject {
    Q_OBJECT
  private slots:
    void initTestCase() { QVERIFY(EnsureCocoaDock()); }
    void sourceCounterErrorPauseAndDestructorClear() {
        CRestoreBadge restore;
        SetNativeDockBadge("prior source");
        {
            CProgressDialog dialog(nullptr, L"Обновление");
            dialog.Sync.Set_NumBytesTotal(100);
            QCOMPARE(dialog.Sync.Set_NumBytesCur(25), S_OK);
            dialog.show();
            QTRY_COMPARE(NativeDockBadge(), QStringLiteral("25%"));
            dialog.Sync.AddError_Message(L"Ошибка файла с пробелом");
            QTRY_COMPARE(NativeDockBadge(), QStringLiteral("! 25%"));
            QCOMPARE(dialog.Sync.Set_NumBytesCur(50), S_OK);
            QTRY_COMPARE(NativeDockBadge(), QStringLiteral("! 50%"));
            Pause(dialog)->click();
            QCOMPARE(NativeDockBadge(), QStringLiteral("Ⅱ 50%"));
            QTest::qWait(250);
            QCOMPARE(NativeDockBadge(), QStringLiteral("Ⅱ 50%"));
            Pause(dialog)->click();
            QCOMPARE(dialog.Sync.Set_NumBytesCur(75), S_OK);
            QTRY_COMPARE(NativeDockBadge(), QStringLiteral("! 75%"));
            QCOMPARE(dialog.Sync.Set_NumBytesCur(100), S_OK);
            QTRY_COMPARE(NativeDockBadge(), QStringLiteral("! 100%"));
        }
        QCOMPARE(NativeDockBadge(), QStringLiteral("prior source"));
    }
    void sourceTargetBranches_data() {
        QTest::addColumn<int>("target");
        QTest::newRow("main-window") << 0;
        QTest::newRow("parent-window") << 1;
        QTest::newRow("dialog-window") << 2;
    }
    void sourceTargetBranches() {
        QFETCH(int, target);
        CRestoreBadge restore;
        SetNativeDockBadge("prior target");
        QWidget main, parent;
        {
            CProgressDialog dialog(target == 1 ? &parent : nullptr, L"Проверка");
            if (target == 0)
                dialog.MainWindow = &main;
            dialog.Sync.Set_NumBytesTotal(8);
            QCOMPARE(dialog.Sync.Set_NumBytesCur(4), S_OK);
            dialog.show();
            QTRY_COMPARE(NativeDockBadge(), QStringLiteral("50%"));
        }
        QCOMPARE(NativeDockBadge(), QStringLiteral("prior target"));
    }
    void preInitAndDeletedTargetLifetime() {
        CRestoreBadge restore;
        SetNativeDockBadge("uncreated source");
        {
            CProgressDialog dialog;
            dialog.Sync.Set_NumBytesTotal(100);
            QCOMPARE(dialog.Sync.Set_NumBytesCur(25), S_OK);
        }
        QCOMPARE(NativeDockBadge(), QStringLiteral("uncreated source"));
        {
            auto *main = new QWidget;
            CProgressDialog dialog;
            dialog.MainWindow = main;
            dialog.Sync.Set_NumBytesTotal(100);
            QCOMPARE(dialog.Sync.Set_NumBytesCur(25), S_OK);
            dialog.show();
            QTRY_COMPARE(NativeDockBadge(), QStringLiteral("25%"));
            delete main;
            QVERIFY(dialog.MainWindow.isNull());
        }
        QCOMPARE(NativeDockBadge(), QStringLiteral("uncreated source"));
    }
};

QTEST_MAIN(ProgressTaskbarTests)
#include "progress_taskbar_tests.moc"
