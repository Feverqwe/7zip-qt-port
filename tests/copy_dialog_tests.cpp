#include "CPP/7zip/UI/FileManager/CopyDialog.h"
#include "CPP/7zip/UI/FileManager/Panel.h"
#include "CPP/7zip/UI/FileManager/ViewSettings.h"
#include "layout_inspection.h"

#include <QComboBox>
#include <QDialogButtonBox>
#include <QDir>
#include <QFileDialog>
#include <QLabel>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSettings>
#include <QTemporaryDir>
#include <QTest>
#include <QTimer>

class CopyDialogTests : public QObject {
    Q_OBJECT

  private slots:
    void initTestCase() { QApplication::setAttribute(Qt::AA_DontUseNativeDialogs); }

    void copiesFieldsAndCommitsOnlyOnOK() {
        CCopyDialog dialog(false, QStringLiteral("/начальная папка/"),
                           {QStringLiteral("/tmp/имя с пробелом.txt")}, {"/one", "/two"});
        QCOMPARE(dialog.Title, QString("Копировать"));
        auto *path = dialog.findChild<QComboBox *>("copyDestination");
        QVERIFY(path);
        QCOMPARE(path->count(), 2);
        QCOMPARE(path->itemText(1), QString("/two"));
        auto *info = dialog.findChild<QPlainTextEdit *>("copySelectionInfo");
        QVERIFY(info && info->isReadOnly());
        QCOMPARE(info->toPlainText(), QStringLiteral("имя с пробелом.txt"));
        path->setCurrentText(QStringLiteral("  /новая папка/  "));
        dialog.reject();
        QCOMPARE(dialog.Value, QStringLiteral("/начальная папка/"));
        dialog.accept();
        QCOMPARE(dialog.Value, QStringLiteral("  /новая папка/  "));
        QCOMPARE(dialog.destination(), dialog.Value);
        CCopyDialog move(true, "/initial", {}, {});
        QCOMPARE(move.Static, QString("Переместить в:"));
        move.findChild<QComboBox *>("copyDestination")->setCurrentText("/by-button");
        move.findChild<QDialogButtonBox *>("copyButtons")->button(QDialogButtonBox::Ok)->click();
        QCOMPARE(move.Value, QString("/by-button"));
        QCOMPARE(move.result(), int(QDialog::Accepted));
    }

    void historyRemovesAllCaseVariantsAndMigratesINI() {
        QTemporaryDir temp;
        QVERIFY(temp.isValid());
        const QString ini = temp.filePath(QStringLiteral("настройки с пробелом.ini"));
        {
            QSettings settings(ini, QSettings::IniFormat);
            settings.setValue("copy/history", QStringList{"/old", "/OLDER"});
            settings.setValue("unrelated/value", 17);
        }
        QStringList history{"stale"};
        ReadCopyHistory(history, ini);
        QCOMPARE(history, (QStringList{"/old", "/OLDER"}));
        history << "/NEW" << "/new" << "/NeW";
        AddUniqueStringToHeadOfList(history, "/new");
        QCOMPARE(history, (QStringList{"/new", "/old", "/OLDER"}));
        QStringList unicode{QStringLiteral("/ПАПКА"), "/keep", QStringLiteral("/папка")};
        AddUniqueStringToHeadOfList(unicode, QStringLiteral("/Папка"));
        QCOMPARE(unicode, (QStringList{QStringLiteral("/Папка"), "/keep"}));
        SaveCopyHistory(history, ini);
        history.clear();
        ReadCopyHistory(history, ini);
        QCOMPARE(history, (QStringList{"/new", "/old", "/OLDER"}));
        QSettings settings(ini, QSettings::IniFormat);
        QCOMPARE(settings.value("unrelated/value").toInt(), 17);
        QCOMPARE(settings.value("copy/history").toStringList(), (QStringList{"/old", "/OLDER"}));
        SaveCopyHistory({}, ini);
        ReadCopyHistory(history, ini);
        QVERIFY(history.isEmpty());
        ReadCopyHistory(history, temp.filePath("missing.ini"));
        QVERIFY(history.isEmpty());
    }

