#include "CPP/7zip/UI/FileManager/BrowseDialog.h"
#include "CPP/7zip/UI/FileManager/ComboDialog.h"
#include "CPP/7zip/UI/FileManager/ViewSettings.h"
#include "layout_inspection.h"
#include <QComboBox>
#include <QDialogButtonBox>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QTimer>
#include <QtTest>

class ComboDialogTests : public QObject {
    Q_OBJECT
  private slots:
    void initializesValueAndOrderedStringsWithoutReplacingEdit() {
        const QStringList strings = {"first", QStringLiteral("второе имя"), "third"};
        CComboDialog dialog("Заголовок", "Имя <буквально>:", "initial", nullptr, strings);
        QCOMPARE(dialog.windowTitle(), QStringLiteral("Заголовок"));
        auto *combo = dialog.findChild<QComboBox *>("comboValue");
        QCOMPARE(combo->count(), strings.size());
        for (int i = 0; i < strings.size(); ++i)
            QCOMPARE(combo->itemText(i), strings[i]);
        QCOMPARE(combo->currentText(), QStringLiteral("initial"));
        QCOMPARE(ToQString(dialog.Value), QStringLiteral("initial"));
        auto *label = dialog.findChild<QLabel *>("comboPrompt");
        QCOMPARE(label->textFormat(), Qt::PlainText);
        QCOMPARE(label->text(), QStringLiteral("Имя <буквально>:"));
        dialog.show();
        QApplication::processEvents();
        QVERIFY(combo->hasFocus() || combo->lineEdit()->hasFocus());
        QCOMPARE(combo->lineEdit()->selectedText(), QStringLiteral("initial"));
    }

    void confirmationPreservesRawValue_data() {
        QTest::addColumn<bool>("button");
        QTest::addColumn<QString>("value");
        QTest::newRow("accept") << false << QStringLiteral("  имя с пробелом  ");
        QTest::newRow("button") << true << QStringLiteral("*.txt");
        QTest::newRow("empty") << false << QString();
    }
    void confirmationPreservesRawValue() {
        QFETCH(bool, button);
        QFETCH(QString, value);
        CComboDialog dialog("Title", "Prompt", "before");
        dialog.findChild<QComboBox *>("comboValue")->setCurrentText(value);
        if (button)
            dialog.findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Ok)->click();
        else
            dialog.accept();
        QCOMPARE(dialog.result(), int(QDialog::Accepted));
        QCOMPARE(ToQString(dialog.Value), value);
        QCOMPARE(dialog.value(), value);
    }

    void cancelPreservesPublicValueAndFolderHelperClearsOutput() {
        CComboDialog dialog("Title", "Prompt", "before");
        dialog.findChild<QComboBox *>("comboValue")->setCurrentText("edited");
        dialog.reject();
        QCOMPARE(ToQString(dialog.Value), QStringLiteral("before"));
        QWidget parent;
        UString name = L"previous";
        QString initialName;
        bool found = false;
        QTimer::singleShot(0, &parent, [&] {
            if (auto *folder = parent.findChild<CComboDialog *>("comboDialog")) {
                found = true;
                initialName = ToQString(folder->Value);
                folder->reject();
            }
        });
        QVERIFY(!Dlg_CreateFolder(&parent, name));
        QVERIFY(found);
        QCOMPARE(initialName, QString("New Folder"));
        QVERIFY(name.IsEmpty());
        QTimer::singleShot(0, &parent, [&] {
            if (auto *folder = parent.findChild<CComboDialog *>("comboDialog")) {
                folder->findChild<QComboBox *>("comboValue")
                    ->setCurrentText(QStringLiteral("папка с пробелом"));
                folder->accept();
            }
        });
        QVERIFY(Dlg_CreateFolder(&parent, name));
        QCOMPARE(ToQString(name), QStringLiteral("папка с пробелом"));
    }

    void geometry_data() {
        QTest::addColumn<QString>("title");
        QTest::addColumn<QString>("prompt");
        QTest::addColumn<int>("width");
        for (int width : {360, 260}) {
            QTest::newRow(qPrintable(QStringLiteral("folder-%1").arg(width)))
                << QString("Создать папку") << QString("Имя папки:") << width;
            QTest::newRow(qPrintable(QStringLiteral("file-%1").arg(width)))
                << QString("Создать файл") << QString("Имя файла:") << width;
            QTest::newRow(qPrintable(QStringLiteral("mask-%1").arg(width)))
                << QString("Выделить") << QString("Маска:") << width;
        }
    }
    void geometry() {
        QFETCH(QString, title);
        QFETCH(QString, prompt);
        QFETCH(int, width);
        CComboDialog dialog(title, prompt, QStringLiteral("длинное имя с пробелами и Unicode"));
        dialog.resize(width, 150);
        dialog.show();
        QApplication::processEvents();
        auto *label = dialog.findChild<QLabel *>("comboPrompt");
        auto *combo = dialog.findChild<QComboBox *>("comboValue");
        auto *buttons = dialog.findChild<QDialogButtonBox *>("comboButtons");
        QVERIFY(UiInspection::fullyVisible(*label, dialog));
        QVERIFY(UiInspection::labelFits(*label));
        QVERIFY(UiInspection::fullyVisible(*combo, dialog));
        QVERIFY(UiInspection::fullyVisible(*buttons, dialog));
        QVERIFY(UiInspection::bounds(*label, dialog).bottom() <
                UiInspection::bounds(*combo, dialog).top());
        QVERIFY(UiInspection::separate(*combo, *buttons, dialog));
        QVERIFY(UiInspection::separate(*buttons->button(QDialogButtonBox::Ok),
                                       *buttons->button(QDialogButtonBox::Cancel), dialog));
        QVERIFY(
            UiInspection::writeReport(dialog, QStringLiteral("combo-%1-%2")
                                                  .arg(QString::fromLatin1(QTest::currentDataTag()))
                                                  .arg(width)));
    }
};
QTEST_MAIN(ComboDialogTests)
#include "combo_dialog_tests.moc"
