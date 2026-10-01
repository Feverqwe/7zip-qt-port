#include "CPP/7zip/UI/FileManager/ProgressDialog2.h"
#include "processpriority.h"
#include <QApplication>
#include <QFile>
#include <QLabel>
#include <QProcess>
#include <QPushButton>
#include <QSocketNotifier>
#include <QTemporaryDir>
#include <QtTest>
#include <cstdio>
#include <unistd.h>

namespace {
int Child(int argc, char **argv) {
    QApplication app(argc, argv);
    CProgressDialog dialog(nullptr, L"Ошибка приоритета");
    dialog.show();
    auto *button = dialog.findChild<QPushButton *>("IDB_PROGRESS_BACKGROUND");
    QSocketNotifier input(STDIN_FILENO, QSocketNotifier::Read);
    QObject::connect(&input, &QSocketNotifier::activated, &app, [&] {
        char command[16];
        if (!std::fgets(command, sizeof(command), stdin)) {
            app.exit(0);
            return;
        }
        if (command[0] == 'x') {
            app.exit(0);
            return;
        }
        button->click();
        QTest::qWait(220);
        // Source error presentation refreshes on elapsed-second changes.
        for (unsigned attempt = 0; attempt < 20 && !dialog.Sync.Messages.IsEmpty() &&
                                   !dialog.findChild<QLabel *>("IDT_PROGRESS_ERRORS")->isVisible();
             ++attempt)
            QTest::qWait(100);
        bool background = false;
        const HRESULT result = NProcessPriority::GetBackground(0, background);
        const QByteArray text = button->text().toUtf8();
        std::printf(
            "%u\t%d\t%u\t%d\t%s\n", unsigned(result), int(background), dialog.Sync.Messages.Size(),
            int(dialog.findChild<QLabel *>("IDT_PROGRESS_ERRORS")->isVisible()), text.constData());
        std::fflush(stdout);
    });
    std::printf("ready %d\n", int(button->isEnabled()));
    std::fflush(stdout);
    return app.exec();
}
class COwnedChild {
  public:
    QProcess process;
    ~COwnedChild() {
        if (process.state() != QProcess::NotRunning) {
            NProcessPriority::SetBackground(UInt32(process.processId()), false);
            process.kill();
            process.waitForFinished(3000);
        }
    }
};
QByteArray Line(QProcess &process) {
    for (unsigned attempt = 0; attempt < 10 && !process.canReadLine(); ++attempt)
        process.waitForReadyRead(500);
    return process.readLine().trimmed();
}
QByteArray Click(QProcess &process) {
    process.write("b\n");
    process.waitForBytesWritten(1000);
    return Line(process);
}
} // namespace

class ProgressPriorityFailureTests : public QObject {
    Q_OBJECT
  private slots:
    void nativeExternalPolicyRejectsForegroundAndAllowsRetry() {
        QTemporaryDir directory(QDir::tempPath() + "/приоритет с пробелом-XXXXXX");
        QVERIFY(directory.isValid());
        const QString executable = directory.filePath("дочерний процесс");
        QVERIFY(QFile::copy(QCoreApplication::applicationFilePath(), executable));
        COwnedChild child;
        child.process.setProgram(executable);
        child.process.setArguments({"--priority-failure-child"});
        child.process.start();
        QVERIFY(child.process.waitForStarted(3000));
        QCOMPARE(Line(child.process), QByteArray("ready 1"));
        const UInt32 pid = UInt32(child.process.processId());
        QCOMPARE(NProcessPriority::SetBackground(pid, true), S_OK);
        QCOMPARE(Click(child.process), QStringLiteral("0\t1\t0\t0\t&Foreground").toUtf8());
        QCOMPARE(Click(child.process), QStringLiteral("0\t1\t1\t1\t&Foreground").toUtf8());
        bool nativeBackground = false;
        QCOMPARE(NProcessPriority::GetBackground(pid, nativeBackground), S_OK);
        QVERIFY(nativeBackground);
        QCOMPARE(NProcessPriority::SetBackground(pid, false), S_OK);
        QCOMPARE(Click(child.process), QStringLiteral("0\t0\t1\t1\tВ &фоновый режим").toUtf8());
        child.process.write("x\n");
        QVERIFY(child.process.waitForFinished(3000));
        QCOMPARE(child.process.exitStatus(), QProcess::NormalExit);
        QCOMPARE(child.process.exitCode(), 0);
    }
};

int main(int argc, char **argv) {
    if (argc == 2 && QByteArray(argv[1]) == "--priority-failure-child")
        return Child(argc, argv);
    QApplication app(argc, argv);
    ProgressPriorityFailureTests tests;
    return QTest::qExec(&tests, argc, argv);
}
#include "progress_priority_failure_tests.moc"
