#include "createarchivedialog.h"
#include <QSettings>
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
