#include "filemanager/support/window_state_support.h"
#include "platform/associations/qtassociationfileopen.h"
#include <QFileOpenEvent>
#include <QScopeGuard>

class StartupPathTests final : public QObject, protected MainWindowTestSupport {
    Q_OBJECT
  private slots:
    void associationEventUsesTheExistingArchiveOpeningConsumer() {
        QTemporaryDir fixture;
        const QString path = fixture.filePath(QStringLiteral("архив 空 с пробелами.zip"));
        ControlledBackend backend;
        QVERIFY(backend.prepareArchiveFixture(path));
        backend.queueList({ArchiveEntry{QStringLiteral("payload.txt"), 4, 4}});
        QObject nativeEvents;
        QtAssociationFileOpen receiver(nativeEvents);
        QFileOpenEvent early(path);
        QVERIFY(QApplication::sendEvent(&nativeEvents, &early));
        MainWindow window(&backend);
        QObject::connect(&receiver, &QtAssociationFileOpen::OpenRequested, &window,
                         &MainWindow::openPath);
        receiver.Activate();
        QTRY_COMPARE(backend.listCalls, 1);
        QTRY_COMPARE(table(window)->rowCount(), 1);
        QCOMPARE(QDir::fromNativeSeparators(backend.lastArchive), path);
    }
    void directoryArgumentUsesNativeFolderOwnerWithoutArchiveProcess() {
        QTemporaryDir fixture;
        const QString folder = fixture.filePath(QStringLiteral("папка 空 с пробелами"));
        QVERIFY(QDir().mkpath(folder));
        QFile file(QDir(folder).filePath(QStringLiteral("данные.txt")));
        QVERIFY(file.open(QIODevice::WriteOnly));
        QCOMPARE(file.write("kept"), qint64(4));
        file.close();
        ControlledBackend backend;
        MainWindow window(&backend);
        window.openPath(folder);
        QTRY_VERIFY(table(window)->rowCount() >= 1);
        bool found = false;
        for (int row = 0; row < table(window)->rowCount(); ++row)
            found |= table(window)->model()->index(row, 0).data().toString() ==
                     QStringLiteral("данные.txt");
        QVERIFY(found);
        QCOMPARE(backend.listCalls, 0);
        QCOMPARE(QFileInfo(window.findChild<QComboBox *>("pathCombo")->currentText())
                     .canonicalFilePath(),
                 QFileInfo(folder).canonicalFilePath());
        QVERIFY(file.open(QIODevice::ReadOnly));
        QCOMPARE(file.readAll(), QByteArray("kept"));
    }
    void relativeArchiveArgumentUsesLaunchDirectory() {
        QTemporaryDir fixture;
        const QString name = QStringLiteral("архив 空 с пробелами.zip");
        ControlledBackend backend;
        QVERIFY(backend.prepareArchiveFixture(fixture.filePath(name)));
        MainWindow window(&backend);
        const QString previous = QDir::currentPath();
        const auto restore = qScopeGuard([&] { QDir::setCurrent(previous); });
        QVERIFY(QDir::setCurrent(fixture.path()));
        backend.queueList({ArchiveEntry{QStringLiteral("payload.txt"), 4, 4}});
        window.openPath(name);
        QTRY_COMPARE(backend.listCalls, 1);
        QTRY_COMPARE(table(window)->rowCount(), 1);
        QCOMPARE(QDir::fromNativeSeparators(backend.lastArchive), fixture.filePath(name));
    }
};
QTEST_MAIN(StartupPathTests)
#include "startup_path_tests.moc"
