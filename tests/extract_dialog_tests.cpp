#include "extractdialog.h"
#include "layout_inspection.h"
#include <QDialogButtonBox>
#include <QDir>
#include <QFile>
#include <QProcess>
#include <QPushButton>
#include <QSettings>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QtTest>

class ExtractDialogTests final : public QObject {
    Q_OBJECT
    QTemporaryDir dir_;
    QByteArray oldSettings_;
    QString settings_;
    QComboBox *path(CExtractDialog &dialog) {
        const QString full = dialog.destination();
        dialog.findChild<QCheckBox *>("extractNameEnable")->setChecked(false);
        dialog.findChild<QComboBox *>("extractDestination")->setEditText(full);
        return dialog.findChild<QComboBox *>("extractDestination");
    }
    QComboBox *pathMode(CExtractDialog &dialog) {
        return dialog.findChild<QComboBox *>("extractPathMode");
    }
    QComboBox *overwrite(CExtractDialog &dialog) {
        return dialog.findChild<QComboBox *>("extractOverwriteMode");
    }
  private slots:
    void initTestCase() {
        QVERIFY(dir_.isValid());
        oldSettings_ = qgetenv("SEVENZIP_SETTINGS_PATH");
        settings_ = dir_.filePath("extract.ini");
        qputenv("SEVENZIP_SETTINGS_PATH", settings_.toUtf8());
    }
    void init() { QFile::remove(settings_); }
    void cleanupTestCase() {
        if (oldSettings_.isNull())
            qunsetenv("SEVENZIP_SETTINGS_PATH");
        else
            qputenv("SEVENZIP_SETTINGS_PATH", oldSettings_);
    }
    void originalModesAndPersistence() {
        {
            CExtractDialog dialog(dir_.path(), {});
            QCOMPARE(overwrite(dialog)->itemText(1), QStringLiteral("Overwrite without prompt"));
            QCOMPARE(overwrite(dialog)->itemText(2), QStringLiteral("Skip existing files"));
            pathMode(dialog)->setCurrentIndex(1);
            overwrite(dialog)->setCurrentIndex(4);
            dialog.findChild<QCheckBox *>("extractShowPassword")->setChecked(true);
            dialog.findChild<QLineEdit *>("extractPassword")
                ->setText(QStringLiteral("секрет с пробелом"));
            dialog.findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Ok)->click();
            QCOMPARE(dialog.result(), int(QDialog::Accepted));
            QCOMPARE(dialog.PathMode, NExtract::NPathMode::kNoPaths);
            QCOMPARE(dialog.OverwriteMode, NExtract::NOverwriteMode::kRenameExisting);
            QCOMPARE(dialog.Password, QStringLiteral("секрет с пробелом"));
            QVERIFY(QDir::fromNativeSeparators(dialog.destination()).endsWith('/'));
        }
        CExtractDialog restarted(dir_.path(), {});
        QCOMPARE(pathMode(restarted)->currentIndex(), 1);
        QCOMPARE(overwrite(restarted)->currentIndex(), 4);
        QCOMPARE(restarted.findChild<QLineEdit *>("extractPassword")->echoMode(),
                 QLineEdit::Normal);
        QVERIFY(restarted.password().isEmpty());
        QFile saved(settings_);
        QVERIFY(saved.open(QIODevice::ReadOnly));
        QVERIFY(!saved.readAll().contains(QStringLiteral("секрет").toUtf8()));
        pathMode(restarted)->setCurrentIndex(0);
        overwrite(restarted)->setCurrentIndex(0);
        restarted.reject();
        CExtractDialog afterCancel(dir_.path(), {});
        QCOMPARE(pathMode(afterCancel)->currentIndex(), 1);
        QCOMPARE(overwrite(afterCancel)->currentIndex(), 4);
    }
    void sourceOptionsRespectForcedAndSavedModes() {
        NExtract::CInfo saved;
        saved.PathMode = NExtract::NPathMode::kNoPaths;
        saved.PathMode_Force = true;
        saved.OverwriteMode = NExtract::NOverwriteMode::kSkip;
        saved.OverwriteMode_Force = true;
        saved.Save(settings_);
        CExtractDialog dialog(dir_.path(), {});
        dialog.SetInitialOptions(NExtract::NPathMode::kFullPaths, NExtract::NOverwriteMode::kAsk,
                                 QStringLiteral("пароль с пробелом"));
        QCOMPARE(pathMode(dialog)->currentIndex(), 1);
        QCOMPARE(overwrite(dialog)->currentIndex(), 2);
        QCOMPARE(dialog.findChild<QLineEdit *>("extractPassword")->text(),
                 QStringLiteral("пароль с пробелом"));
        CExtractDialog forced(dir_.path(), {});
        forced.PathMode_Force = true;
        forced.OverwriteMode_Force = true;
        forced.SetInitialOptions(NExtract::NPathMode::kFullPaths, NExtract::NOverwriteMode::kAsk,
                                 QString());
        QCOMPARE(pathMode(forced)->currentIndex(), 0);
        QCOMPARE(overwrite(forced)->currentIndex(), 0);
        QVERIFY(forced.findChild<QLineEdit *>("extractPassword")->text().isEmpty());
    }
    void originalHistoryRules() {
        QStringList history;
        for (int i = 0; i < 20; i++)
            history.append(QString("/history/%1/").arg(i));
        {
            CExtractDialog dialog("/initial", history);
            QCOMPARE(path(dialog)->count(), 16);
            QCOMPARE(path(dialog)->currentText(), QString("/initial"));
            path(dialog)->setCurrentIndex(5);
            dialog.accept();
        }
        NExtract::CInfo saved;
        saved.Load(settings_);
        QCOMPARE(saved.Paths.Size(), 16u);
        QCOMPARE(saved.Paths[0], history[5]);
        QCOMPARE(saved.Paths[1], history[0]);
        {
            CExtractDialog dialog("/initial", {});
            path(dialog)->setCurrentIndex(2);
            path(dialog)->setEditText(QStringLiteral("  /новая папка/δ space  "));
            dialog.accept();
            QCOMPARE(QDir::fromNativeSeparators(dialog.destination()),
                     QStringLiteral("/новая папка/δ space/"));
        }
        saved.Load(settings_);
        QCOMPARE(saved.Paths.Size(), 16u);
        QCOMPARE(QDir::fromNativeSeparators(saved.Paths[0]),
                 QStringLiteral("/новая папка/δ space/"));
        QVERIFY(saved.Paths[15] != history[15]);
        {
            CExtractDialog dialog("/initial", {" /DATA/ ", "/data/", " /different/ "});
            path(dialog)->setCurrentIndex(0);
            dialog.accept();
        }
        saved.Load(settings_);
        QCOMPARE(saved.Paths.Size(), 2u);
        QCOMPARE(saved.Paths[0], QString("/DATA/"));
        QCOMPARE(saved.Paths[1], QString("/different/"));
    }
    void legacyAndInvalidSettings() {
        {
            QSettings settings(settings_, QSettings::IniFormat);
            settings.setValue("history/extract", QStringList{"/legacy/"});
            settings.setValue("Extraction/ExtractMode", 99u);
            settings.setValue("Extraction/OverwriteMode", "invalid");
            settings.setValue("Toolbar/labels", false);
        }
        CExtractDialog dialog("/initial", {});
        QCOMPARE(path(dialog)->itemText(0), QString("/legacy/"));
        QCOMPARE(pathMode(dialog)->currentIndex(), 0);
        QCOMPARE(overwrite(dialog)->currentIndex(), 0);
        QCOMPARE(dialog.options().overwriteMode, ExtractionOptions::OverwriteMode::Ask);
        dialog.accept();
        QSettings settings(settings_, QSettings::IniFormat);
        QCOMPARE(settings.value("Toolbar/labels").toBool(), false);
        settings.setValue("Extraction/ExtractMode", UInt32(NExtract::NPathMode::kAbsPaths));
        settings.sync();
        CExtractDialog absolute("/initial", {});
        QCOMPARE(pathMode(absolute)->count(), 3);
        QCOMPARE(absolute.options().pathMode, ExtractionOptions::PathMode::AbsolutePaths);
    }
    void renderedLayout() {
        CExtractDialog dialog(QStringLiteral("/данные с пробелом/"), {"/previous/"});
        dialog.show();
        for (QSize size : {QSize(640, 360), QSize(470, 330)}) {
            dialog.resize(size);
            QApplication::processEvents();
            QVERIFY(
                UiInspection::writeReport(dialog, QString("extract-port-%1").arg(size.width())));
        }
        dialog.reject();
    }
    void realExtractionUsesDialogModes() {
        const QString source = dir_.filePath(QStringLiteral("файл с пробелом.txt"));
        QFile file(source);
        QVERIFY(file.open(QIODevice::WriteOnly));
        QCOMPARE(file.write("archive content"), qint64(15));
        file.close();
        ArchiveBackend backend;
        QSignalSpy done(&backend, &ArchiveBackend::finished);
        const QString archive = dir_.filePath("fixture.zip");
        backend.create(archive, {source});
        QTRY_COMPARE_WITH_TIMEOUT(done.size(), 1, 15000);
        QVERIFY2(done.first()[0].toBool(), qPrintable(done.first()[1].toString()));
        done.clear();
        backend.list(archive);
        QTRY_COMPARE_WITH_TIMEOUT(done.size(), 1, 15000);
        QVERIFY2(done.first()[0].toBool(), qPrintable(done.first()[1].toString()));
        done.clear();
        const QString output = dir_.filePath(QStringLiteral("папка извлечения"));
        QVERIFY(QDir().mkpath(output));
        QFile existing(QDir(output).filePath(QFileInfo(source).fileName()));
        QVERIFY(existing.open(QIODevice::WriteOnly));
        QCOMPARE(existing.write("existing"), qint64(8));
        existing.close();
        CExtractDialog dialog(output, {});
        pathMode(dialog)->setCurrentIndex(1);
        overwrite(dialog)->setCurrentIndex(2);
        dialog.accept();
        auto options = dialog.options();
        options.ElimDup.Val = false; // This fixture exercises the basic CLI transport.
        backend.extract(archive, dialog.destination(), {}, {}, options);
        QTRY_COMPARE_WITH_TIMEOUT(done.size(), 1, 15000);
        QVERIFY2(done.first()[0].toBool(), qPrintable(done.first()[1].toString()));
        QVERIFY(existing.open(QIODevice::ReadOnly));
        QCOMPARE(existing.readAll(), QByteArray("existing"));
        existing.close();
        done.clear();
        overwrite(dialog)->setCurrentIndex(1);
        options = dialog.options();
        options.ElimDup.Val = false;
        backend.extract(archive, dialog.destination(), {}, {}, options);
        QTRY_COMPARE_WITH_TIMEOUT(done.size(), 1, 15000);
        QVERIFY2(done.first()[0].toBool(), qPrintable(done.first()[1].toString()));
        QVERIFY(existing.open(QIODevice::ReadOnly));
        QCOMPARE(existing.readAll(), QByteArray("archive content"));
    }
};
QTEST_MAIN(ExtractDialogTests)
#include "extract_dialog_tests.moc"
