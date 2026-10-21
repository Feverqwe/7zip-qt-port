#include "CPP/7zip/UI/FileManager/RegistryUtils.h"
#include "archivebackend.h"
#include "exfat_fixture.h"
#include "layout_inspection.h"
#include "mainwindow.h"
#include <QAction>
#include <QFile>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QSignalSpy>
#include <QTableWidget>
#include <QTemporaryDir>
#include <QTimer>
#include <QtTest>

namespace {
int Row(QTableWidget &table, const QString &name) {
    for (int i = 0; i < table.rowCount(); ++i)
        if (table.item(i, 0) && table.item(i, 0)->text() == name)
            return i;
    return -1;
}
} // namespace

class FilesystemReadUiTests final : public QObject {
    Q_OBJECT
    QTemporaryDir runtime_;
    QString engine_, module_;
    QByteArray previousSettings_;
  private slots:
    void initTestCase() {
        QVERIFY(runtime_.isValid());
        previousSettings_ = qgetenv("SEVENZIP_SETTINGS_PATH");
        qputenv("SEVENZIP_SETTINGS_PATH", runtime_.filePath("settings.ini").toUtf8());
        const QDir prepared(QStringLiteral(SEVENZIP_EXFAT_RUNTIME));
#ifdef Q_OS_WIN
        const QString engineName = "7z.exe", coreName = "7z.dll", suffix = ".dll";
#else
        const QString engineName = "7z", coreName = "7z.so", suffix = ".so";
#endif
        engine_ = runtime_.filePath(engineName);
        module_ = runtime_.filePath(coreName);
        QVERIFY(QFile::copy(prepared.filePath(engineName), engine_));
        QVERIFY(QFile::copy(prepared.filePath(coreName), module_));
        QVERIFY(QDir(runtime_.path()).mkdir("Formats"));
        QVERIFY(QFile::copy(QStringLiteral(SEVENZIP_EXFAT_MODULE),
                            runtime_.filePath("Formats/exfat" + suffix)));
    }
    void cleanupTestCase() { qputenv("SEVENZIP_SETTINGS_PATH", previousSettings_); }
    void confirmsAndCopiesFlaggedVolume_data() {
        QTest::addColumn<QString>("language");
        QTest::addColumn<bool>("accept");
        for (const QString &language : {"en", "ru"})
            for (bool accept : {false, true})
                QTest::newRow(qPrintable(language + (accept ? "-accept" : "-cancel")))
                    << language << accept;
    }
    void confirmsAndCopiesFlaggedVolume() {
        QFETCH(QString, language);
        QFETCH(bool, accept);
        SaveRegLang(UString(language == "en" ? L"-" : L"ru"),
                    qEnvironmentVariable("SEVENZIP_SETTINGS_PATH"));
        TexfatFixture fixture(language == "en" ? 0 : 1);
        fixture.image[106] = char(language == "en" ? 2 : 5);
        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        const QString path = directory.filePath(QStringLiteral("том 空.img"));
        QFile source(path);
        QVERIFY(source.open(QIODevice::WriteOnly));
        QCOMPARE(source.write(fixture.image), qint64(fixture.image.size()));
        source.close();
        ArchiveBackend backend(nullptr, engine_);
        QSignalSpy ready(&backend, &ArchiveBackend::codecCatalogReady);
        QVERIFY(backend.discoverCodecCatalog(module_));
        QTRY_COMPARE(ready.size(), 1);
        QVERIFY(ready[0][0].toBool());
        MainWindow window(&backend);
        window.openPath(directory.path());
        window.show();
        int questions = 0;
        bool geometry = true;
        QStringList unexpectedErrors;
        QPointer<QMessageBox> handled;
        QTimer automation;
        connect(&automation, &QTimer::timeout, &window, [&] {
            QMessageBox *message = nullptr;
            for (QWidget *widget : QApplication::topLevelWidgets())
                if (auto *box = qobject_cast<QMessageBox *>(widget);
                    box && box != handled &&
                    (box->isVisible() || box->objectName() == "filesystemReadLimitationsWarning")) {
                    message = box;
                    break;
                }
            if (!message)
                return;
            handled = message;
            if (message->objectName() != "filesystemReadLimitationsWarning") {
                unexpectedErrors.append(message->text());
                message->reject();
                return;
            }
            ++questions;
            message->layout()->activate();
            for (auto *label : message->findChildren<QLabel *>())
                if (label->isVisible())
                    geometry &= UiInspection::fullyVisible(*label, *message) &&
                                UiInspection::labelFits(*label);
            message->setProperty("uiInspection_language", language);
            message->setProperty("uiInspection_state", "flagged-exfat");
            geometry &= UiInspection::writeReport(*message, "filesystem-limitations-" + language);
            if (accept)
                message->findChild<QPushButton *>("deviceOpenWithoutRecoveryButton")->click();
            else
                QTest::keyClick(message, language == "en" ? Qt::Key_Return : Qt::Key_Escape);
        });
        automation.start(10);
        window.openArchive(path);
        QVERIFY2(unexpectedErrors.isEmpty(), qPrintable(unexpectedErrors.join('\n')));
        QCOMPARE(questions, 1);
        QVERIFY(geometry);
        auto *badge = window.findChild<QLabel *>("deviceReadOnlyLabel");
        QVERIFY(badge);
        if (!accept) {
            QVERIFY(!badge->isVisible());
        } else {
            QTRY_VERIFY(badge->isVisible());
            QCOMPARE(badge->text(),
                     QCoreApplication::translate("mainwindow_panel", "Read only · limited access"));
            for (QSize size : {QSize(980, 620), QSize(640, 400)}) {
                window.resize(size);
                QCoreApplication::processEvents();
                QVERIFY(UiInspection::fullyVisible(*badge, window));
                QVERIFY(UiInspection::labelFits(*badge));
                window.setProperty("uiInspection_language", language);
                QVERIFY(UiInspection::writeReport(window, "filesystem-limited-" + language + '-' +
                                                              QString::number(size.width())));
            }
            auto *table = window.findChild<QTableWidget *>("archiveTable");
            QVERIFY(table);
            QTRY_VERIFY(Row(*table, fixture.fragmentedName) >= 0);
            table->selectRow(Row(*table, fixture.fragmentedName));
            for (const char *name : {"deleteAction", "renameAction", "moveAction"})
                QVERIFY(!window.findChild<QAction *>(name)->isEnabled());
            const QString output = directory.filePath("copied");
            window.extractTo(output, true, {}, {});
            QFile copied(output + '/' + fixture.fragmentedName);
            QVERIFY(copied.open(QIODevice::ReadOnly));
            QCOMPARE(copied.readAll(), QByteArray(512, 'A') + QByteArray(188, 'B'));
            window.findChild<QAction *>("refreshAction")->trigger();
            QTRY_VERIFY(Row(*table, fixture.fragmentedName) >= 0);
            QCOMPARE(questions, 1);
            QVERIFY(badge->isVisible());
            window.openPath(directory.path());
            QVERIFY(!badge->isVisible());
            // Change our fixture after closing the old Windows read lease.
            fixture.image[106] = char(quint8(fixture.image[106]) | 6u);
            QFile changed(path);
            QVERIFY(changed.open(QIODevice::ReadWrite));
            QVERIFY(changed.seek(106));
            QCOMPARE(changed.write(fixture.image.constData() + 106, 1), qint64(1));
            changed.close();
            window.openArchive(path);
            QCOMPARE(questions, 2);
        }
        automation.stop();
        QVERIFY2(unexpectedErrors.isEmpty(), qPrintable(unexpectedErrors.join('\n')));
        QVERIFY(source.open(QIODevice::ReadOnly));
        QCOMPARE(source.readAll(), fixture.image);
    }
};

QTEST_MAIN(FilesystemReadUiTests)
#include "filesystem_read_ui_tests.moc"
