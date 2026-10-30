#include "dialogs/edit/EditDialog.h"
#include "settings/ViewSettings.h"
#include "ui/support/layout_inspection.h"
#include <QApplication>
#include <QClipboard>
#include <QDialogButtonBox>
#include <QMimeData>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QScopeGuard>
#include <QScrollBar>
#include <QtTest>
#include <memory>

class EditDialogTests : public QObject {
    Q_OBJECT
  private slots:
    void initialReturnClosesAndTabReachesTheReadOnlyEdit() {
        for (bool tab : {false, true}) {
            CEditDialog dialog;
            dialog.OnInit();
            dialog.show();
            QVERIFY(QTest::qWaitForWindowExposed(&dialog));
            auto *close = dialog.findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Close);
            QCOMPARE(dialog.focusWidget(), close);
            QVERIFY(close->isDefault());
            if (tab) {
                QTest::keyClick(close, Qt::Key_Tab);
                auto *edit = dialog.findChild<QPlainTextEdit *>("editText");
                QCOMPARE(dialog.focusWidget(), edit);
                QTest::keyClick(edit, Qt::Key_Return);
                QVERIFY(dialog.isVisible());
            } else {
                QTest::keyClick(close, Qt::Key_Return);
                QVERIFY(!dialog.isVisible());
            }
        }
    }
    void displaysRawUnicodeTextWithoutInterpretationOrEditing() {
        auto *clipboard = QApplication::clipboard();
        auto saved = std::make_unique<QMimeData>();
        if (clipboard->mimeData())
            for (const QString &type : clipboard->mimeData()->formats())
                saved->setData(type, clipboard->mimeData()->data(type));
        const auto restore = qScopeGuard([&] { clipboard->setMimeData(saved.release()); });
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
        auto *copy = dialog.findChild<QPushButton *>("copyReportButton");
        copy->click();
        QCOMPARE(clipboard->text(), raw);
        auto cursor = edit->textCursor();
        cursor.setPosition(0);
        cursor.setPosition(raw.indexOf('\n'), QTextCursor::KeepAnchor);
        edit->setTextCursor(cursor);
        copy->click();
        QCOMPARE(clipboard->text(), raw.section('\n', 0, 0));
        QVERIFY(dialog.isVisible());
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
        QTRY_VERIFY(edit->horizontalScrollBar()->maximum() > 0);
        QTRY_VERIFY(edit->verticalScrollBar()->maximum() > 0);
        QVERIFY(UiInspection::fullyVisible(*buttons->button(QDialogButtonBox::Close), dialog));
        QCOMPARE(edit->toPlainText(), raw);
        QVERIFY(UiInspection::writeReport(dialog, QString("edit-dialog-%1").arg(size.width())));
    }
};
#include "ui/support/review_test_main.h"
SEVENZIP_UI_TEST_MAIN(EditDialogTests)
#include "edit_dialog_tests.moc"
