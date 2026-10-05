#include "archivecodeccatalog.h"
#include "createarchivedialog.h"
#include "layout_inspection.h"
#include <QScrollArea>
#include <QSignalSpy>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QtTest>

class CodecCreationUiTests final : public QObject {
    Q_OBJECT
    QTemporaryDir fixture_;
    QString source_, engine_, module_;
    QByteArray previousSettings_;
    std::shared_ptr<const ArchiveCodecCatalog> catalog_;

  private slots:
    void initTestCase() {
#ifndef SEVENZIP_CODEC_MODULE
        QSKIP("Official module unavailable; native creation UI remains unverified");
#else
        QVERIFY(fixture_.isValid());
        previousSettings_ = qgetenv("SEVENZIP_SETTINGS_PATH");
        qputenv("SEVENZIP_SETTINGS_PATH", fixture_.filePath("settings.ini").toUtf8());
        module_ = QString::fromUtf8(SEVENZIP_CODEC_MODULE);
        engine_ = QStandardPaths::findExecutable("7zz");
        if (engine_.isEmpty())
            QSKIP("Official engine unavailable; native creation UI remains unverified");
        ArchiveBackend backend(nullptr, engine_);
        QSignalSpy ready(&backend, &ArchiveBackend::codecCatalogReady);
        QVERIFY(backend.discoverCodecCatalog(module_));
        QTRY_COMPARE(ready.size(), 1);
        QVERIFY2(ready[0][0].toBool(), qPrintable(ready[0][1].toString()));
        catalog_ = backend.codecCatalog();
        source_ = fixture_.filePath(QStringLiteral("источник 空 with spaces.txt"));
        QFile source(source_);
        QVERIFY(source.open(QIODevice::WriteOnly));
        QCOMPARE(source.write("payload"), qint64(7));
#endif
    }
    void init() { QFile::remove(fixture_.filePath("settings.ini")); }
    void cleanupTestCase() {
        if (previousSettings_.isNull())
            qunsetenv("SEVENZIP_SETTINGS_PATH");
        else
            qputenv("SEVENZIP_SETTINGS_PATH", previousSettings_);
    }
    void selectedFormatCreatesRealArchive_data() {
        QTest::addColumn<QString>("type");
        QTest::addColumn<QSize>("size");
        for (const QString &type : {"7z", "zip", "gzip", "bzip2", "xz", "tar", "wim"})
            for (const QSize size : {QSize(760, 720), QSize(640, 480)})
                QTest::newRow(qPrintable(type + QString::number(size.width()))) << type << size;
    }
    void selectedFormatCreatesRealArchive() {
        QFETCH(QString, type);
        QFETCH(QSize, size);
        CreateArchiveDialog dialog(fixture_.path(), {source_}, nullptr, catalog_);
        auto *format = dialog.findChild<QComboBox *>("archiveFormat");
        QCOMPARE(format->count(), 7);
        const int index = format->findText(type);
        QVERIFY(index >= 0);
        format->setCurrentIndex(index);
        if (type == "tar") {
            auto *method = dialog.findChild<QComboBox *>("compressionMethod");
            QVERIFY(method->isEnabled());
            method->setCurrentIndex(method->findText("POSIX"));
            QCOMPARE(method->currentText(), QStringLiteral("POSIX"));
        }
        dialog.resize(size);
        dialog.show();
        QApplication::processEvents();
        auto *password = dialog.findChild<QLineEdit *>("archivePassword");
        password->setText("retained password");
        const bool encrypt = type == "7z" || type == "zip";
        QCOMPARE(password->isEnabled(), encrypt);
        if (!encrypt)
            QVERIFY(dialog.password().isEmpty());
        password->clear();
        auto *scroll = dialog.findChild<QScrollArea *>();
        scroll->ensureWidgetVisible(format);
        QApplication::processEvents();
        QVERIFY(UiInspection::fullyVisible(*format, dialog));
        QVERIFY(UiInspection::writeReport(dialog,
                                          type + "-creation-top-" + QString::number(size.width())));
        auto *group = dialog.findChild<QGroupBox *>("archiveEncryptionGroup");
        QCOMPARE(group->isEnabled(), encrypt);
        scroll->ensureWidgetVisible(group);
        QApplication::processEvents();
        QVERIFY(UiInspection::fullyVisible(*group, dialog));
        auto *buttons = dialog.findChild<QDialogButtonBox *>();
        QVERIFY(UiInspection::fullyVisible(*buttons, dialog));
        QVERIFY(
            UiInspection::writeReport(dialog, type + "-creation-" + QString::number(size.width())));
        const auto options = dialog.compressionOptions();
        QCOMPARE(options.format, type);
        if (type == "tar")
            QCOMPARE(QString::fromWCharArray(options.Method.Ptr()), QStringLiteral("POSIX"));
        buttons->button(QDialogButtonBox::Ok)->click();
        QCOMPARE(dialog.result(), int(QDialog::Accepted));
        ArchiveBackend backend(nullptr, engine_);
        QCOMPARE(backend.loadCodecCatalog(module_, catalog_->version()), S_OK);
        QSignalSpy finished(&backend, &ArchiveBackend::finished);
        backend.create(dialog.target(), dialog.sources(), dialog.password(),
                       dialog.encryptHeaders(), options);
        QTRY_COMPARE_WITH_TIMEOUT(finished.size(), 1, 10000);
        QVERIFY2(finished[0][0].toBool(), qPrintable(finished[0][1].toString()));
        QVERIFY(QFileInfo::exists(dialog.target()));
        if (type == "tar") {
            // TarHeader.cpp defines POSIX versus GNU magic. This observes the
            // actual writer result, not only the dialog's requested method.
            QFile archive(dialog.target());
            QVERIFY(archive.open(QIODevice::ReadOnly));
            QCOMPARE(archive.read(512).mid(257, 8), QByteArray("ustar\0"
                                                               "00",
                                                               8));
        }
        QVERIFY(QFile::remove(dialog.target()));
    }
    void sourceSelectionDeterminesFormatParameters() {
        const QString folder = fixture_.filePath(QStringLiteral("папка 空"));
        QVERIFY(QDir().mkpath(folder));
        CreateArchiveDialog dialog(fixture_.path(), {source_}, nullptr, catalog_);
        auto *format = dialog.findChild<QComboBox *>("archiveFormat");
        format->setCurrentIndex(format->findText("gzip"));
        QVERIFY(!dialog.findChild<QLineEdit *>("archivePassword")->isEnabled());
        QVERIFY(!dialog.findChild<QListWidget *>("availableSources"));
        CreateArchiveDialog multiple(fixture_.path(), {source_, folder}, nullptr, catalog_);
        QCOMPARE(multiple.sources().size(), 2);
        auto *types = multiple.findChild<QComboBox *>("archiveFormat");
        QCOMPARE(types->findText("gzip"), -1);
        types->setCurrentText("7z");
        QVERIFY(multiple.findChild<QLineEdit *>("archivePassword")->isEnabled());
        QCOMPARE(multiple.compressionOptions().format, QStringLiteral("7z"));
        QVERIFY(multiple.findChild<QComboBox *>("compressionMethod")->findText("*  LZMA2") >= 0);
        QVERIFY(
            multiple.findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Ok)->isEnabled());
    }
};
QTEST_MAIN(CodecCreationUiTests)
#include "codec_creation_ui_tests.moc"
