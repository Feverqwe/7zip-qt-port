#include "qttaskbarprogress.h"
#import <AppKit/AppKit.h>
#include <QApplication>
#include <QProcess>
#include <QThread>
#include <QWidget>
#include <QtTest>

namespace {
constexpr HRESULT S_OK = 0;
QString Badge() { return QString::fromUtf8(NSApp.dockTile.badgeLabel.UTF8String); }
class CRestoreBadge {
    NSString *_original;

  public:
    CRestoreBadge() : _original([NSApp.dockTile.badgeLabel copy]) {}
    ~CRestoreBadge() {
        NSApp.dockTile.badgeLabel = _original;
        [_original release];
    }
};
} // namespace

class TaskbarProgressMacTests : public QObject {
    Q_OBJECT
  private slots:
    void initTestCase() {
        [NSApplication sharedApplication];
        QVERIFY(NSApp.dockTile != nil);
    }
    void absentCocoaApplicationReportsUnavailable() {
        QProcess child;
        child.start(QCoreApplication::applicationFilePath(), {"--no-cocoa"});
        QVERIFY(child.waitForStarted(3000));
        QVERIFY(child.waitForFinished(3000));
        QCOMPARE(child.exitStatus(), QProcess::NormalExit);
        QCOMPARE(child.exitCode(), 0);
    }
    void nonGuiThreadFailsWithoutChangingNativeBadge() {
        CRestoreBadge restore;
        NSApp.dockTile.badgeLabel = @"GUI owner";
        QWidget window;
        CQtTaskbarProgress progress;
        QCOMPARE(progress.HrInit(), S_OK);
        HRESULT initialize = S_OK, state = S_OK, value = S_OK;
        QThread *worker = QThread::create([&] {
            initialize = progress.HrInit();
            state = progress.SetProgressState(&window, TBPF_NORMAL);
            value = progress.SetProgressValue(&window, 1, 2);
        });
        worker->start();
        worker->wait();
        delete worker;
        QVERIFY(initialize != S_OK);
        QVERIFY(state != S_OK);
        QVERIFY(value != S_OK);
        QCOMPARE(Badge(), QStringLiteral("GUI owner"));
    }
    void nativeStatesValuesAndClear() {
        CRestoreBadge restore;
        NSApp.dockTile.badgeLabel = @"prior";
        QWidget window;
        CQtTaskbarProgress progress;
        QCOMPARE(progress.HrInit(), S_OK);
        QCOMPARE(progress.HrInit(), S_OK);
        QCOMPARE(progress.SetProgressState(&window, TBPF_INDETERMINATE), S_OK);
        QCOMPARE(Badge(), QStringLiteral("…"));
        QCOMPARE(progress.SetProgressValue(&window, 1, 4), S_OK);
        QCOMPARE(Badge(), QStringLiteral("25%"));
        QCOMPARE(progress.SetProgressState(&window, TBPF_PAUSED), S_OK);
        QCOMPARE(Badge(), QStringLiteral("Ⅱ 25%"));
        QCOMPARE(progress.SetProgressValue(&window, 3, 4), S_OK);
        QCOMPARE(Badge(), QStringLiteral("Ⅱ 75%"));
        QCOMPARE(progress.SetProgressState(&window, TBPF_ERROR), S_OK);
        QCOMPARE(Badge(), QStringLiteral("! 75%"));
        QCOMPARE(progress.SetProgressValue(&window, ~UInt64(0), ~UInt64(0)), S_OK);
        QCOMPARE(Badge(), QStringLiteral("! 100%"));
        QCOMPARE(progress.SetProgressState(&window, TBPF_NORMAL), S_OK);
        QCOMPARE(Badge(), QStringLiteral("100%"));
        QCOMPARE(progress.SetProgressValue(&window, 100, 4), S_OK);
        QCOMPARE(Badge(), QStringLiteral("100%"));
        QCOMPARE(progress.SetProgressState(&window, TBPF_NOPROGRESS), S_OK);
        QCOMPARE(Badge(), QStringLiteral("prior"));
        QCOMPARE(progress.SetProgressState(&window, TBPF_NOPROGRESS), S_OK);
        QCOMPARE(Badge(), QStringLiteral("prior"));
    }
    void failureAndDestructorOwnership() {
        CRestoreBadge restore;
        NSApp.dockTile.badgeLabel = @"existing";
        QWidget window;
        {
            CQtTaskbarProgress progress;
            QVERIFY(progress.SetProgressState(&window, TBPF_NORMAL) != S_OK);
            QCOMPARE(Badge(), QStringLiteral("existing"));
            QCOMPARE(progress.HrInit(), S_OK);
            QCOMPARE(progress.SetProgressState(nullptr, TBPF_NORMAL), E_INVALIDARG);
            QCOMPARE(progress.SetProgressState(&window, TBPFLAG(TBPF_INDETERMINATE | TBPF_NORMAL)),
                     E_INVALIDARG);
            QCOMPARE(progress.SetProgressValue(&window, 0, 0), E_INVALIDARG);
            QCOMPARE(Badge(), QStringLiteral("existing"));
            QCOMPARE(progress.SetProgressValue(&window, 0, 4), S_OK);
            QCOMPARE(Badge(), QStringLiteral("0%"));
        }
        QCOMPARE(Badge(), QStringLiteral("existing"));
        {
            CQtTaskbarProgress progress;
            QCOMPARE(progress.HrInit(), S_OK);
            QCOMPARE(progress.SetProgressValue(&window, 1, 2), S_OK);
            NSApp.dockTile.badgeLabel = @"other owner";
        }
        QCOMPARE(Badge(), QStringLiteral("other owner"));
    }
};

int main(int argc, char **argv) {
    if (argc == 2 && QByteArray(argv[1]) == "--no-cocoa") {
        QCoreApplication app(argc, argv);
        CQtTaskbarProgress progress;
        if (NSApp || progress.HrInit() == S_OK)
            return 1;
        return 0;
    }
    QApplication app(argc, argv);
    TaskbarProgressMacTests tests;
    return QTest::qExec(&tests, argc, argv);
}
#include "taskbar_progress_mac_tests.moc"
