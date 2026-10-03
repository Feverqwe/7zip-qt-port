#include "CPP/7zip/UI/Common/WorkDir.h"
#include "Common/StringConvert.h"
#include "layout_inspection.h"
#include "qtfmoptionsdialog.h"
#include <QCheckBox>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QRadioButton>
#include <QScopeGuard>
#include <QSettings>
#include <QTabWidget>
#include <QTemporaryDir>
#include <QTimer>
#include <QtTest>

namespace {
QString qtPath(const FString &path) { return QString::fromWCharArray(fs2us(path)); }
} // namespace

class OptionsDialogTests : public QObject {
    Q_OBJECT
  private slots:
    void settingsGeometryAndKeyboardSpin() {
        QTemporaryDir fixture;
        QtFmOptionsDialog dialog(fixture.filePath("options.ini"));
        dialog.show();
        for (QSize size : {QSize(640, 390), QSize(520, 380)}) {
            dialog.resize(size);
            QApplication::processEvents();
            QCOMPARE(dialog.size(), size);
            QWidget *previous = nullptr;
            for (const char *name :
                 {"showDotsOption", "realFileIconsOption", "fullRowOption", "gridOption",
                  "singleClickOption", "alternativeSelectionOption", "showSystemMenuOption",
                  "largePagesOption", "extractMemoryLabel", "extractMemorySpin"}) {
                auto *control = dialog.findChild<QWidget *>(name);
                QVERIFY(control && UiInspection::fullyVisible(*control, dialog));
                if (previous)
                    QVERIFY(UiInspection::bounds(*previous, dialog).bottom() <
                            UiInspection::bounds(*control, dialog).top());
                previous = control;
            }
            auto *spin = dialog.findChild<QWidget *>("extractMemorySpin");
            auto *ram = dialog.findChild<QWidget *>("extractMemoryRam");
            auto *enabled = dialog.findChild<QCheckBox *>("extractMemorySet");
            QVERIFY(UiInspection::fullyVisible(*ram, dialog));
            QVERIFY(UiInspection::separate(*enabled, *spin, dialog));
            QVERIFY(UiInspection::separate(*spin, *ram, dialog));
            auto *buttons = dialog.findChild<QDialogButtonBox *>();
            QVERIFY(UiInspection::fullyVisible(*buttons, dialog));
            QVERIFY(UiInspection::separate(*spin, *buttons, dialog));
            QVERIFY(UiInspection::writeReport(dialog,
                                              QString("options-settings-%1").arg(size.width())));
        }
        dialog.findChild<QCheckBox *>("extractMemorySet")->setChecked(true);
        auto *edit = dialog.findChild<QLineEdit *>("extractMemoryEdit");
        edit->setText("1");
        edit->setFocus();
        QTest::keyClick(edit, Qt::Key_Down);
        QCOMPARE(edit->text(), QString("1"));
        edit->setText("16385");
        QTest::keyClick(edit, Qt::Key_Up);
        QCOMPARE(edit->text(),
                 QString("16385")); // Above arrow range stays valid typed source input.
    }
    void foldersBrowseCancelPreservesPath() {
        QTemporaryDir fixture;
        QtFmOptionsDialog dialog(fixture.filePath("options.ini"));
        dialog.findChild<QTabWidget *>("optionsPages")->setCurrentIndex(2);
        dialog.show();
        dialog.findChild<QRadioButton *>("workSpecified")->click();
        auto *path = dialog.findChild<QLineEdit *>("workPath");
        path->setText(fixture.path());
        bool browsed = false;
        QTimer::singleShot(0, &dialog, [&] {
            auto *browse = qobject_cast<QFileDialog *>(QApplication::activeModalWidget());
            QVERIFY(browse);
            browsed = true;
            browse->reject();
        });
        dialog.findChild<QPushButton *>("workPathBrowse")->click();
        QVERIFY(browsed);
        QCOMPARE(path->text(), fixture.path());
    }
    void sourceMemoryValidation_data() {
        QTest::addColumn<QString>("text");
        QTest::addColumn<bool>("valid");
        QTest::newRow("empty") << QString() << false;
        QTest::newRow("trailing") << QString("3GB") << false;
        QTest::newRow("negative") << QString("-1") << false;
        QTest::newRow("over-source-limit") << QString::number((1u << 30) + 1) << false;
        QTest::newRow("zero-source-accepted") << QString("0") << true;
        QTest::newRow("source-maximum") << QString::number(1u << 30) << true;
        QTest::newRow("above-arrow-range") << QString("16385") << true;
    }
    void sourceMemoryValidation() {
        QFETCH(QString, text);
        QFETCH(bool, valid);
        QTemporaryDir fixture;
        const QString settings = fixture.filePath("memory.ini");
        QtFmOptionsDialog dialog(settings);
        dialog.show();
        auto *enabled = dialog.findChild<QCheckBox *>("extractMemorySet");
        auto *edit = dialog.findChild<QLineEdit *>("extractMemoryEdit");
        QVERIFY(!enabled->isChecked() && !edit->isEnabled());
        enabled->setChecked(true);
        edit->setText(text);
        dialog.findChild<QCheckBox *>("showDotsOption")->setChecked(true);
        bool errorShown = false;
        if (!valid)
            QTimer::singleShot(0, &dialog, [&] {
                auto *error = qobject_cast<QMessageBox *>(QApplication::activeModalWidget());
                QVERIFY(error);
                errorShown = true;
                error->accept();
            });
        dialog.findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Ok)->click();
        QCOMPARE(dialog.isVisible(), !valid);
        QCOMPARE(errorShown, !valid);
        QCOMPARE(NExtract::Read_LimitGB(settings), valid ? text.toUInt() : UInt32(-1));
        CFmSettings general;
        general.Load(settings);
        QVERIFY(general.ShowDots); // Source saves general flags before memory validation.
        if (!valid) {
            auto *focused = dialog.focusWidget();
            QVERIFY2(focused == edit || focused == dialog.findChild<QWidget *>("extractMemorySpin"),
                     focused ? qPrintable(focused->objectName()) : "No pending focus widget");
            QVERIFY(dialog.findChild<QDialogButtonBox *>()
                        ->button(QDialogButtonBox::Apply)
                        ->isEnabled());
            QCOMPARE(edit->text(), text);
            edit->setText("2");
            dialog.findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Ok)->click();
            QCOMPARE(dialog.result(), int(QDialog::Accepted));
            QCOMPARE(NExtract::Read_LimitGB(settings), UInt32(2));
        }
    }
    void memoryApplyThenCancelAndUnset() {
        QTemporaryDir fixture;
        const QString settings = fixture.filePath("memory.ini");
        NExtract::Save_LimitGB(settings, 3);
        QtFmOptionsDialog dialog(settings);
        auto *edit = dialog.findChild<QLineEdit *>("extractMemoryEdit");
        QCOMPARE(edit->text(), QString("3"));
        QVERIFY(edit->isEnabled());
        edit->setText("7");
        dialog.apply();
        QCOMPARE(NExtract::Read_LimitGB(settings), UInt32(7));
        edit->setText("9");
        dialog.reject();
        QCOMPARE(NExtract::Read_LimitGB(settings), UInt32(7));
        QtFmOptionsDialog unset(settings);
        unset.findChild<QCheckBox *>("extractMemorySet")->setChecked(false);
        unset.accept();
        QCOMPARE(NExtract::Read_LimitGB(settings), UInt32(-1));
        QtFmOptionsDialog reloaded(settings);
        QVERIFY(!reloaded.findChild<QCheckBox *>("extractMemorySet")->isChecked());
        QCOMPARE(reloaded.findChild<QLineEdit *>("extractMemoryEdit")->text(), QString("4"));
        auto *largePages = reloaded.findChild<QCheckBox *>("largePagesOption");
        QCOMPARE(largePages->isEnabled(), IsLargePageSupported());
    }
    void foldersApplyCancelAndRealWorkDirectory_data() {
        QTest::addColumn<bool>("apply");
        QTest::newRow("cancel") << false;
        QTest::newRow("apply-then-cancel") << true;
    }
    void foldersApplyCancelAndRealWorkDirectory() {
        QFETCH(bool, apply);
        QTemporaryDir fixture;
        QVERIFY(fixture.isValid());
        const QString settings = fixture.filePath("options.ini");
        NWorkDir::CInfo old;
        old.SetDefault();
        old.ForRemovableOnly = false;
        old.Save(settings);
        QtFmOptionsDialog dialog(settings);
        auto *pages = dialog.findChild<QTabWidget *>("optionsPages");
        QCOMPARE(pages->count(), 6);
        pages->setCurrentIndex(2);
        QCOMPARE(pages->currentWidget()->property("sourcePageID").toUInt(), 2400u);
        auto *path = dialog.findChild<QLineEdit *>("workPath");
        auto *browse = dialog.findChild<QPushButton *>("workPathBrowse");
        QVERIFY(!path->isEnabled() && !browse->isEnabled());
        auto *buttons = dialog.findChild<QDialogButtonBox *>();
        auto *applyButton = buttons->button(QDialogButtonBox::Apply);
        QVERIFY(!applyButton->isEnabled());
        dialog.show();
        auto *specified = dialog.findChild<QRadioButton *>("workSpecified");
        specified->setFocus();
        QTest::keyClick(specified, Qt::Key_Space);
        QVERIFY(path->isEnabled() && browse->isEnabled());
        const QString work = fixture.filePath(QStringLiteral("рабочая папка"));
        path->setText(work);
        QVERIFY(applyButton->isEnabled());
        if (apply) {
            applyButton->click();
            QVERIFY(!applyButton->isEnabled());
            path->setText(fixture.filePath("discarded"));
        }
        dialog.reject();
        NWorkDir::CInfo loaded;
        loaded.Load(settings);
        QCOMPARE(loaded.Mode, apply ? NWorkDir::NMode::kSpecified : NWorkDir::NMode::kSystem);
        QCOMPARE(qtPath(loaded.Path), apply ? work : QString());
        if (apply) {
            const QByteArray previous = qgetenv("SEVENZIP_SETTINGS_PATH");
            qputenv("SEVENZIP_SETTINGS_PATH", settings.toUtf8());
            const auto restore = qScopeGuard([&] {
                if (previous.isNull())
                    qunsetenv("SEVENZIP_SETTINGS_PATH");
                else
                    qputenv("SEVENZIP_SETTINGS_PATH", previous);
            });
            QString temporary;
            {
                CWorkDirTempFile file;
                QCOMPARE(file.CreateTempFile(
                             us2fs(fixture.filePath("archive.zip").toStdWString().c_str())),
                         HRESULT(S_OK));
                temporary = qtPath(file.Get_TempFilePath());
                QVERIFY(QDir::fromNativeSeparators(temporary).startsWith(
                    QDir::fromNativeSeparators(work) + '/'));
                QVERIFY(QFileInfo::exists(temporary));
            }
            QVERIFY(!QFileInfo::exists(temporary));
        }
    }
    void foldersGeometryAndInvalidConfiguration() {
        QTemporaryDir fixture;
        const QString settings = fixture.filePath("options.ini");
        {
            QSettings corrupt(settings, QSettings::IniFormat);
            corrupt.setValue("Options/WorkDirType", "invalid");
            corrupt.setValue("Options/WorkDirPath", "ignored");
        }
        QtFmOptionsDialog dialog(settings);
        QVERIFY(dialog.findChild<QRadioButton *>("workSystem")->isChecked());
        dialog.findChild<QTabWidget *>("optionsPages")->setCurrentIndex(2);
        dialog.show();
        for (QSize size : {QSize(640, 390), QSize(520, 380)}) {
            dialog.resize(size);
            QApplication::processEvents();
            QCOMPARE(dialog.size(), size);
            QWidget *previous = nullptr;
            for (const char *name :
                 {"workSystem", "workCurrent", "workSpecified", "workPath", "workRemovableOnly"}) {
                auto *control = dialog.findChild<QWidget *>(name);
                QVERIFY(control && UiInspection::fullyVisible(*control, dialog));
                if (previous)
                    QVERIFY(UiInspection::bounds(*previous, dialog).bottom() <
                            UiInspection::bounds(*control, dialog).top());
                previous = control;
            }
            auto *path = dialog.findChild<QLineEdit *>("workPath");
            auto *browse = dialog.findChild<QPushButton *>("workPathBrowse");
            QVERIFY(UiInspection::fullyVisible(*browse, dialog));
            QVERIFY(UiInspection::separate(*path, *browse, dialog));
            QVERIFY(UiInspection::fullyVisible(*dialog.findChild<QDialogButtonBox *>(), dialog));
            QVERIFY(
                UiInspection::writeReport(dialog, QString("options-folders-%1").arg(size.width())));
        }
    }
};
QTEST_MAIN(OptionsDialogTests)
#include "options_dialog_tests.moc"
