#include "processpause.h"
#include <QCoreApplication>
#include <QFile>
#include <QProcess>
#include <QTemporaryDir>
#include <QTimer>
#include <QtTest>
#include <cstdio>

class ProcessPauseTests : public QObject {
    Q_OBJECT
  private slots:
    void ownedChildStopsResumesAndCanBeKilled() {
        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        const QString path = directory.filePath(QStringLiteral("worker с пробелом")
#ifdef _WIN32
                                                + QStringLiteral(".exe")
#endif
        );
        QVERIFY(QFile::copy(QCoreApplication::applicationFilePath(), path));
        QProcess child;
        child.start(path, {QStringLiteral("--pause-child")});
        QVERIFY(child.waitForStarted());
        QVERIFY(child.waitForReadyRead());
        CProcessPause pause;
        pause.Attach(UInt32(child.processId()));
        for (unsigned cycle = 0; cycle != 3; ++cycle) {
            QCOMPARE(pause.SetPaused(true), HRESULT(S_OK));
            QCOMPARE(pause.SetPaused(true), HRESULT(S_OK)); // Does not add a suspend count.
            QTest::qWait(50); // Drain data written before the native stop took effect.
            child.readAllStandardOutput();
            QTest::qWait(150);
            QCOMPARE(child.readAllStandardOutput(), QByteArray());
            QCOMPARE(child.state(), QProcess::Running);
            QCOMPARE(pause.SetPaused(false), HRESULT(S_OK));
            QCOMPARE(pause.SetPaused(false), HRESULT(S_OK));
            QVERIFY(child.waitForReadyRead());
            QVERIFY(!child.readAllStandardOutput().isEmpty());
        }
        QCOMPARE(pause.SetPaused(true), HRESULT(S_OK));
        child.kill();
        QVERIFY(child.waitForFinished());
        pause.ResetAfterExit();
        QCOMPARE(pause.SetPaused(true), HRESULT(E_INVALIDARG));
        QCOMPARE(pause.SetPaused(false), HRESULT(S_OK));
    }
    void invalidTargetFailsAndStateRemainsRetryable() {
        CProcessPause pause;
        pause.Attach(UInt32(-1));
        QVERIFY(pause.SetPaused(true) != S_OK);
        QCOMPARE(pause.SetPaused(false), HRESULT(S_OK));
        pause.ResetAfterExit();
        QCOMPARE(pause.SetPaused(true), HRESULT(E_INVALIDARG));
    }
};

int main(int argc, char **argv) {
    QCoreApplication app(argc, argv);
    if (app.arguments().contains(QStringLiteral("--pause-child"))) {
        QTimer timer;
        QObject::connect(&timer, &QTimer::timeout, [] {
            std::fputs("tick\n", stdout);
            std::fflush(stdout);
        });
        timer.start(20);
        return app.exec();
    }
    ProcessPauseTests tests;
    return QTest::qExec(&tests, argc, argv);
}
#include "process_pause_tests.moc"
