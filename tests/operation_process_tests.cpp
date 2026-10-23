#include "qtoperationprocess.h"
#include <QCoreApplication>
#include <QFile>
#include <QProcess>
#include <QSaveFile>
#include <QTemporaryDir>
#include <QTimer>
#include <QtTest>
#ifdef Q_OS_WIN
#include <windows.h>
#else
#include <csignal>
#include <unistd.h>
#endif

class OperationProcessTests final : public QObject {
    Q_OBJECT
  private slots:
    void exits_data() {
        QTest::addColumn<QString>("mode");
        QTest::addColumn<int>("code");
        QTest::newRow("success") << QString("success") << 0;
        QTest::newRow("operation failure") << QString("failure") << 2;
        QTest::newRow("cancellation exit") << QString("cancelled") << 255;
    }
    void exits() {
        QFETCH(QString, mode);
        QFETCH(int, code);
        QtOperationProcess process;
        QString error;
        QVERIFY2(process.start(QCoreApplication::applicationFilePath(), mode, error),
                 qPrintable(error));
        QTRY_COMPARE(process.poll().Status, QtOperationProcess::State::Exited);
        QCOMPARE(process.poll().Code, quint32(code));
    }
    void unavailableProgram() {
        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        QtOperationProcess process;
        QString error;
        QVERIFY(!process.start(directory.filePath("missing 7zG"), "channel", error));
        QVERIFY(!error.isEmpty());
        QCOMPARE(process.poll().Status, QtOperationProcess::State::Idle);
        QVERIFY(process.start(QCoreApplication::applicationFilePath(), "success", error));
        QTRY_COMPARE(process.poll().Status, QtOperationProcess::State::Exited);
        QCOMPARE(process.poll().Code, quint32(0));
    }
    void crashIsNotAnOperationExit() {
        QtOperationProcess process;
        QString error;
        QVERIFY(process.start(QCoreApplication::applicationFilePath(), "crash", error));
        QTRY_COMPARE(process.poll().Status, QtOperationProcess::State::Crashed);
        QVERIFY(process.poll().Code != 0);
    }
    void ownerExitDoesNotTerminateAcceptedWork() {
        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        const QString output = directory.filePath(QStringLiteral("итог 空 with spaces.txt"));
        QProcess parent;
        auto environment = QProcessEnvironment::systemEnvironment();
        environment.insert("SEVENZIP_OPERATION_PROBE_OUTPUT", output);
        parent.setProcessEnvironment(environment);
        parent.start(QCoreApplication::applicationFilePath(), {"--probe-parent"});
        QVERIFY(parent.waitForStarted(5000));
        QVERIFY(parent.waitForFinished(5000));
        QCOMPARE(parent.exitStatus(), QProcess::NormalExit);
        QCOMPARE(parent.exitCode(), 0);
        QTRY_VERIFY(QFileInfo::exists(output));
        QFile file(output);
        QVERIFY(file.open(QIODevice::ReadOnly));
        QCOMPARE(file.readAll(), QByteArray("completed after parent exit"));
    }
};

int main(int argc, char **argv) {
    QCoreApplication application(argc, argv);
    const auto arguments = application.arguments();
    if (arguments.contains("--probe-parent")) {
        QtOperationProcess operation;
        QString error;
        return operation.start(application.applicationFilePath(), "survive", error) ? 0 : 1;
    }
    const int option = arguments.indexOf("--operation-channel");
    if (option >= 0) {
        const QString mode = arguments.value(option + 1);
        if (mode == "crash") {
#ifdef Q_OS_WIN
            TerminateProcess(GetCurrentProcess(), 0xc0000005u);
#else
            ::kill(::getpid(), SIGKILL);
#endif
            return 1;
        }
        if (mode == "survive") {
            QTimer::singleShot(250, &application, [&] {
                QSaveFile output(qEnvironmentVariable("SEVENZIP_OPERATION_PROBE_OUTPUT"));
                if (!output.open(QIODevice::WriteOnly) ||
                    output.write("completed after parent exit") != 27 || !output.commit())
                    application.exit(1);
                else
                    application.exit(0);
            });
            return application.exec();
        }
        return mode == "success" ? 0 : mode == "cancelled" ? 255 : 2;
    }
    OperationProcessTests tests;
    return QTest::qExec(&tests, argc, argv);
}

#include "operation_process_tests.moc"
