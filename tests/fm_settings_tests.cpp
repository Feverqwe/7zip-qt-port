#include "CPP/7zip/UI/FileManager/SettingsPage.h"
#include "layout_inspection.h"
#include <QCheckBox>
#include <QDialogButtonBox>
#include <QSettings>
#include <QTemporaryDir>
#include <QtTest>

class FmSettingsTests : public QObject {
    Q_OBJECT
  private slots:
    void defaultsAndInvalidValues() {
        QTemporaryDir dir;
        const QString path = dir.filePath("настройки списка.ini");
        QSettings ini(path, QSettings::IniFormat);
        ini.setValue("FM/ShowGrid", "corrupt");
        ini.setValue("list/singleClick", true);
        ini.setValue("FM/SingleClick", 1);
        ini.setValue("FM/FullRow", 2);
        ini.sync();
        CFmSettings st;
        st.Load(path);
        QVERIFY(!st.ShowDots && !st.ShowRealFileIcons && !st.ShowGrid);
        QVERIFY(st.FullRow);
        QVERIFY(st.SingleClick);
        QVERIFY(!st.AlternativeSelection && !st.ShowSystemMenu);
        st.ShowDots = st.FullRow = st.AlternativeSelection = st.ShowSystemMenu = true;
        st.Save(path);
        CFmSettings loaded;
        loaded.Load(path);
        QVERIFY(loaded.ShowDots && loaded.FullRow && loaded.AlternativeSelection &&
                loaded.ShowSystemMenu && loaded.SingleClick);
        QVERIFY(!loaded.ShowGrid);
    }
    void confirmCancelAndPersistListFlags() {
        QTemporaryDir dir;
        const QString path = dir.filePath("fm.ini");
        CFmSettings st;
        st.Load(path);
        st.Save(path);
        for (bool confirm : {false, true}) {
            CSettingsPage page(path);
            for (const char *name : {"showDotsOption", "realFileIconsOption", "fullRowOption",
                                     "gridOption", "singleClickOption", "showSystemMenuOption"})
                page.findChild<QCheckBox *>(name)->setChecked(true);
            QVERIFY(!page.findChild<QCheckBox *>("alternativeSelectionOption")->isEnabled());
            if (confirm)
                page.accept();
            else
                page.reject();
            st.Load(path);
            QCOMPARE(st.ShowDots, confirm);
            QCOMPARE(st.ShowRealFileIcons, confirm);
            QCOMPARE(st.FullRow, confirm);
            QCOMPARE(st.ShowGrid, confirm);
            QCOMPARE(st.SingleClick, confirm);
            QVERIFY(!st.AlternativeSelection);
            QCOMPARE(st.ShowSystemMenu, confirm);
        }
        // Opening and confirming an unchanged page must not materialize defaults.
        const QString empty = dir.filePath("empty.ini");
        CSettingsPage page(empty);
        page.accept();
        QVERIFY(!QFileInfo::exists(empty));
    }
    void geometry() {
        QTemporaryDir dir;
        for (int width : {460, 340}) {
            CSettingsPage page(dir.filePath("fm.ini"));
            page.resize(width, 300);
            page.show();
            QApplication::processEvents();
            auto *buttons = page.findChild<QDialogButtonBox *>("settingsButtons");
            QWidget *previous = nullptr;
            for (const char *name :
                 {"showDotsOption", "realFileIconsOption", "fullRowOption", "gridOption",
                  "singleClickOption", "alternativeSelectionOption", "showSystemMenuOption"}) {
                auto *control = page.findChild<QCheckBox *>(name);
                QVERIFY(UiInspection::fullyVisible(*control, page));
                QVERIFY(control->width() >= control->sizeHint().width());
                QVERIFY(control->height() >= control->sizeHint().height());
                if (previous)
                    QVERIFY(UiInspection::bounds(*previous, page).bottom() <
                            UiInspection::bounds(*control, page).top());
                previous = control;
            }
            QVERIFY(UiInspection::fullyVisible(*buttons, page));
            QVERIFY(UiInspection::bounds(*previous, page).bottom() <
                    UiInspection::bounds(*buttons, page).top());
            QVERIFY(UiInspection::writeReport(page, QString("settings-%1").arg(width)));
        }
    }
};
QTEST_MAIN(FmSettingsTests)
#include "fm_settings_tests.moc"
