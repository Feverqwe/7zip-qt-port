#include "CPP/7zip/UI/Common/ZipRegistry.h"
#include "CPP/7zip/UI/FileManager/LangUtils.h"
#include "CPP/7zip/UI/FileManager/SystemPage.h"
#include "Common/MyCom.h"
#include "fixtures/options_page_fixture.h"
#include "layout_inspection.h"
#include "qtfmoptionsdialog.h"
#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QFont>
#include <QLineEdit>
#include <QListWidget>
#include <QMessageBox>
#include <QPushButton>
#include <QRadioButton>
#include <QScopeGuard>
#include <QSettings>
#include <QTabBar>
#include <QTabWidget>
#include <QTemporaryDir>
#include <QTimer>
#include <QTreeWidget>
#include <QtTest>

namespace {
using OptionsPageFixture::COptionsAssociations;
using OptionsPageFixture::COptionsManager;
QString text(const UString &value) { return QString::fromWCharArray(value.Ptr()); }
} // namespace

class OptionsIntegrationTests final : public QObject {
    Q_OBJECT
  private slots:
    void sourceInitialFrame_data() {
        QTest::addColumn<QString>("language");
        QTest::newRow("en") << QString("-");
        QTest::newRow("ru") << QString("ru");
    }
    void sourceInitialFrame() {
        QFETCH(QString, language);
        const QFont originalFont = QApplication::font();
        const auto restoreFont = qScopeGuard([&] { QApplication::setFont(originalFont); });
        int smallHeight = 0;
        for (int points : {8, 12}) {
            QFont font = originalFont;
            font.setPointSize(points);
            QApplication::setFont(font);
            QTemporaryDir fixture;
            const QString settings = fixture.filePath("options.ini");
            SaveRegLang(UString(language.toStdWString().c_str()), settings);
            CMyComPtr<IFolderManager> manager = new COptionsManager;
            auto associations = std::make_shared<COptionsAssociations>();
            QtFmOptionsDialog dialog(settings, nullptr, manager, associations);
            dialog.show();
            QApplication::processEvents();
            auto *pages = dialog.findChild<QTabWidget *>("optionsPages");
            QCOMPARE(pages->currentIndex(), 0);
            auto *list = dialog.findChild<QTreeWidget *>("systemAssociations");
            QVERIFY(list && list->topLevelItemCount() == 1);
            const int rowHeight = list->sizeHintForRow(0);
            QVERIFY(rowHeight > 0);
            // The rendered original System page exposes more than twenty rows.
            // Its source page template remains full-height even with one format.
            const int rows = list->viewport()->height() / rowHeight;
            QVERIFY2(rows >= 20,
                     qPrintable(QString("Only %1 initial rows at %2pt").arg(rows).arg(points)));
            if (points == 8)
                smallHeight = dialog.height();
            else
                QVERIFY(dialog.height() > smallHeight);
            QVERIFY(UiInspection::writeReport(
                dialog, QString("options-initial-%1-%2").arg(language).arg(points)));
            dialog.resize(520, 380);
            QApplication::processEvents();
            QVERIFY(UiInspection::fullyVisible(
                *dialog.findChild<QDialogButtonBox *>("settingsButtons"), dialog));
            QVERIFY(UiInspection::fullyVisible(*list, dialog));
            dialog.reject();
            QCOMPARE(associations->writes, 0);
            QCOMPARE(associations->notifications, 0);
        }
    }
    void translatedTabsKeepSourceInitialWidth() {
        const QFont originalFont = QApplication::font();
        const auto restoreFont = qScopeGuard([&] { QApplication::setFont(originalFont); });
        QFont font = originalFont;
        font.setPointSize(12);
        QApplication::setFont(font);
        int initialWidth = 0;
        for (const char *language : {"-", "ru"}) {
            QTemporaryDir fixture;
            const QString settings = fixture.filePath("options.ini");
            SaveRegLang(UString(language), settings);
            CMyComPtr<IFolderManager> manager = new COptionsManager;
            auto associations = std::make_shared<COptionsAssociations>();
            QtFmOptionsDialog dialog(settings, nullptr, manager, associations);
            dialog.show();
            QApplication::processEvents();
            // Both rendered source languages use the same page dimensions.
            // Their tab titles fit inside that page; they must not add another
            // content-width difference to its native border/chrome.
            if (!initialWidth)
                initialWidth = dialog.width();
            else
                QCOMPARE(dialog.width(), initialWidth);
            dialog.reject();
            QCOMPARE(associations->writes, 0);
        }
    }
    void allPagesApplyCancelAndRestart_data() {
        QTest::addColumn<int>("finish");
        QTest::addColumn<LONG>("failure");
        QTest::newRow("cancel") << 0 << LONG(0);
        QTest::newRow("apply-then-cancel") << 1 << LONG(0);
        QTest::newRow("ok") << 2 << LONG(0);
        QTest::newRow("cancel-denied") << 0 << LONG(5);
        QTest::newRow("apply-denied") << 1 << LONG(5);
        QTest::newRow("ok-denied") << 2 << LONG(5);
        QTest::newRow("apply-external-change") << 1 << LONG(1237);
        QTest::newRow("ok-external-change") << 2 << LONG(1237);
        QTest::newRow("denied-then-invalid-memory") << 3 << LONG(5);
    }
    void allPagesApplyCancelAndRestart() {
        QFETCH(int, finish);
        QFETCH(LONG, failure);
        QTemporaryDir fixture;
        const QString path = fixture.filePath("options 空.ini");
        SaveRegLang(UString("-"), path);
        {
            QSettings saved(path, QSettings::IniFormat);
            saved.setValue("Unrelated/value", "preserved");
        }
        CMyComPtr<IFolderManager> manager = new COptionsManager;
        auto associations = std::make_shared<COptionsAssociations>();
        associations->failure = failure;
        QtFmOptionsDialog dialog(path, nullptr, manager, associations);
        dialog.resize(finish == 1 ? QSize(520, 380) : QSize(640, 390));
        dialog.show();
        auto *pages = dialog.findChild<QTabWidget *>("optionsPages");
        QCOMPARE(pages->currentIndex(), 0);
        auto *apply = dialog.findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Apply);
        QVERIFY(!apply->isEnabled());
        int applied = 0;
        dialog.Applied = [&](const CFmSettings &) { ++applied; };
        auto *types = dialog.findChild<QTreeWidget *>("systemAssociations");
        QCOMPARE(types->topLevelItemCount(), 1);
        types->topLevelItem(0)->setSelected(true);
        dialog.findChild<QPushButton *>("systemCurrent")->click();
        pages->setCurrentIndex(1);
        dialog.findChild<QCheckBox *>("cascadedMenu")->click();
        pages->setCurrentIndex(2);
        dialog.findChild<QRadioButton *>("workSpecified")->click();
        const QString work = fixture.filePath("рабочая папка");
        dialog.findChild<QLineEdit *>("workPath")->setText(work);
        pages->setCurrentIndex(3);
        const QString editor = QStringLiteral("\"/fixture/editor 空\" --wait");
        auto *command = dialog.findChild<QLineEdit *>("editorCommand");
        command->setText(editor);
        command->setFocus();
        QCOMPARE(dialog.focusWidget(), command);
        pages->setCurrentIndex(4);
        dialog.findChild<QCheckBox *>("gridOption")->setChecked(true);
        pages->setCurrentIndex(5);
        auto *language = dialog.findChild<QComboBox *>("languageChoice");
        QVERIFY(language->count() > 1);
        language->setCurrentIndex(1);
        auto *memory = dialog.findChild<QLineEdit *>("extractMemoryEdit");
        if (finish == 3) {
            dialog.findChild<QCheckBox *>("extractMemorySet")->setChecked(true);
            memory->setText("not-a-number");
        }
        QVERIFY(apply->isEnabled());
        int errors = 0;
        QMessageBox::Icon errorIcon = QMessageBox::NoIcon;
        QTimer errorDriver;
        connect(&errorDriver, &QTimer::timeout, &dialog, [&] {
            auto *box = qobject_cast<QMessageBox *>(QApplication::activeModalWidget());
            if (!box)
                return;
            ++errors;
            errorIcon = box->icon();
            const bool visible = UiInspection::fullyVisible(*box, *box);
            const bool report = UiInspection::writeReport(
                *box, QString("options-apply-error-%1-%2-%3").arg(failure).arg(finish).arg(errors));
            box->accept();
            QVERIFY(visible);
            QVERIFY(report);
        });
        errorDriver.start(10);
        if (finish >= 2) {
            dialog.accept();
            if (finish == 3) {
                QVERIFY(dialog.isVisible());
                QCOMPARE(dialog.result(), int(QDialog::Rejected));
                QCOMPARE(pages->currentIndex(), 4);
                QCOMPARE(memory->text(), QString("not-a-number"));
                QCOMPARE(NExtract::Read_LimitGB(path), UInt32(-1));
                UString committedEditor;
                ReadRegEditor(true, committedEditor, path);
                QCOMPARE(text(committedEditor), editor);
                memory->setText("2");
                dialog.accept();
                QCOMPARE(NExtract::Read_LimitGB(path), UInt32(2));
            }
            QCOMPARE(dialog.result(), int(QDialog::Accepted));
        } else {
            if (finish == 1) {
                apply->click();
                QVERIFY(!apply->isEnabled());
                command->setText("discarded editor");
                dialog.findChild<QLineEdit *>("workPath")->setText("discarded folder");
                dialog.findChild<QCheckBox *>("gridOption")->setChecked(false);
                language->setCurrentIndex(0);
                dialog.findChild<QCheckBox *>("cascadedMenu")->setChecked(true);
                QVERIFY(apply->isEnabled());
            }
            dialog.reject();
            QCOMPARE(dialog.result(), int(QDialog::Rejected));
        }
        QCOMPARE(applied, finish == 0 ? 0 : 1);
        QCOMPARE(errors, (finish != 0 && failure != 0 ? 1 : 0) + (finish == 3 ? 1 : 0));
        if (errors)
            QCOMPARE(errorIcon, QMessageBox::Critical);
        QCOMPARE(associations->writes, finish == 0 ? 0 : 1);
        QCOMPARE(associations->notifications, finish == 0 ? 0 : 1);
        QCOMPARE(types->topLevelItem(0)->text(1),
                 finish != 0 && failure != 0 ? QString() : QString("7-Zip"));
        // Only source memory validation selects a different page.
        QCOMPARE(pages->currentIndex(), finish == 3 ? 4 : 5);
        UString savedEditor;
        ReadRegEditor(true, savedEditor, path);
        QCOMPARE(text(savedEditor), finish == 0 ? QString() : editor);
        NWorkDir::CInfo folders;
        folders.Load(path);
        QCOMPARE(folders.Mode,
                 finish == 0 ? NWorkDir::NMode::kSystem : NWorkDir::NMode::kSpecified);
        QCOMPARE(QString::fromWCharArray(fs2us(folders.Path).Ptr()),
                 finish == 0 ? QString() : work);
        CFmSettings settings;
        settings.Load(path);
        QCOMPARE(settings.ShowGrid, finish != 0);
        CContextMenuInfo menu;
        menu.Load(path);
        QCOMPARE(menu.Cascaded.Val, finish == 0);
        UString savedLanguage;
        ReadRegLang(savedLanguage, path);
        QCOMPARE(text(savedLanguage), finish == 0 ? QString("-") : QString("ru"));
        QtFmOptionsDialog restarted(path, nullptr, manager, associations);
        QCOMPARE(restarted.findChild<QTabWidget *>("optionsPages")->currentIndex(), 0);
        QCOMPARE(restarted.findChild<QLineEdit *>("editorCommand")->text(),
                 finish == 0 ? QString() : editor);
        QVERIFY(!restarted.findChild<QDialogButtonBox *>()
                     ->button(QDialogButtonBox::Apply)
                     ->isEnabled());
        QSettings saved(path, QSettings::IniFormat);
        QCOMPARE(saved.value("Unrelated/value").toString(), QString("preserved"));
        SaveRegLang(UString("-"), path);
        ReloadLang(path);
    }
};
QTEST_MAIN(OptionsIntegrationTests)
#include "options_integration_tests.moc"
