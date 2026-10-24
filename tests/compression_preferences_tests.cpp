#include "createarchivedialog.h"
#include "source_creation_fixture.h"
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
            QCOMPARE(format(restarted)->currentText(), QString("zip"));
            format(restarted)->setCurrentText("7z");
            QCOMPARE(limit(restarted)->currentData().toString(), QString("256M"));
            format(restarted)->setCurrentText("zip");
            QCOMPARE(limit(restarted)->currentData().toString(), QString("50%"));
        }
        QSettings saved(settingsPath_, QSettings::IniFormat);
        QCOMPARE(saved.value("Toolbar/labels").toBool(), false);
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
            invalid.findChild<QLineEdit *>("archiveName")
                ->setText(QStringLiteral("invalid") + QChar::Null + "name");
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
            QCOMPARE(dialog.compressionOptions().Level, 5);
            QVERIFY(dialog.compressionOptions().Method.IsEmpty());
            set(dialog, "compressionLevel", 9);
            set(dialog, "compressionWordSize", 258);
            set(dialog, "compressionThreads", 1);
            choose(dialog, "384M");
            format(dialog)->setCurrentText("7z");
            const auto options = dialog.compressionOptions();
            QCOMPARE(options.Level, 7);
            QCOMPARE(QString::fromWCharArray(options.Method.Ptr()), QString("PPMd"));
            QCOMPARE(options.Dict64, quint64(1 << 20));
            QCOMPARE(options.Order, 16);
            QCOMPARE(options.NumThreads, UInt32(-1));
            QCOMPARE(options.SolidBlockSize, UInt64(-1));
            dialog.findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Ok)->click();
            QCOMPARE(dialog.result(), int(QDialog::Accepted));
        }
        CreateArchiveDialog restored(dir_.path(), {source_});
        QCOMPARE(QString::fromWCharArray(restored.compressionOptions().Method.Ptr()),
                 QString("PPMd"));
        QCOMPARE(restored.compressionOptions().Dict64, quint64(1 << 20));
        format(restored)->setCurrentText("zip");
        const auto zip = restored.compressionOptions();
        QCOMPARE(zip.Level, 9);
        QVERIFY(zip.Method.IsEmpty());
        QCOMPARE(zip.Order, 258);
        QCOMPARE(zip.NumThreads, 1);
        QCOMPARE(limit(restored)->currentData().toString(), QString("384M"));
        // Feed the restored public options through the real backend.
        ArchiveBackend backend;
        QSignalSpy done(&backend, &ArchiveBackend::finished);
        const QString target = dir_.filePath("restored.zip");
        QCOMPARE(SourceCreationFixture::Create(backend, target, {source_}, {}, false, zip), S_OK);
        QProcess process;
        process.start("7z", {"t", "--", target});
        QVERIFY(process.waitForFinished(15000));
        QCOMPARE(process.exitCode(), 0);
        process.start("7z", {"x", "-o" + dir_.filePath("restored-out"), "--", target});
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
        QCOMPARE(options.Level, 5);
        QVERIFY(options.Method.IsEmpty());
        QCOMPARE(options.Dict64, UInt64(-1));
        QCOMPARE(options.Order, UInt32(-1));
        QCOMPARE(options.NumThreads, UInt32(-1));
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
        QCOMPARE(restarted.compressionOptions().Level, 5);
    }
    void levelChangeResetsProfile_data() {
        QTest::addColumn<QString>("formatName");
        QTest::newRow("7z") << QString("7z");
        QTest::newRow("zip") << QString("zip");
    }
    void methodAndDictionaryDependencies() {
        NCompression::CInfo info;
        NCompression::CFormatOptions fo;
        fo.FormatID = "7z";
        fo.Level = 5;
        fo.Method = "LZMA2";
        fo.Dictionary = 1 << 20;
        fo.Order = 48;
        fo.BlockLogSize = 25;
        fo.NumThreads = 1;
        info.Formats.Add(fo);
        info.Save(settingsPath_);
        CreateArchiveDialog dialog(dir_.path(), {source_});
        auto *method = dialog.findChild<QComboBox *>("compressionMethod");
        auto *dict = dialog.findChild<QComboBox *>("compressionDictionary");
        auto *solid = dialog.findChild<QComboBox *>("compressionSolidBlock");
        QCOMPARE(dialog.compressionOptions().Dict64, quint64(1 << 20));
        method->setCurrentText("LZMA");
        QCOMPARE(dialog.compressionOptions().Dict64, UInt64(-1));
        QCOMPARE(dialog.compressionOptions().Order, UInt32(-1));
        QCOMPARE(dialog.compressionOptions().NumThreads, UInt32(-1));
        QVERIFY(!dialog.compressionOptions().SolidIsSpecified);
        method->setCurrentIndex(method->findData(-1));
        QCOMPARE(dialog.compressionOptions().Dict64, quint64(1 << 20));
        QCOMPARE(dialog.compressionOptions().Order, 48);
        QCOMPARE(dialog.compressionOptions().NumThreads, 1);
        QCOMPARE(dialog.compressionOptions().SolidBlockSize, quint64(32 << 20));
        dict->setCurrentIndex(dict->findData(QVariant::fromValue(quint64(2 << 20))));
        QVERIFY(!dialog.compressionOptions().SolidIsSpecified);
        QCOMPARE(dialog.compressionOptions().Order, 48);
        QCOMPARE(dialog.compressionOptions().NumThreads, 1);
        for (const UInt32 block : {UInt32(0), UInt32(64)}) {
            solid->setCurrentIndex(solid->findData(block));
            const quint64 size = block == 0 ? quint64(3 << 20) : quint64(4 << 20);
            dict->setCurrentIndex(dict->findData(QVariant::fromValue(size)));
            QCOMPARE(solid->currentData().toUInt(), block);
        }
        dialog.reject();
        info.Load(settingsPath_);
        QCOMPARE(info.Formats[0].Dictionary, UInt32(1 << 20));
        QCOMPARE(info.Formats[0].BlockLogSize, 25u);
    }
    void levelChangeResetsProfile() {
        QFETCH(QString, formatName);
        NCompression::CInfo info;
        NCompression::CFormatOptions fo;
        fo.FormatID = formatName;
        fo.Level = 5;
        fo.Method = "PPMd";
        fo.Dictionary = 1 << 20;
        fo.Order = 12;
        fo.BlockLogSize = 64;
        fo.NumThreads = 1;
        fo.MemUse = "256M";
        info.Formats.Add(fo);
        info.Save(settingsPath_);
        CreateArchiveDialog dialog(dir_.path(), {source_});
        format(dialog)->setCurrentText(formatName);
        QCOMPARE(QString::fromWCharArray(dialog.compressionOptions().Method.Ptr()),
                 QString("PPMd"));
        QCOMPARE(dialog.compressionOptions().Order, 12);
        dialog.findChild<QComboBox *>("compressionLevel")
            ->setCurrentIndex(dialog.findChild<QComboBox *>("compressionLevel")->findData(7u));
        auto options = dialog.compressionOptions();
        QCOMPARE(options.Level, 7);
        QVERIFY(options.Method.IsEmpty());
        QCOMPARE(options.Dict64, UInt64(-1));
        QCOMPARE(options.Order, UInt32(-1));
        QCOMPARE(options.NumThreads, UInt32(-1));
        QVERIFY(!options.SolidIsSpecified);
        QCOMPARE(limit(dialog)->currentData().toString(), QString("256M"));
        // Store also resets the specs and disables dependent controls.
        dialog.findChild<QComboBox *>("compressionLevel")->setCurrentIndex(0);
        QVERIFY(!dialog.findChild<QComboBox *>("compressionMethod")->isEnabled());
        QVERIFY(!dialog.findChild<QComboBox *>("compressionDictionary")->isEnabled());
        QVERIFY(!dialog.findChild<QComboBox *>("compressionWordSize")->isEnabled());
        dialog.findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Ok)->click();
        QCOMPARE(dialog.result(), int(QDialog::Accepted));
        CreateArchiveDialog restarted(dir_.path());
        format(restarted)->setCurrentText(formatName);
        QCOMPARE(restarted.compressionOptions().Level, 0);
        QCOMPARE(restarted.compressionOptions().Dict64, UInt64(-1));
        QCOMPARE(limit(restarted)->currentData().toString(), QString("256M"));
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
    void malformedPasswordPreferencesUseSourceDefaults() {
        {
            QSettings key(settingsPath_, QSettings::IniFormat);
            key.setValue("Compression/ShowPassword", "invalid");
            key.setValue("Compression/EncryptHeaders", "invalid");
        }
        {
            CreateArchiveDialog dialog(dir_.path(), {source_});
            QVERIFY(!dialog.findChild<QCheckBox *>("archiveShowPassword")->isChecked());
            QVERIFY(!dialog.findChild<QCheckBox *>("archiveEncryptHeaders")->isChecked());
            QCOMPARE(dialog.findChild<QLineEdit *>("archivePassword")->echoMode(),
                     QLineEdit::Password);
            dialog.findChild<QCheckBox *>("archiveShowPassword")->setChecked(true);
            dialog.findChild<QCheckBox *>("archiveEncryptHeaders")->setChecked(true);
            submit(dialog);
            QCOMPARE(dialog.result(), int(QDialog::Accepted));
        }
        CreateArchiveDialog restarted(dir_.path(), {source_});
        QVERIFY(restarted.findChild<QCheckBox *>("archiveShowPassword")->isChecked());
        QVERIFY(restarted.findChild<QCheckBox *>("archiveEncryptHeaders")->isChecked());
        QCOMPARE(restarted.findChild<QLineEdit *>("archivePassword")->echoMode(),
                 QLineEdit::Normal);
    }
};
QTEST_MAIN(CompressionPreferencesTests)
#include "compression_preferences_tests.moc"
