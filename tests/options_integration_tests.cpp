#include "CPP/7zip/UI/Common/ZipRegistry.h"
#include "CPP/7zip/UI/FileManager/LangUtils.h"
#include "CPP/7zip/UI/FileManager/SystemPage.h"
#include "Common/MyCom.h"
#include "layout_inspection.h"
#include "qtfmoptionsdialog.h"
#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QLineEdit>
#include <QListWidget>
#include <QMessageBox>
#include <QPushButton>
#include <QRadioButton>
#include <QSettings>
#include <QTabBar>
#include <QTabWidget>
#include <QTemporaryDir>
#include <QTimer>
#include <QTreeWidget>
#include <QtTest>

namespace {
class COptionsManager final : public IFolderManager, public CMyUnknownImp {
    Z7_COM_UNKNOWN_IMP_1(IFolderManager)
  public:
    Z7_COM7F_IMF(OpenFolderFile(IInStream *, const wchar_t *, const wchar_t *, IFolderFolder **,
                                IProgress *))
    override { return E_NOTIMPL; }
    Z7_COM7F_IMF(GetExtensions(BSTR *extensions)) override {
        return StringToBstr(UString(L"zip"), extensions);
    }
    Z7_COM7F_IMF(GetIconPath(const wchar_t *, BSTR *path, Int32 *index)) override {
        *index = 0;
        return StringToBstr(UString(L"/fixture/sevenzip"), path);
    }
};
class COptionsAssociations final : public IQtAssociationServices {
  public:
    int writes = 0;
    int notifications = 0;
    LONG failure = 0;
    bool CanApply(unsigned) const override { return true; }
    QString UnavailableReason(unsigned) const override { return {}; }
    UString OwnIconPath() const override { return UString(L"/fixture/sevenzip"); }
    bool Read(unsigned, const UString &, NRegistryAssoc::CShellExtInfo &info) override {
        info = NRegistryAssoc::CShellExtInfo();
        return true;
    }
    LONG Add(unsigned, const UString &, const NRegistryAssoc::CShellExtInfo &, const UString &,
             const UString &, const UString &, int) override {
        ++writes;
        return failure;
    }
    LONG Delete(unsigned, const UString &, const NRegistryAssoc::CShellExtInfo &) override {
        ++writes;
        return failure;
    }
    void NotifyChanged() override { ++notifications; }
};
QString text(const UString &value) { return QString::fromWCharArray(value.Ptr()); }
} // namespace

class OptionsIntegrationTests final : public QObject {
    Q_OBJECT
  private slots:
    void keyboardPageSwitchKeepsPendingInputAndVisibleFocus() {
        QTemporaryDir fixture;
        QtFmOptionsDialog dialog(fixture.filePath("options.ini"));
        dialog.show();
        auto *pages = dialog.findChild<QTabWidget *>("optionsPages");
        pages->setCurrentIndex(3);
        auto *editor = dialog.findChild<QLineEdit *>("editorCommand");
        editor->setText("pending editor --wait");
        editor->setFocus();
        QTest::keyClick(editor, Qt::Key_Tab, Qt::ControlModifier);
        QCOMPARE(pages->currentIndex(), 4);
        QTest::keyClick(pages, Qt::Key_Tab, Qt::ControlModifier | Qt::ShiftModifier);
        QCOMPARE(pages->currentIndex(), 3);
        QCOMPARE(editor->text(), QString("pending editor --wait"));
        auto *tabs = pages->findChild<QTabBar *>();
        QCOMPARE(dialog.focusWidget(), tabs);
        // Native Qt styles may visit the footer before entering the page.
        bool enteredPage = false;
        for (int i = 0; i < 16 && !enteredPage; ++i) {
            auto *focused = dialog.focusWidget();
            QVERIFY(focused && focused->isVisible());
            QTest::keyClick(focused, Qt::Key_Tab);
            focused = dialog.focusWidget();
            QVERIFY(focused && focused->isVisible());
            enteredPage = pages->currentWidget()->isAncestorOf(focused);
        }
        QVERIFY(enteredPage);
        dialog.reject();
        UString saved;
        ReadRegEditor(true, saved, fixture.filePath("options.ini"));
        QVERIFY(saved.IsEmpty());
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