    void browseNormalizesDirectoryAndCancelPreservesText() {
        QTemporaryDir temp;
        QVERIFY(temp.isValid());
        const QString chosen = temp.filePath(QStringLiteral("папка назначения"));
        QVERIFY(QDir().mkdir(chosen));
        CCopyDialog dialog(false, temp.path(), {}, {chosen});
        auto *path = dialog.findChild<QComboBox *>("copyDestination");
        auto *browse = dialog.findChild<QPushButton *>("copyBrowseButton");
        dialog.show();
        bool found = false;
        QTimer::singleShot(0, [&found] {
            for (QWidget *widget : QApplication::topLevelWidgets()) {
                if (auto *picker = qobject_cast<QFileDialog *>(widget);
                    picker && picker->isVisible()) {
                    found = true;
                    picker->reject();
                }
            }
        });
        browse->click();
        QVERIFY(found);
        QCOMPARE(path->currentText(), temp.path());
        found = false;
        QTimer::singleShot(0, [&found, &chosen] {
            for (QWidget *widget : QApplication::topLevelWidgets()) {
                if (auto *picker = qobject_cast<QFileDialog *>(widget);
                    picker && picker->isVisible()) {
                    found = true;
                    picker->setDirectory(chosen);
                    QMetaObject::invokeMethod(picker, "accept", Qt::DirectConnection);
                }
            }
        });
        browse->click();
        QVERIFY(found);
        QCOMPARE(path->currentIndex(), -1);
        QCOMPARE(path->currentText(), chosen + '/');
        dialog.accept();
        QCOMPARE(dialog.Value, chosen + '/');
    }

    void geometryAndLongSelection_data() {
        QTest::addColumn<bool>("move");
        QTest::addColumn<QSize>("size");
        for (bool move : {false, true}) {
            QTest::newRow(move ? "move-normal" : "copy-normal") << move << QSize(440, 250);
            QTest::newRow(move ? "move-compact" : "copy-compact") << move << QSize(340, 220);
        }
    }

    void geometryAndLongSelection() {
        QFETCH(bool, move);
        QFETCH(QSize, size);
        QStringList sources;
        CObjectVector<CPanelItem> items;
        CRecordVector<UInt32> indices;
        for (int i = 0; i < 40; ++i) {
            sources << QStringLiteral("/tmp/очень длинное имя с пробелами %1.txt").arg(i);
            indices.Add(items.Add({QFileInfo(sources.last()).fileName(), i == 0, true, 1000}));
        }
        const QString summary = CPanel("/initial/", items).GetItemsInfoString(indices);
        CCopyDialog dialog(move, "/initial", sources, {"/one"}, nullptr, summary);
        dialog.resize(size);
        dialog.show();
        QCoreApplication::processEvents();
        auto *label = dialog.findChild<QLabel *>("copyDestinationLabel");
        auto *path = dialog.findChild<QComboBox *>("copyDestination");
        auto *browse = dialog.findChild<QPushButton *>("copyBrowseButton");
        auto *info = dialog.findChild<QPlainTextEdit *>("copySelectionInfo");
        auto *buttons = dialog.findChild<QDialogButtonBox *>("copyButtons");
        QVERIFY(label && path && browse && info && buttons);
        for (QWidget *widget : QList<QWidget *>{label, path, browse, info, buttons})
            QVERIFY(UiInspection::fullyVisible(*widget, dialog));
        QVERIFY(UiInspection::labelFits(*label));
        QVERIFY(UiInspection::separate(*path, *browse, dialog));
        QVERIFY(UiInspection::bounds(*label, dialog).bottom() <
                UiInspection::bounds(*path, dialog).top());
        QVERIFY(UiInspection::bounds(*path, dialog).bottom() <
                UiInspection::bounds(*info, dialog).top());
        QVERIFY(UiInspection::bounds(*info, dialog).bottom() <
                UiInspection::bounds(*buttons, dialog).top());
        QVERIFY(UiInspection::separate(*buttons->button(QDialogButtonBox::Ok),
                                       *buttons->button(QDialogButtonBox::Cancel), dialog));
        QCOMPARE(info->toPlainText(), summary);
        QCOMPARE(info->toPlainText().split('\n').size(), kCopyDialog_NumInfoLines);
        info->moveCursor(QTextCursor::End);
        QCOMPARE(info->textCursor().blockNumber(), kCopyDialog_NumInfoLines - 1);
        QVERIFY(UiInspection::writeReport(
            dialog, QString("copy-dialog-%1-%2").arg(move).arg(size.width())));
    }
};

QTEST_MAIN(CopyDialogTests)
#include "copy_dialog_tests.moc"
