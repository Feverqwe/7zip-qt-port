#include "CPP/7zip/UI/FileManager/PanelSelect.h"
#include <QtTest>

class PanelSelectionTests final : public QObject {
    Q_OBJECT
  private slots:
    void maskPreservesUnmatchedSelection() {
        CPanelSelection panel;
        panel.Items = {{QStringLiteral("Альфа файл.txt"), false, false},
                       {QStringLiteral("бета.TXT"), false, false},
                       {"folder.txt", true, false},
                       {"keep.bin", false, true}};
        panel.SelectSpec(QStringView(u"*.txt"), true);
        for (const auto &item : panel.Items)
            QVERIFY(item.Selected);
        panel.SelectSpec(QStringView(u"*.TXT"), false);
        QVERIFY(!panel.Items[0].Selected);
        QVERIFY(!panel.Items[1].Selected);
        QVERIFY(!panel.Items[2].Selected);
        QVERIFY(panel.Items[3].Selected);
    }

    void typeUsesLastDotAndExcludesFolders() {
        CPanelSelection panel;
        panel.Items = {{"a.tar.gz", false, false},
                       {"b.GZ", false, false},
                       {"c.tar", false, true},
                       {"folder.gz", true, false}};
        panel.SelectByType(0, true);
        QVERIFY(panel.Items[0].Selected);
        QVERIFY(panel.Items[1].Selected);
        QVERIFY(panel.Items[2].Selected);
        QVERIFY(!panel.Items[3].Selected);
        panel.SelectByType(1, false);
        QVERIFY(!panel.Items[0].Selected);
        QVERIFY(!panel.Items[1].Selected);
        QVERIFY(panel.Items[2].Selected);
    }

    void foldersAndNamesWithoutExtensions() {
        CPanelSelection panel;
        panel.Items = {{"README", false, false},   {QStringLiteral("имя без точки"), false, false},
                       {"directory", true, false}, {"directory.txt", true, false},
                       {".hidden", false, false},  {"trailing.", false, false}};
        panel.SelectByType(0, true);
        QVERIFY(panel.Items[0].Selected);
        QVERIFY(panel.Items[1].Selected);
        for (size_t i = 2; i < panel.Items.size(); ++i)
            QVERIFY(!panel.Items[i].Selected);
        panel.SelectByType(3, true);
        QVERIFY(panel.Items[2].Selected);
        QVERIFY(panel.Items[3].Selected);
        panel.SelectByType(2, false);
        QVERIFY(!panel.Items[2].Selected);
        QVERIFY(!panel.Items[3].Selected);
        QVERIFY(panel.Items[0].Selected);
        panel.SelectByType(4, true);
        QVERIFY(panel.Items[4].Selected);
        QVERIFY(!panel.Items[5].Selected);
    }

    void allInvertAndMissingFocus() {
        CPanelSelection panel;
        panel.Items = {{"one", false, true}, {"two", true, false}};
        panel.SelectByType(-1, true);
        panel.SelectByType(2, false);
        QVERIFY(panel.Items[0].Selected);
        QVERIFY(!panel.Items[1].Selected);
        panel.InvertSelection();
        QVERIFY(!panel.Items[0].Selected);
        QVERIFY(panel.Items[1].Selected);
        panel.SelectAll(true);
        QVERIFY(panel.Items[0].Selected);
        QVERIFY(panel.Items[1].Selected);
        panel.SelectAll(false);
        QVERIFY(!panel.Items[0].Selected);
        QVERIFY(!panel.Items[1].Selected);
        panel.Items.clear();
        panel.InvertSelection();
        panel.SelectByType(0, true);
        QVERIFY(panel.Items.empty());
    }
};
QTEST_GUILESS_MAIN(PanelSelectionTests)
#include "panel_selection_tests.moc"
