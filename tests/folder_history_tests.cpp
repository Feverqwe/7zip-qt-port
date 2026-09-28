#include "CPP/7zip/UI/FileManager/AppState.h"
#include "CPP/7zip/UI/FileManager/ListViewDialog.h"
#include "CPP/7zip/UI/FileManager/Panel.h"
#include "layout_inspection.h"
#include <QApplication>
#include <QClipboard>
#include <QDialogButtonBox>
#include <QListWidget>
#include <QPushButton>
#include <QSettings>
#include <QTemporaryDir>
#include <QTimer>
#include <QtTest>

class FolderHistoryTests : public QObject {
    Q_OBJECT
  private slots:
    void upstreamLimitUniquenessAndPersistence() {
        QTemporaryDir temp;
        const QString path = temp.filePath("history.ini");
        CFolderHistory history;
        for (int i = 0; i < 105; ++i)
            history.AddString(ToUString(QString("/folder/%1").arg(i)));
        history.AddString(ToUString(QStringLiteral("/ПАПКА с пробелом/😀")));
        history.AddString(ToUString(QStringLiteral("/папка с пробелом/😀")));
        UStringVector list;
        history.GetList(list);
        QCOMPARE(list.Size(), 100u);
        QCOMPARE(ToQString(list[0]), QStringLiteral("/папка с пробелом/😀"));
        QCOMPARE(ToQString(list[1]), QString("/folder/104"));
        QCOMPARE(ToQString(list[99]), QString("/folder/6"));
        history.Save(path);
        CFolderHistory restored;
        restored.Read(path);
        restored.GetList(list);
        QCOMPARE(list.Size(), 100u);
        QCOMPARE(ToQString(list[0]), QStringLiteral("/папка с пробелом/😀"));
        QCOMPARE(ToQString(list[99]), QString("/folder/6"));
    }

    void readsLegacyAndTrimsOversizedHistory() {
        QTemporaryDir temp;
        const QString path = temp.filePath("history.ini");
        QStringList legacy;
        for (int i = 0; i < 120; ++i)
            legacy << QString("/old/%1").arg(i);
        {
            QSettings settings(path, QSettings::IniFormat);
            settings.setValue("history/paths", legacy);
        }
        CFolderHistory history;
        history.Read(path);
        UStringVector list;
        history.GetList(list);
        QCOMPARE(list.Size(), 100u);
        QCOMPARE(ToQString(list[0]), legacy[0]);
        history.RemoveAll();
        history.Save(path);
        history.Read(path);
        history.GetList(list);
        QVERIFY(list.IsEmpty()); // Empty new key must not resurrect the legacy key.
    }

    void selectionCopyDeleteAndConfirmation() {
        CListViewDialog dialog;
        for (const char *s : {"first", "second", "third", "fourth"})
            dialog.Strings.Add(UString(s));
        dialog.SelectFirst = true;
        dialog.DeleteIsAllowed = true;
        dialog.OnInit();
        dialog.show();
        auto *list = dialog.findChild<QListWidget *>();
        list->item(2)->setSelected(true);
        QTest::keyClick(list, Qt::Key_C, Qt::ControlModifier);
#ifdef _WIN32
        QCOMPARE(QApplication::clipboard()->text(), QString("first\r\nthird\r\n"));
#else
        QCOMPARE(QApplication::clipboard()->text(), QString("first\nthird\n"));
#endif
        QTest::keyClick(list, Qt::Key_Delete);
        QVERIFY(dialog.StringsWereChanged);
        QCOMPARE(dialog.Strings.Size(), 2u);
        QCOMPARE(ToQString(dialog.Strings[0]), QString("second"));
        QCOMPARE(ToQString(dialog.Strings[1]), QString("fourth"));
        list->setCurrentRow(1);
        dialog.accept();
        QCOMPARE(dialog.FocusedItemIndex, 1);
        QCOMPARE(dialog.result(), int(QDialog::Accepted));
    }

    void deletionRequiresPermissionAndEmptySelectionIsSafe() {
        CListViewDialog dialog;
        dialog.Strings.Add(UString("keep"));
        dialog.SelectFirst = true;
        dialog.OnInit();
        auto *list = dialog.findChild<QListWidget *>();
        QTest::keyClick(list, Qt::Key_Delete);
        QVERIFY(!dialog.StringsWereChanged);
        QCOMPARE(dialog.Strings.Size(), 1u);
        dialog.DeleteIsAllowed = true;
        QTest::keyClick(list, Qt::Key_A, Qt::ControlModifier);
        QTest::keyClick(list, Qt::Key_Delete);
        QVERIFY(dialog.Strings.IsEmpty());
        dialog.accept();
        QCOMPARE(dialog.FocusedItemIndex, -1);
    }

    void panelCommitsOnlyAcceptedChanges_data() {
        QTest::addColumn<bool>("accept");
        QTest::newRow("ok") << true;
        QTest::newRow("cancel") << false;
    }
    void panelCommitsOnlyAcceptedChanges() {
        QFETCH(bool, accept);
        QTemporaryDir temp;
        const QString path = temp.filePath("history.ini");
        CFolderHistory history;
        for (const char *s : {"third", "second", "first"})
            history.AddString(UString(s));
        history.Save(path);
        QWidget parent;
        QTimer::singleShot(0, &parent, [&] {
            auto *dialog = parent.findChild<CListViewDialog *>();
            QVERIFY(dialog);
            auto *list = dialog->findChild<QListWidget *>();
            QTest::keyClick(list, Qt::Key_Delete);
            list->setCurrentRow(1);
            if (accept)
                dialog->accept();
            else
                dialog->reject();
        });
        const QString selected = CPanel::FoldersHistory(history, path, &parent);
        QCOMPARE(selected, accept ? QString("third") : QString());
        CFolderHistory restored;
        restored.Read(path);
        UStringVector list;
        restored.GetList(list);
        QCOMPARE(list.Size(), accept ? 2u : 3u);
        QCOMPARE(ToQString(list[0]), accept ? QString("second") : QString("first"));
    }

    void geometry_data() {
        QTest::addColumn<QSize>("size");
        QTest::newRow("normal") << QSize(540, 360);
        QTest::newRow("compact") << QSize(360, 240);
    }
    void geometry() {
        QFETCH(QSize, size);
        CListViewDialog dialog;
        dialog.Title = ToUString(QStringLiteral("История папок"));
        dialog.Strings.Add(ToUString(QString("/long path/").repeated(60)));
        dialog.SelectFirst = true;
        dialog.OnInit();
        dialog.resize(size);
        dialog.show();
        QApplication::processEvents();
        auto *list = dialog.findChild<QListWidget *>();
        auto *buttons = dialog.findChild<QDialogButtonBox *>();
        QVERIFY(UiInspection::fullyVisible(*list, dialog));
        QVERIFY(UiInspection::fullyVisible(*buttons, dialog));
        QVERIFY(UiInspection::separate(*list, *buttons, dialog));
        QCOMPARE(list->textElideMode(), Qt::ElideNone);
        for (auto *button : buttons->buttons())
            QVERIFY(UiInspection::fullyVisible(*button, dialog));
        QVERIFY(UiInspection::writeReport(dialog, QString("folder-history-%1").arg(size.width())));
    }
};
QTEST_MAIN(FolderHistoryTests)
#include "folder_history_tests.moc"
