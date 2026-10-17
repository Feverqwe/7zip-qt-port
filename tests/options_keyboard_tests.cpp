#include "CPP/7zip/UI/Common/ZipRegistry.h"
#include "CPP/7zip/UI/FileManager/LangUtils.h"
#include "fixtures/options_page_fixture.h"
#include "layout_inspection.h"
#include "qtfmoptionsdialog.h"
#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QLineEdit>
#include <QPushButton>
#include <QRadioButton>
#include <QSet>
#include <QStyleHints>
#include <QTabBar>
#include <QTabWidget>
#include <QTemporaryDir>
#include <QTreeWidget>
#include <QtTest>

using OptionsPageFixture::COptionsAssociations;
using OptionsPageFixture::COptionsManager;

class OptionsKeyboardTests final : public QObject {
    Q_OBJECT
  private slots:
    void sourcePageActivation_data() {
        QTest::addColumn<QString>("language");
        QTest::addColumn<QSize>("size");
        for (const QString &language : {QString("-"), QString("ru")})
            for (const QSize size : {QSize(640, 390), QSize(520, 380)})
                QTest::newRow(qPrintable(QString("%1-%2").arg(language).arg(size.width())))
                    << language << size;
    }
    void sourcePageActivation() {
        QFETCH(QString, language);
        QFETCH(QSize, size);
        QTemporaryDir fixture;
        const QString settings = fixture.filePath("activation.ini");
        SaveRegLang(UString(language.toStdWString().c_str()), settings);
        CMyComPtr<IFolderManager> manager = new COptionsManager;
        auto associations = std::make_shared<COptionsAssociations>();
        QtFmOptionsDialog dialog(settings, nullptr, manager, associations);
        dialog.resize(size.expandedTo(dialog.minimumSizeHint()));
        dialog.show();
        QVERIFY(QTest::qWaitForWindowActive(&dialog));
        auto *pages = dialog.findChild<QTabWidget *>("optionsPages");
        auto *current = dialog.findChild<QRadioButton *>("workCurrent");
        pages->setCurrentIndex(2);
        current->click();
        auto *viewer = dialog.findChild<QLineEdit *>("viewerCommand");
        auto *editor = dialog.findChild<QLineEdit *>("editorCommand");
        viewer->setText("pending viewer --wait");
        editor->setText("pending editor --wait");
        pages->setCurrentIndex(0);
        dialog.findChild<QPushButton *>("systemCurrent")->setFocus();
        auto expected = [&](int index) -> QWidget * {
            const char *first[] = {"systemCurrent", nullptr,          "workCurrent",
                                   "viewerCommand", "showDotsOption", "languageChoice"};
            if (first[index])
                return dialog.findChild<QWidget *>(first[index]);
            // MenuPage2.rc order; native capabilities can disable this page.
            for (const char *name : {"shellIntegration", "shellIntegration32", "cascadedMenu",
                                     "menuIcons", "menuElimDup", "menuZone", "menuItems"}) {
                auto *control = dialog.findChild<QWidget *>(name);
                if (control && control->isVisible() && control->isEnabled())
                    return control;
            }
            return pages->tabBar();
        };
        for (int direction : {1, -1})
            for (int step = 0; step < 6; ++step) {
                const int target = (pages->currentIndex() + direction + 6) % 6;
                Qt::KeyboardModifiers modifiers = Qt::ControlModifier;
                if (direction < 0)
                    modifiers |= Qt::ShiftModifier;
                QTest::keyClick(dialog.focusWidget(), Qt::Key_Tab, modifiers);
                QCOMPARE(pages->currentIndex(), target);
                QCOMPARE(dialog.focusWidget(), expected(target));
                QVERIFY(dialog.focusWidget()->isVisible());
                QVERIFY(current->isChecked());
                QCOMPARE(viewer->text(), QString("pending viewer --wait"));
                QCOMPARE(editor->text(), QString("pending editor --wait"));
                if (target == 3)
                    QCOMPARE(viewer->selectedText(), viewer->text());
                QVERIFY(UiInspection::writeReport(dialog, QString("options-activation-%1-%2-%3-%4")
                                                              .arg(language)
                                                              .arg(size.width())
                                                              .arg(direction)
                                                              .arg(target)));
            }
        dialog.reject();
        QCOMPARE(associations->writes, 0);
        QCOMPARE(associations->notifications, 0);
    }
    void sourceInitialFocusAndSystemTraversal_data() {
        QTest::addColumn<QString>("language");
        QTest::addColumn<QSize>("size");
        QTest::addColumn<int>("availability");
        for (const QString &language : {QString("-"), QString("ru")})
            for (const QSize size : {QSize(640, 390), QSize(520, 380)})
                for (int availability = 0; availability < 4; ++availability)
                    QTest::newRow(qPrintable(
                        QString("%1-%2-%3").arg(language).arg(size.width()).arg(availability)))
                        << language << size << availability;
    }
    void sourceInitialFocusAndSystemTraversal() {
        QFETCH(QString, language);
        QFETCH(QSize, size);
        QFETCH(int, availability);
        QTemporaryDir fixture;
        const QString settings = fixture.filePath("focus.ini");
        SaveRegLang(UString(language.toStdWString().c_str()), settings);
        CMyComPtr<IFolderManager> manager = new COptionsManager;
        auto associations = std::make_shared<COptionsAssociations>();
        associations->available[0] = availability & 1;
        associations->available[1] = availability & 2;
        QtFmOptionsDialog dialog(settings, nullptr, manager, associations);
        dialog.resize(size.expandedTo(dialog.minimumSizeHint()));
        dialog.show();
        QVERIFY(QTest::qWaitForWindowActive(&dialog));
        auto *current = dialog.findChild<QPushButton *>("systemCurrent");
        auto *all = dialog.findChild<QPushButton *>("systemAll");
        auto *list = dialog.findChild<QTreeWidget *>("systemAssociations");
        QVERIFY(current && all && list);
        QCOMPARE(current->isEnabled(), bool(availability & 1));
        QCOMPARE(all->isEnabled(), bool(availability & 2));
        QCOMPARE(current->focusPolicy(), Qt::StrongFocus);
        QCOMPARE(all->focusPolicy(), Qt::StrongFocus);
        QList<QWidget *> stops;
        if (current->isEnabled())
            stops << current;
        if (all->isEnabled())
            stops << all;
        stops << list;
        // SystemPage.rc's first tab stop; the pinned rendered page confirms it.
        QCOMPARE(dialog.focusWidget(), stops.front());
        qInfo("System tab policy: %d", int(QGuiApplication::styleHints()->tabFocusBehavior()));
        for (int i = 1; i < stops.size(); ++i) {
            QTest::keyClick(stops[i - 1], Qt::Key_Tab);
            QCOMPARE(dialog.focusWidget(), stops[i]);
        }
        for (int i = stops.size() - 1; i > 0; --i) {
            QTest::keyClick(stops[i], Qt::Key_Backtab);
            QCOMPARE(dialog.focusWidget(), stops[i - 1]);
        }
        QSet<QWidget *> visited;
        for (int i = 0; i < 16; ++i) {
            auto *focused = dialog.focusWidget();
            QVERIFY(focused && focused->isVisible() && focused->isEnabled());
            if (visited.contains(focused))
                break;
            visited.insert(focused);
            QTest::keyClick(focused, Qt::Key_Tab);
        }
        for (auto *stop : stops)
            QVERIFY(visited.contains(stop));
        auto *pages = dialog.findChild<QTabWidget *>("optionsPages");
        QVERIFY(visited.contains(pages->tabBar()));
        auto *footer = dialog.findChild<QDialogButtonBox *>("settingsButtons");
        for (auto *button : footer->buttons())
            if (button->isEnabled())
                QVERIFY(visited.contains(button));
        for (QWidget *control : QList<QWidget *>{current, all, list})
            QVERIFY(UiInspection::fullyVisible(*control, dialog));
        QVERIFY(UiInspection::writeReport(dialog, QString("options-system-focus-%1-%2-%3")
                                                      .arg(language)
                                                      .arg(size.width())
                                                      .arg(availability)));
        dialog.findChild<QTabWidget *>("optionsPages")->setCurrentIndex(3);
        auto *editor = dialog.findChild<QLineEdit *>("editorCommand");
        editor->setText("pending editor --wait");
        editor->setFocus();
        dialog.hide();
        dialog.show();
        QVERIFY(QTest::qWaitForWindowActive(&dialog));
        QCOMPARE(dialog.focusWidget(), editor);
        QCOMPARE(editor->text(), QString("pending editor --wait"));
        dialog.reject();
        QCOMPARE(associations->writes, 0);
        QCOMPARE(associations->notifications, 0);
    }
    void keyboardPageSwitchKeepsPendingInputAndVisibleFocus() {
        QTemporaryDir fixture;
        QtFmOptionsDialog dialog(fixture.filePath("options.ini"));
        dialog.show();
        auto *pages = dialog.findChild<QTabWidget *>("optionsPages");
        pages->setCurrentIndex(3);
        auto *editor = dialog.findChild<QLineEdit *>("editorCommand");
        auto *viewer = dialog.findChild<QLineEdit *>("viewerCommand");
        viewer->setText("pending viewer --wait");
        editor->setText("pending editor --wait");
        editor->setFocus();
        QTest::keyClick(editor, Qt::Key_Tab, Qt::ControlModifier);
        QCOMPARE(pages->currentIndex(), 4);
        QCOMPARE(dialog.focusWidget(), dialog.findChild<QCheckBox *>("showDotsOption"));
        QTest::keyClick(pages, Qt::Key_Tab, Qt::ControlModifier | Qt::ShiftModifier);
        QCOMPARE(pages->currentIndex(), 3);
        QCOMPARE(editor->text(), QString("pending editor --wait"));
        QCOMPARE(dialog.focusWidget(), viewer);
        QCOMPARE(viewer->selectedText(), QString("pending viewer --wait"));
        auto *tabs = pages->findChild<QTabBar *>();
        QCOMPARE(tabs->focusPolicy(), Qt::StrongFocus);
        QVERIFY(UiInspection::fullyVisible(*viewer, dialog));
        dialog.reject();
        UString saved;
        ReadRegEditor(true, saved, fixture.filePath("options.ini"));
        QVERIFY(saved.IsEmpty());
    }
};
QTEST_MAIN(OptionsKeyboardTests)
#include "options_keyboard_tests.moc"
