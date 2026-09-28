#include "CPP/7zip/UI/FileManager/EditDialog.h"
#include "CPP/7zip/UI/FileManager/ViewSettings.h"
#include "layout_inspection.h"
#include <QDialogButtonBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QScrollBar>
#include <QtTest>

class EditDialogTests : public QObject {
    Q_OBJECT
  private slots:
    void displaysRawUnicodeTextWithoutInterpretationOrEditing() {
        CEditDialog dialog;
        dialog.Title = ToUString(QStringLiteral("Свойство 😀"));
        const QString raw = QStringLiteral("<b>имя с пробелом 😀</b>\nsecond line");
        dialog.Text = ToUString(raw);
        dialog.OnInit();
        QCOMPARE(dialog.windowTitle(), QStringLiteral("Свойство 😀"));
        auto *edit = dialog.findChild<QPlainTextEdit *>("editText");
        QVERIFY(edit && edit->isReadOnly());
        QCOMPARE(edit->toPlainText(), raw);
        QCOMPARE(edit->lineWrapMode(), QPlainTextEdit::NoWrap);
        dialog.show();
        QTest::keyClicks(edit, "overwrite");
        QCOMPARE(edit->toPlainText(), raw);
        QTest::keyClick(edit, Qt::Key_Return);
        QVERIFY(dialog.isVisible());
        QCOMPARE(ToQString(dialog.Text), raw);
        dialog.findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Close)->click();
        QCOMPARE(dialog.result(), int(QDialog::Rejected));
        QVERIFY(!dialog.isVisible());
    }

    void geometry_data() {
        QTest::addColumn<QSize>("size");
        QTest::newRow("normal") << QSize(480, 320);
        QTest::newRow("compact") << QSize(320, 240);
    }
    void geometry() {
        QFETCH(QSize, size);
        CEditDialog dialog;
        dialog.Title = ToUString(QStringLiteral("Полное значение"));
        QString raw = QString("long value ").repeated(150);
        for (int i = 0; i < 80; ++i)
            raw += QString("\nline %1").arg(i);
        dialog.Text = ToUString(raw);
        dialog.OnInit();
        dialog.resize(size);
        dialog.show();
        QApplication::processEvents();
        auto *edit = dialog.findChild<QPlainTextEdit *>();
        auto *buttons = dialog.findChild<QDialogButtonBox *>();
        QVERIFY(UiInspection::fullyVisible(*edit, dialog));
        QVERIFY(UiInspection::fullyVisible(*buttons, dialog));
        QVERIFY(UiInspection::separate(*edit, *buttons, dialog));
        QVERIFY(edit->horizontalScrollBar()->maximum() > 0);
        QVERIFY(edit->verticalScrollBar()->maximum() > 0);
        QVERIFY(UiInspection::fullyVisible(*buttons->button(QDialogButtonBox::Close), dialog));
        QCOMPARE(edit->toPlainText(), raw);
        QVERIFY(UiInspection::writeReport(dialog, QString("edit-dialog-%1").arg(size.width())));
    }
};
QTEST_MAIN(EditDialogTests)
#include "edit_dialog_tests.moc"
