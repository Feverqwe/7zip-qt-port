#include "archivebackend.h"
#include "fixtures/widget_dialogs.h"
#include "mainwindow.h"
#include "qtassociationfileopen.h"
#include "qtfilemanagerstartup.h"
#include "source_extract_fixture.h"
#include <QFileOpenEvent>
#include <QMessageBox>
#include <QPointer>
#include <QScopeGuard>
#include <QTableWidget>
#include <QTemporaryDir>
#include <QtTest>

class AssociatedWindowTests final : public QObject {
    Q_OBJECT
    WidgetDialogFixture dialogs_;
    QList<QPointer<MainWindow>> windows_;

  private slots:
    void init() { QFile::remove(qEnvironmentVariable("SEVENZIP_SETTINGS_PATH")); }
    void cleanup() {
        for (auto &window : windows_)
            delete window.data();
        windows_.clear();
    }
    void earlyAndRepeatedRequestsOpenRealArchivesAfterDiscovery() {
        QTemporaryDir fixture;
        QVERIFY(fixture.isValid());
        const auto closeWindows = qScopeGuard([this] { cleanup(); });
        const QString name = QStringLiteral("данные 空 with spaces.bin");
        const QByteArray payload("associated file bytes\0\xff", 23);
        QVERIFY(SourceExtractFixture::writeFile(fixture.filePath(name), payload));
        const QString archive = fixture.filePath(QStringLiteral("архив 空 with spaces.zip"));
        QProcess engine;
        engine.setWorkingDirectory(fixture.path());
        engine.start(QStandardPaths::findExecutable("7zz"), {"a", "-tzip", archive, name});
        QVERIFY(engine.waitForFinished(10000));
        QCOMPARE(engine.exitCode(), 0);
        const auto original = SourceExtractFixture::readFile(archive);
        QStringList errors;
        QObject responder;
        dialogs_.respond(responder, [&](QDialog &dialog) {
            if (auto *message = qobject_cast<QMessageBox *>(&dialog)) {
                errors.append(message->text());
                message->accept();
            }
        });
        QObject source;
        QtAssociationFileOpen events(source);
        connect(&events, &QtAssociationFileOpen::OpenRequested, this,
                [&](const QString &path) { windows_.append(OpenSevenzipAssociatedFile(path)); });
        QFileOpenEvent early(archive);
        QVERIFY(QApplication::sendEvent(&source, &early));
        events.Activate();
        QTRY_COMPARE(windows_.size(), 1);
        QTRY_VERIFY(!windows_[0].isNull());
        auto *first = windows_[0]->findChild<QTableWidget *>("archiveTable");
        QVERIFY(first);
        QTRY_VERIFY(first->rowCount() == 1 && first->item(0, 0)->text() == name);
        QVERIFY2(errors.isEmpty(), qPrintable(errors.join('\n')));

        QFileOpenEvent repeated(archive);
        QVERIFY(QApplication::sendEvent(&source, &repeated));
        QTRY_COMPARE(windows_.size(), 2);
        QVERIFY(windows_[0] != windows_[1]);
        auto *second = windows_[1]->findChild<QTableWidget *>("archiveTable");
        QVERIFY(second);
        QTRY_VERIFY(second->rowCount() == 1 && second->item(0, 0)->text() == name);
        QVERIFY2(errors.isEmpty(), qPrintable(errors.join('\n')));
        QCOMPARE(first->item(0, 0)->text(), name);
        QCOMPARE(SourceExtractFixture::readFile(archive), original);
        QCOMPARE(SourceExtractFixture::readFile(fixture.filePath(name)), payload);
        QVERIFY(windows_[1]->close());
        QTRY_VERIFY(windows_[1].isNull());
        QCOMPARE(first->item(0, 0)->text(), name);
    }
    void closeDuringDiscoveryJoinsAndDoesNotOpenAnArchive() {
        QTemporaryDir fixture;
        QVERIFY(fixture.isValid());
        const QString suffix = QFileInfo(QString::fromUtf8(CODEC_PROBE_FIXTURE)).suffix();
        const QString engine =
            fixture.filePath("probe hang" + (suffix.isEmpty() ? "" : "." + suffix));
        QVERIFY(QFile::copy(QString::fromUtf8(CODEC_PROBE_FIXTURE), engine));
        const QByteArray previous = qgetenv("SEVENZIP_ENGINE_PATH");
        const bool wasSet = qEnvironmentVariableIsSet("SEVENZIP_ENGINE_PATH");
        const auto restore = qScopeGuard([&] {
            if (wasSet)
                qputenv("SEVENZIP_ENGINE_PATH", previous);
            else
                qunsetenv("SEVENZIP_ENGINE_PATH");
        });
        qputenv("SEVENZIP_ENGINE_PATH", engine.toUtf8());
        QPointer<MainWindow> window =
            OpenSevenzipAssociatedFile(fixture.filePath("not-opened.zip"));
        windows_.append(window);
        auto *backend = window->findChild<ArchiveBackend *>();
        QVERIFY(backend);
        QVERIFY(backend->busy());
        QSignalSpy completed(backend, &ArchiveBackend::finished);
        QVERIFY(window->close());
        QTRY_VERIFY(window.isNull());
        QCOMPARE(completed.count(), 0);
        QVERIFY(!QFileInfo::exists(fixture.filePath("not-opened.zip")));
    }
    void discoveryFailureReportsItsErrorWithoutStartingArchiveRead() {
        QTemporaryDir fixture;
        QVERIFY(fixture.isValid());
        const auto closeWindows = qScopeGuard([this] { cleanup(); });
        const QByteArray previous = qgetenv("SEVENZIP_ENGINE_PATH");
        const bool wasSet = qEnvironmentVariableIsSet("SEVENZIP_ENGINE_PATH");
        const auto restore = qScopeGuard([&] {
            if (wasSet)
                qputenv("SEVENZIP_ENGINE_PATH", previous);
            else
                qunsetenv("SEVENZIP_ENGINE_PATH");
        });
        qputenv("SEVENZIP_ENGINE_PATH", fixture.filePath("missing-engine").toUtf8());
        QString message;
        QObject responder;
        dialogs_.respond(responder, [&](QDialog &dialog) {
            if (auto *error = qobject_cast<QMessageBox *>(&dialog)) {
                message = error->text();
                QCOMPARE(error->icon(), QMessageBox::Critical);
                error->accept();
            }
        });
        auto *window = OpenSevenzipAssociatedFile(fixture.filePath("not-opened.zip"));
        windows_.append(window);
        auto *backend = window->findChild<ArchiveBackend *>();
        QVERIFY(backend);
        QSignalSpy readCompleted(backend, &ArchiveBackend::finished);
        QTRY_VERIFY(!message.isEmpty());
        QVERIFY(window->isVisible());
        QVERIFY(!backend->busy());
        QCOMPARE(readCompleted.count(), 0);
    }
};
QTEST_MAIN(AssociatedWindowTests)
#include "associated_window_tests.moc"
