#include "createarchivedialog.h"
#include <QScrollArea>
#include <QSettings>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTimer>
#include <QtTest>

class CompressionPreferencesTests final : public QObject {
    Q_OBJECT
    QTemporaryDir dir_;
    QByteArray oldPath_;
    QString settingsPath_;
    QString source_;
    QComboBox *limit(CreateArchiveDialog &dialog) {
        return dialog.findChild<QComboBox *>("compressionMemoryLimit");
    }
    QComboBox *format(CreateArchiveDialog &dialog) {
        return dialog.findChild<QComboBox *>("archiveFormat");
    }
    void choose(CreateArchiveDialog &dialog, const QString &spec) {
        auto *combo = limit(dialog);
        combo->setCurrentIndex(combo->findData(spec));
    }
    void submit(CreateArchiveDialog &dialog) {
        dialog.findChild<QComboBox *>("compressionMethod")->setCurrentText("PPMd");
        auto *dictionary = dialog.findChild<QComboBox *>("compressionDictionary");
        dictionary->setCurrentIndex(dictionary->findData(QVariant::fromValue(quint64(1 << 20))));
        dialog.findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Ok)->click();
    }
  private slots:
    void initTestCase() {
        QVERIFY(dir_.isValid());
        oldPath_ = qgetenv("SEVENZIP_SETTINGS_PATH");
        settingsPath_ = dir_.filePath("settings.ini");
        qputenv("SEVENZIP_SETTINGS_PATH", settingsPath_.toUtf8());
        source_ = dir_.filePath(QStringLiteral("данные с пробелом.txt"));
        QFile file(source_);
        QVERIFY(file.open(QIODevice::WriteOnly));
        QCOMPARE(file.write("source"), qint64(6));
    }
    void init() { QFile::remove(settingsPath_); }
    void cleanupTestCase() {
        if (oldPath_.isNull())
            qunsetenv("SEVENZIP_SETTINGS_PATH");
        else
            qputenv("SEVENZIP_SETTINGS_PATH", oldPath_);
    }
    void independentFormatsAndRestart() {
        QSettings unrelated(settingsPath_, QSettings::IniFormat);
        unrelated.setValue("Toolbar/labels", false);
        unrelated.sync();
        {
            CreateArchiveDialog dialog(dir_.path(), {source_});
            QCOMPARE(limit(dialog)->currentIndex(), 0);
            choose(dialog, "256M");
            format(dialog)->setCurrentText("zip");
            QCOMPARE(limit(dialog)->currentIndex(), 0);
            choose(dialog, "50%");
            format(dialog)->setCurrentText("7z");
            QCOMPARE(limit(dialog)->currentData().toString(), QString("256M"));
            format(dialog)->setCurrentText("zip");
            QCOMPARE(limit(dialog)->currentData().toString(), QString("50%"));
            submit(dialog);
            QCOMPARE(dialog.result(), int(QDialog::Accepted));
        }
        {
            CreateArchiveDialog restarted(dir_.path(), {source_});
            QCOMPARE(limit(restarted)->currentData().toString(), QString("256M"));
            format(restarted)->setCurrentText("zip");
            QCOMPARE(limit(restarted)->currentData().toString(), QString("50%"));
        }
        QSettings saved(settingsPath_, QSettings::IniFormat);
        QCOMPARE(saved.value("Toolbar/labels").toBool(), false);
        const QString key = sizeof(size_t) == 8 ? "MemUse64" : "MemUse32";
        QCOMPARE(saved.value("Compression/Options/7z/" + key).toString(), QString("256M"));
        QCOMPARE(saved.value("Compression/Options/zip/" + key).toString(), QString("50%"));
    }
    void cancelAndValidationFailureDoNotSave() {
        {
            CreateArchiveDialog dialog(dir_.path(), {source_});
            choose(dialog, "512M");
            submit(dialog);
            QCOMPARE(dialog.result(), int(QDialog::Accepted));
        }
        {
            CreateArchiveDialog cancelled(dir_.path(), {source_});
            choose(cancelled, "100%");
            format(cancelled)->setCurrentText("zip");
            choose(cancelled, "256M");
            cancelled.reject();
        }
        {
            CreateArchiveDialog invalid(dir_.path(), {source_});
            choose(invalid, "100%");
            invalid.findChild<QLineEdit *>("archiveName")->setText("invalid/name");
            QTimer::singleShot(0, &invalid, [] {
                auto *message = qobject_cast<QMessageBox *>(QApplication::activeModalWidget());
                if (message)
                    message->accept();
            });
            submit(invalid);
            QCOMPARE(invalid.result(), 0);
            invalid.reject();
        }
        CreateArchiveDialog restarted(dir_.path());
        QCOMPARE(limit(restarted)->currentData().toString(), QString("512M"));
        format(restarted)->setCurrentText("zip");
        QCOMPARE(limit(restarted)->currentIndex(), 0);
    }
    void customAndMalformedSavedLimits() {
        NCompression::CInfo info;
        NCompression::CFormatOptions sevenz;
        sevenz.FormatID = "7z";
        sevenz.MemUse = "37%";
        info.Formats.Add(sevenz);
        NCompression::CFormatOptions zip;
        zip.FormatID = "zip";
        zip.MemUse = "320M";
        info.Formats.Add(zip);
        info.Save(settingsPath_);
        {
            CreateArchiveDialog dialog(dir_.path());
            QCOMPARE(limit(dialog)->currentData().toString(), QString("37%"));
            format(dialog)->setCurrentText("zip");
            QCOMPARE(limit(dialog)->currentData().toString(), QString("320M"));
            dialog.reject();
        }
        info.Formats[0].MemUse = "18446744073709551616M";
        info.Save(settingsPath_);
        CreateArchiveDialog invalid(dir_.path());
        QCOMPARE(limit(invalid)->currentIndex(), 0);
        format(invalid)->setCurrentText("zip");
        QCOMPARE(limit(invalid)->currentData().toString(), QString("320M"));
        NCompression::CInfo reloaded;
        reloaded.Load(settingsPath_);
        QCOMPARE(reloaded.Formats.Size(), 2u);
        reloaded.Load(dir_.filePath("missing.ini"));
        QVERIFY(reloaded.Formats.IsEmpty());
    }
    void completeFormatPreferences() {
        auto combo = [](CreateArchiveDialog &dialog, const char *name) {
            return dialog.findChild<QComboBox *>(name);
        };
        auto set = [&](CreateArchiveDialog &dialog, const char *name, UInt32 value) {
            auto *c = combo(dialog, name);
            const int index = c->findData(value);
            QVERIFY2(index >= 0, name);
            c->setCurrentIndex(index);
        };
        {
            CreateArchiveDialog dialog(dir_.path(), {source_});
            set(dialog, "compressionLevel", 7);
            combo(dialog, "compressionMethod")->setCurrentText("PPMd");
            set(dialog, "compressionDictionary", 1 << 20);
            set(dialog, "compressionWordSize", 16);
            set(dialog, "compressionSolidBlock", 64);
            choose(dialog, "256M");
            format(dialog)->setCurrentText("zip");
            QCOMPARE(dialog.compressionOptions().level, 5);
            QCOMPARE(dialog.compressionOptions().method, QString("Deflate"));
            set(dialog, "compressionLevel", 9);
            set(dialog, "compressionWordSize", 258);
            set(dialog, "compressionThreads", 1);
            choose(dialog, "384M");
            format(dialog)->setCurrentText("7z");
            const auto options = dialog.compressionOptions();
            QCOMPARE(options.level, 7);
            QCOMPARE(options.method, QString("PPMd"));
            QCOMPARE(options.dictionaryBytes, quint64(1 << 20));
            QCOMPARE(options.wordSize, 16);
            QCOMPARE(options.threads, 0);
            QCOMPARE(options.SolidBlockSize, UInt64(-1));
            dialog.findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Ok)->click();
            QCOMPARE(dialog.result(), int(QDialog::Accepted));
        }
        CreateArchiveDialog restored(dir_.path(), {source_});
        QCOMPARE(restored.compressionOptions().method, QString("PPMd"));
        QCOMPARE(restored.compressionOptions().dictionaryBytes, quint64(1 << 20));
        format(restored)->setCurrentText("zip");
        const auto zip = restored.compressionOptions();
        QCOMPARE(zip.level, 9);
        QCOMPARE(zip.method, QString("Deflate"));
        QCOMPARE(zip.wordSize, 258);
        QCOMPARE(zip.threads, 1);
        QCOMPARE(limit(restored)->currentData().toString(), QString("384M"));
        restored.show();
        const QString screenshots = QDir::currentPath() + "/ui-screenshots";
        QVERIFY(QDir().mkpath(screenshots));
        for (const QSize size : {QSize(760, 720), QSize(640, 480)}) {
            restored.resize(size);
            QApplication::processEvents();
            restored.findChild<QScrollArea *>()->ensureWidgetVisible(limit(restored), 0, 80);
            QApplication::processEvents();
            QVERIFY(restored.grab().save(
                screenshots + QString("/compression-preferences-%1.png").arg(size.width())));
        }
        // Feed the restored public options through the real backend.
        ArchiveBackend backend;
        QSignalSpy done(&backend, &ArchiveBackend::finished);
        const QString target = dir_.filePath("restored.zip");
        backend.create(target, {source_}, {}, false, zip);
        QTRY_COMPARE_WITH_TIMEOUT(done.size(), 1, 15000);
        QVERIFY2(done.first()[0].toBool(), qPrintable(done.first()[1].toString()));
        QProcess process;
        process.start("7zz", {"t", "--", target});
        QVERIFY(process.waitForFinished(15000));
        QCOMPARE(process.exitCode(), 0);
        process.start("7zz", {"x", "-o" + dir_.filePath("restored-out"), "--", target});
        QVERIFY(process.waitForFinished(15000));
        QCOMPARE(process.exitCode(), 0);
        QFile output(dir_.filePath("restored-out/" + QFileInfo(source_).fileName()));
        QVERIFY(output.open(QIODevice::ReadOnly));
        QCOMPARE(output.readAll(), QByteArray("source"));
        restored.reject();
        QFile::remove(target);
    }
    void invalidStoredControlsAndAutoSentinels() {
        QSettings saved(settingsPath_, QSettings::IniFormat);
        saved.setValue("Compression/Options/7z/Level", "broken");
        saved.setValue("Compression/Options/7z/Method", "removed codec");
        saved.setValue("Compression/Options/7z/Dictionary", "18446744073709551615");
        saved.setValue("Compression/Options/7z/Order", 999);
        saved.setValue("Compression/Options/7z/BlockSize", 63);
        saved.setValue("Compression/Options/7z/NumThreads", 999999);
        saved.sync();
        CreateArchiveDialog dialog(dir_.path(), {source_});
        auto options = dialog.compressionOptions();
        QCOMPARE(options.level, 5);
        QCOMPARE(options.method, QString("LZMA2"));
        QCOMPARE(options.dictionaryBytes, quint64(0));
        QCOMPARE(options.wordSize, 0);
        QCOMPARE(options.threads, 0);
        dialog.findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Ok)->click();
        QCOMPARE(dialog.result(), int(QDialog::Accepted));
        NCompression::CInfo loaded;
        loaded.Load(settingsPath_);
        QCOMPARE(loaded.Formats[0].Dictionary, UInt32(-1));
        QCOMPARE(loaded.Formats[0].Order, UInt32(-1));
        QCOMPARE(loaded.Formats[0].NumThreads, UInt32(-1));
        QCOMPARE(loaded.Formats[0].BlockLogSize, UInt32(-1));
        CreateArchiveDialog cancelled(dir_.path(), {source_});
        cancelled.findChild<QComboBox *>("compressionLevel")->setCurrentIndex(0);
        cancelled.reject();
        CreateArchiveDialog restarted(dir_.path());
        QCOMPARE(restarted.compressionOptions().level, 5);
    }
    void autoClearsSavedLimit() {
        {
            CreateArchiveDialog dialog(dir_.path(), {source_});
            choose(dialog, "256M");
            submit(dialog);
            QCOMPARE(dialog.result(), int(QDialog::Accepted));
        }
        {
            CreateArchiveDialog dialog(dir_.path(), {source_});
            choose(dialog, QString());
            submit(dialog);
            QCOMPARE(dialog.result(), int(QDialog::Accepted));
        }
        CreateArchiveDialog restarted(dir_.path());
        QCOMPARE(limit(restarted)->currentIndex(), 0);
        QVERIFY(!restarted.compressionOptions().MemUsage.IsDefined);
    }
};
QTEST_MAIN(CompressionPreferencesTests)
#include "compression_preferences_tests.moc"
