#include "CPP/7zip/UI/FileManager/ListViewDialog.h"
#include "CPP/7zip/UI/FileManager/ViewSettings.h"
#include <QApplication>
#include <QClipboard>
#include <QTextEdit>
#include <QTimer>
#include <QTreeWidget>
#include <QtTest>

class ListViewDialogTests : public QObject {
    Q_OBJECT
  private slots:
    void twoColumnsNormalizeAndTruncateDisplayOnly() {
        CListViewDialog dialog;
        dialog.NumColumns = 2;
        dialog.Strings.Add(UString("multiline"));
        dialog.Values.Add(UString(L"first\r\nsecond\nthird"));
        dialog.Strings.Add(UString("long"));
        const QString raw = QString(1100, QChar('x')) + QStringLiteral("😀 tail");
        dialog.Values.Add(ToUString(raw));
        dialog.Strings.Add(UString("missing value"));
        dialog.OnInit();
        auto *list = dialog.findChild<QTreeWidget *>();
        QCOMPARE(list->columnCount(), 2);
        QCOMPARE(list->topLevelItem(0)->text(1), QString("first second third"));
        QCOMPARE(list->topLevelItem(1)->text(1), QString(1024, QChar('x')) + " ...");
        QVERIFY(list->topLevelItem(2)->text(1).isEmpty());
        QCOMPARE(ToQString(dialog.Values[1]), raw);
        list->setCurrentItem(list->topLevelItem(1));
        QTest::keyClick(list, Qt::Key_Insert, Qt::ControlModifier);
#ifdef _WIN32
        QCOMPARE(QApplication::clipboard()->text(), "long: " + raw + "\r\n");
#else
        QCOMPARE(QApplication::clipboard()->text(), "long: " + raw + "\n");
#endif
    }

    void enterInPropertiesOpensFullValueWithoutAcceptingList() {
        CListViewDialog dialog;
        dialog.NumColumns = 2;
        dialog.SelectFirst = true;
        dialog.Strings.Add(UString("Name"));
        const QString raw = QString(1200, QChar('v')) + "\nnext line";
        dialog.Values.Add(ToUString(raw));
        dialog.OnInit();
        dialog.show();
        auto *list = dialog.findChild<QTreeWidget *>();
        bool checked = false;
        QTimer::singleShot(0, &dialog, [&] {
            auto *info = dialog.findChild<QDialog *>("listItemInfoDialog");
            QVERIFY(info);
            QCOMPARE(info->windowTitle(), QString("Name"));
            auto *text = info->findChild<QTextEdit *>();
            QVERIFY(text && text->isReadOnly());
            QCOMPARE(text->toPlainText(), raw);
            checked = true;
            info->reject();
        });
        QTest::keyClick(list, Qt::Key_Return);
        QVERIFY(checked);
        QVERIFY(dialog.isVisible());
        QCOMPARE(dialog.result(), int(QDialog::Rejected));
        QCOMPARE(dialog.FocusedItemIndex, -1);
        dialog.accept();
        QCOMPARE(dialog.result(), int(QDialog::Accepted));
        QCOMPARE(dialog.FocusedItemIndex, 0);
    }

    void deleteKeepsParallelStringsAndValuesAligned() {
        CListViewDialog dialog;
        dialog.NumColumns = 2;
        dialog.DeleteIsAllowed = true;
        for (int i = 0; i < 4; ++i) {
            dialog.Strings.Add(ToUString(QString::number(i)));
            dialog.Values.Add(ToUString(QString("value %1").arg(i)));
        }
        dialog.OnInit();
        auto *list = dialog.findChild<QTreeWidget *>();
        list->topLevelItem(1)->setSelected(true);
        list->topLevelItem(3)->setSelected(true);
        QTest::keyClick(list, Qt::Key_Delete);
        QCOMPARE(dialog.Strings.Size(), 2u);
        QCOMPARE(dialog.Values.Size(), 2u);
        QCOMPARE(ToQString(dialog.Strings[0]), QString("0"));
        QCOMPARE(ToQString(dialog.Values[0]), QString("value 0"));
        QCOMPARE(ToQString(dialog.Strings[1]), QString("2"));
        QCOMPARE(ToQString(dialog.Values[1]), QString("value 2"));
        QVERIFY(dialog.StringsWereChanged);
    }
};
QTEST_MAIN(ListViewDialogTests)
#include "list_view_dialog_tests.moc"
