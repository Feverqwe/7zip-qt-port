#include "CPP/7zip/UI/FileManager/RegistryUtils.h"
#include "archivebackend.h"
#include "exfat_fixture.h"
#include "layout_inspection.h"
#include "mainwindow.h"
#include <QAction>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QMessageBox>
#include <QProcess>
#include <QPushButton>
#include <QSignalSpy>
#include <QTableWidget>
#include <QTemporaryDir>
#include <QTimer>
#include <QtEndian>
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
        QVERIFY(QFile::copy(QStringLiteral(SEVENZIP_HFS_MODULE),
                            runtime_.filePath("Formats/hfs" + suffix)));
        QVERIFY(QFile::copy(QStringLiteral(SEVENZIP_NTFS_MODULE),
                            runtime_.filePath("Formats/ntfs" + suffix)));
        QVERIFY(QFile::copy(QStringLiteral(SEVENZIP_EXT_MODULE),
                            runtime_.filePath("Formats/ext" + suffix)));
    }
    void cleanupTestCase() { qputenv("SEVENZIP_SETTINGS_PATH", previousSettings_); }
    void confirmsAndCopiesFlaggedVolume_data() {
        QTest::addColumn<QString>("language");
        QTest::addColumn<bool>("accept");
        QTest::addColumn<QString>("kind");
        for (const QString &language : {"en", "ru"})
            for (bool accept : {false, true})
                for (const QString &kind : {"exfat",
                                            "wrapper-no-slock",
                                            "unclean",
                                            "free-overflow",
                                            "classic-unclean-free-overflow",
                                            "volume-dirty",
                                            "volume-short",
                                            "volume-list-nonresident",
                                            "reparse-list-bad-id",
                                            "ea-record-small",
                                            "data-list-unreadable-list",
                                            "standard-short",
                                            "ext-recover",
                                            "ext-orphan",
                                            "ext-unclean",
                                            "ext-unverified",
                                            "ext-combined",
                                            "dir-root-missing",
                                            "record-total",
                                            "security-root-offset",
                                            "security-sd-short"}) {
                    // Rejection uses the shared consent service; acceptance checks every reason.
                    if (!accept && kind != "exfat" && kind != "unclean" && kind != "volume-dirty" &&
                        kind != "ext-recover")
                        continue;
                    QTest::newRow(qPrintable(language + (accept ? "-accept-" : "-cancel-") + kind))
                        << language << accept << kind;
                }
    }
    void confirmsAndCopiesFlaggedVolume() {
        QFETCH(QString, language);
        QFETCH(bool, accept);
        QFETCH(QString, kind);
        const bool ntfs = kind.startsWith("volume-") || kind.startsWith("reparse-") ||
                          kind.startsWith("ea-") || kind.startsWith("data-list-") ||
                          kind.startsWith("standard-") || kind.startsWith("dir-") ||
                          kind.startsWith("record-") || kind.startsWith("security-");
        const bool ext = kind.startsWith("ext-");
        SaveRegLang(UString(language == "en" ? L"-" : L"ru"),
                    qEnvironmentVariable("SEVENZIP_SETTINGS_PATH"));
        TexfatFixture fixture(language == "en" ? 0 : 1);
        fixture.image[106] = char(language == "en" ? 2 : 5);
        QByteArray image = fixture.image;
        QString filename = fixture.fragmentedName;
        QByteArray payload = QByteArray(512, 'A') + QByteArray(188, 'B');
        if (kind != "exfat") {
            QProcess generator;
            generator.start(
                QStringLiteral(SEVENZIP_HFS_PYTHON),
                ext ? QStringList{QStringLiteral(SEVENZIP_EXT_FIXTURE), "readonly-" + kind.mid(4),
                                  "--bundle"}
                : ntfs ? QStringList{QStringLiteral(SEVENZIP_NTFS_FIXTURE), "--kind=" + kind}
                       : QStringList{QStringLiteral(SEVENZIP_HFS_FIXTURE), kind});
            QVERIFY(generator.waitForFinished(10000));
            QCOMPARE(generator.exitCode(), 0);
            const QJsonObject data =
                QJsonDocument::fromJson(generator.readAllStandardOutput()).object();
            QByteArray packed(4, '\0');
            qToBigEndian(quint32(data["size"].toInt()), packed.data());
            packed += QByteArray::fromBase64(data["image"].toString().toLatin1());
            image = qUncompress(packed);
            QVERIFY(!image.isEmpty());
            filename = ext    ? data["name"].toString()
                       : ntfs ? QStringLiteral("alpha")
                              : QStringLiteral("HfsFixture/alpha");
            if (ext) {
                payload = QByteArray(data["file_size"].toInt(), '\0');
                payload.replace(0, 4, "head");
                payload.replace(payload.size() - 4, 4, "tail");
            } else
                payload = QByteArray::fromBase64(
                    data["files"].toObject()[filename].toString().toLatin1());
        }
        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        const QString path = directory.filePath(QStringLiteral("том 空.img"));
        QFile source(path);
        QVERIFY(source.open(QIODevice::WriteOnly));
        QCOMPARE(source.write(image), qint64(image.size()));
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
            message->setProperty("uiInspection_state", kind);
            geometry &= UiInspection::writeReport(*message, "filesystem-limitations-" + language +
                                                                '-' + kind);
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
            // HFS starts at its volume root; descend through the public UI.
            if (kind != "exfat" && !ntfs && !ext) {
                QTRY_VERIFY(Row(*table, "HfsFixture") >= 0);
                const int row = Row(*table, "HfsFixture");
                table->selectRow(row);
                QVERIFY(QMetaObject::invokeMethod(table, "cellDoubleClicked", Qt::DirectConnection,
                                                  Q_ARG(int, row), Q_ARG(int, 0)));
                filename = "alpha";
            }
            QTRY_VERIFY(Row(*table, filename) >= 0);
            table->selectRow(Row(*table, filename));
            for (const char *name : {"deleteAction", "renameAction", "moveAction"})
                QVERIFY(!window.findChild<QAction *>(name)->isEnabled());
            const QString output = directory.filePath("copied");
            ExtractionOptions copyOptions;
            copyOptions.pathMode = ExtractionOptions::PathMode::CurrentPaths;
            if (kind != "exfat" && !ntfs && !ext)
                copyOptions.currentFolder = "HfsFixture";
            window.extractTo(output, true, {}, copyOptions);
            QVERIFY2(unexpectedErrors.isEmpty(), qPrintable(unexpectedErrors.join('\n')));
            QFile copied(output + '/' + filename);
            QVERIFY2(
                copied.open(QIODevice::ReadOnly),
                qPrintable(
                    QDir(output).entryList(QDir::AllEntries | QDir::NoDotAndDotDot).join('\n')));
            QCOMPARE(copied.readAll(), payload);
            if (kind == "reparse-list-bad-id" || kind == "ea-record-small" ||
                kind == "data-list-unreadable-list" || kind == "standard-short" ||
                kind == "dir-root-missing" || kind.startsWith("security-")) {
                QTRY_VERIFY(Row(*table, "gamma") >= 0);
                table->selectRow(Row(*table, "gamma"));
                window.extractTo(output, true, {}, copyOptions);
                QFile raw(output + "/gamma");
                QVERIFY(raw.open(QIODevice::ReadOnly));
                QCOMPARE(raw.readAll(), QByteArray(65536, 'B'));
            }
            window.findChild<QAction *>("refreshAction")->trigger();
            QTRY_VERIFY(Row(*table, filename) >= 0);
            QCOMPARE(questions, 1);
            QVERIFY(badge->isVisible());
            window.openPath(directory.path());
            QVERIFY(!badge->isVisible());
            // Change our fixture after closing the old Windows read lease.
            if (kind == "exfat") {
                image[106] = char(quint8(image[106]) | 6u);
                QFile changed(path);
                QVERIFY(changed.open(QIODevice::ReadWrite));
                QVERIFY(changed.seek(106));
                QCOMPARE(changed.write(image.constData() + 106, 1), qint64(1));
                changed.close();
            }
            window.openArchive(path);
            QCOMPARE(questions, 2);
        }
        automation.stop();
        QVERIFY2(unexpectedErrors.isEmpty(), qPrintable(unexpectedErrors.join('\n')));
        QVERIFY(source.open(QIODevice::ReadOnly));
        QCOMPARE(source.readAll(), image);
    }
};

QTEST_MAIN(FilesystemReadUiTests)
#include "filesystem_read_ui_tests.moc"
