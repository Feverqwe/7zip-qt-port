#include "Common/MyWindows.h"
#include "support/private_bus.h"
#include "support/taskbar_adapter_child.h"
#include <QDBusArgument>
#include <QDBusMessage>
#include <QDBusPendingCallWatcher>
#include <QFile>
#include <QProcessEnvironment>
#include <QtTest>

namespace {
const QString kInterface = QStringLiteral("com.canonical.Unity.LauncherEntry");
class CChild {
  public:
    QProcess Process;
    bool Start(const QString &executable, const QString &mode, const QString &address) {
        auto environment = QProcessEnvironment::systemEnvironment();
        environment.insert("DBUS_SESSION_BUS_ADDRESS", address);
        environment.insert("SEVENZIP_PRIVATE_DBUS_ADDRESS", address);
        environment.insert("QT_QPA_PLATFORM", "offscreen");
        Process.setProcessEnvironment(environment);
        Process.start(executable, {mode});
        return Process.waitForStarted(3000);
    }
    QByteArray Read() {
        for (unsigned i = 0; !Process.canReadLine() && i < 3; ++i)
            if (!Process.waitForReadyRead(1000))
                return {};
        return Process.readLine();
    }
    QByteArray Command(const QByteArray &command) {
        Process.write(command + '\n');
        Process.waitForBytesWritten(1000);
        return Read();
    }
    ~CChild() {
        if (Process.state() != QProcess::NotRunning) {
            Process.write("x\n");
            Process.waitForBytesWritten(1000);
            if (!Process.waitForFinished(1000)) {
                Process.kill();
                Process.waitForFinished(1000);
            }
        }
    }
};
} // namespace

class CAdapterObserver : public QObject {
    Q_OBJECT
  public:
    unsigned Updates = 0;
    QString Uri;
    QString Path;
    QString Service;
    QVariantMap Properties;
  public slots:
    void Receive(const QString &uri, const QVariantMap &properties, const QDBusMessage &message) {
        ++Updates;
        Uri = uri;
        Path = message.path();
        Service = message.service();
        Properties = properties;
    }
};

class TaskbarAdapterTests : public QObject {
    Q_OBJECT
    static QDBusMessage Query(const QDBusConnection &bus, const CAdapterObserver &observer) {
        const auto request =
            QDBusMessage::createMethodCall(observer.Service, observer.Path, kInterface, "Query");
        QDBusPendingCallWatcher watcher(bus.asyncCall(request));
        QSignalSpy finished(&watcher, &QDBusPendingCallWatcher::finished);
        if (!watcher.isFinished())
            finished.wait(3000);
        return watcher.isFinished()
                   ? watcher.reply()
                   : QDBusMessage::createError(QDBusError::Timeout, "Query timed out");
    }

  private slots:
    void sessionIdentityStatesValuesAndCleanup() {
        CPrivateBus bus;
        QVERIFY(bus.Start());
        auto listener = bus.Connect();
        QVERIFY(listener.isConnected());
        CAdapterObserver observer;
        QVERIFY(listener.connect(QString(), QString(), kInterface, "Update", &observer,
                                 SLOT(Receive(QString, QVariantMap, QDBusMessage))));
        QTemporaryDir temporary;
        QVERIFY(temporary.isValid());
        const QString executable = temporary.filePath(QStringLiteral("адаптер с пробелом"));
        QVERIFY(QFile::copy(QCoreApplication::applicationFilePath(), executable));
        CChild child;
        QVERIFY(child.Start(executable, "--adapter-child", bus.Address));
        QCOMPARE(child.Read(), QByteArray("0\n"));
        QTRY_COMPARE(observer.Updates, 1u);
        QCOMPARE(observer.Uri, QStringLiteral("application://sevenzip_desktop.desktop"));
        QCOMPARE(child.Command("s 2"), QByteArray("0\n"));
        QTRY_COMPARE(observer.Updates, 2u);
        QVERIFY(observer.Properties.value("progress-visible").toBool());
        QCOMPARE(child.Command("v 1 4"), QByteArray("0\n"));
        QTRY_COMPARE(observer.Updates, 3u);
        QCOMPARE(observer.Properties.value("progress").toDouble(), 0.25);
        QCOMPARE(child.Command("s 4"), QByteArray("0\n"));
        QTRY_COMPARE(observer.Updates, 4u);
        QVERIFY(observer.Properties.value("urgent").toBool());
        const auto reply = Query(listener, observer);
        QCOMPARE(reply.type(), QDBusMessage::ReplyMessage);
        QCOMPARE(reply.arguments().size(), 2);
        const auto before = qdbus_cast<QVariantMap>(reply.arguments()[1]);
        QCOMPARE(before.value("progress").toDouble(), 0.25);
        QVERIFY(before.value("urgent").toBool());
        const auto threadErrors = child.Command("t").trimmed().split(' ');
        QCOMPARE(threadErrors.size(), 3);
        for (const auto &error : threadErrors)
            QVERIFY(error != "0");
        QCOMPARE(child.Command("n"), QByteArray::number(qint64(E_INVALIDARG)) + '\n');
        QCOMPARE(child.Command("v 1 0"), QByteArray::number(qint64(E_INVALIDARG)) + '\n');
        QVERIFY(child.Command("s 1") != QByteArray("0\n"));
        const auto unchanged = Query(listener, observer);
        QCOMPARE(unchanged.type(), QDBusMessage::ReplyMessage);
        QCOMPARE(qdbus_cast<QVariantMap>(unchanged.arguments()[1]), before);
        QCOMPARE(observer.Updates, 4u);
        QCOMPARE(child.Command("s 8"), QByteArray("0\n"));
        QTRY_COMPARE(observer.Updates, 5u);
        QVERIFY(!observer.Properties.value("urgent").toBool());
        QCOMPARE(child.Command("d"), QByteArray("0\n"));
        QTRY_COMPARE(observer.Updates, 6u);
        QVERIFY(!observer.Properties.value("progress-visible").toBool());
        QCOMPARE(Query(listener, observer).type(), QDBusMessage::ErrorMessage);
        QCOMPARE(child.Command("x"), QByteArray("0\n"));
        QVERIFY(child.Process.waitForFinished(3000));
        QCOMPARE(child.Process.exitCode(), 0);
    }
    void absentGuiInvalidIdentityAndUnavailableSession() {
        QTemporaryDir temporary;
        QVERIFY(temporary.isValid());
        const QString unavailable = "unix:path=" + temporary.filePath("missing-socket");
        for (const auto &mode : {"--no-gui", "--empty-id", "--invalid-id", "--unavailable"}) {
            CChild child;
            QVERIFY(child.Start(QCoreApplication::applicationFilePath(), QString::fromLatin1(mode),
                                unavailable));
            QVERIFY(child.Process.waitForFinished(3000));
            QCOMPARE(child.Process.exitCode(), 0);
            if (QByteArray(mode) != "--no-gui")
                QVERIFY(child.Process.readAllStandardOutput().trimmed() != "0");
        }
    }
};

int main(int argc, char **argv) {
    if (argc == 2) {
        const QByteArray mode(argv[1]);
        if (mode == "--adapter-child" || mode == "--no-gui" || mode == "--empty-id" ||
            mode == "--invalid-id" || mode == "--unavailable")
            return RunTaskbarAdapterChild(argc, argv);
    }
    QCoreApplication app(argc, argv);
    TaskbarAdapterTests tests;
    return QTest::qExec(&tests, argc, argv);
}
#include "taskbar_adapter_tests.moc"
