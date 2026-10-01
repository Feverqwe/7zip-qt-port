#include "CPP/7zip/UI/FileManager/LangUtils.h"
#include "CPP/7zip/UI/FileManager/RegistryUtils.h"
#include "fileviewerdialog.h"
#include "layout_inspection.h"
#include "qtlangcontrols.h"
#include <QDialogButtonBox>
#include <QFile>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QTemporaryDir>
#include <QTextCursor>
#include <QtTest>

class LanguageViewerTests final : public QObject {
    Q_OBJECT
    QTemporaryDir directory;
    void language(const char *name) {
        const QString path = directory.filePath("language.ini");
        SaveRegLang(UString(name), path);
        ReloadLang(path);
        QtNotifyLanguageChanged();
        QApplication::processEvents();
    }

  private slots:
    void init() { language("-"); }
    void cleanup() { language("-"); }
    void editableContentsAndSelectionRemainPending() {
        const QString path = directory.filePath(QStringLiteral("viewer 密碼 file.txt"));
        QFile file(path);
        QVERIFY(file.open(QIODevice::WriteOnly));
        QCOMPARE(file.write("original"), qint64(8));
        file.close();
        FileViewerDialog dialog(path, true);
        dialog.show();
        auto *edit = dialog.findChild<QPlainTextEdit *>("fileViewerText");
        edit->setPlainText(QStringLiteral("pending contents 密碼"));
        QTextCursor cursor = edit->textCursor();
        cursor.setPosition(2);
        cursor.setPosition(7, QTextCursor::KeepAnchor);
        edit->setTextCursor(cursor);
        edit->document()->setModified(true);
        const QString selection = edit->textCursor().selectedText();
        for (const char *lang : {"ru", "-"}) {
            language(lang);
            QCOMPARE(edit->toPlainText(), QStringLiteral("pending contents 密碼"));
            QCOMPARE(edit->textCursor().selectedText(), selection);
            QVERIFY(edit->document()->isModified());
            QVERIFY(!dialog.wasSaved());
            QVERIFY(!edit->isReadOnly());
            QCOMPARE(dialog.windowTitle(), QString(lang) == "ru"
                                               ? QStringLiteral("Редактор — viewer 密碼 file.txt")
                                               : QStringLiteral("Editor — viewer 密碼 file.txt"));
            for (QSize size : {QSize(760, 560), QSize(420, 300)}) {
                dialog.resize(size);
                QApplication::processEvents();
                QVERIFY(UiInspection::fullyVisible(*edit, dialog));
                QVERIFY(UiInspection::writeReport(
                    dialog, QString("language-viewer-%1-%2").arg(lang).arg(size.width())));
                QVERIFY(!UiInspection::dump(dialog).contains("pending contents"));
            }
        }
        QVERIFY(file.open(QIODevice::ReadOnly));
        QCOMPARE(file.readAll(), QByteArray("original"));
        file.close();
        dialog.findChild<QPushButton *>("fileViewerSave")->click();
        QVERIFY(dialog.wasSaved());
    }
    void diagnosticsTranslateAndStayReadOnly() {
        const QString path = directory.filePath("binary.bin");
        QFile file(path);
        QVERIFY(file.open(QIODevice::WriteOnly));
        QCOMPARE(file.write(QByteArray("x\0y", 3)), qint64(3));
        file.close();
        FileViewerDialog dialog(path, true);
        auto *edit = dialog.findChild<QPlainTextEdit *>("fileViewerText");
        QCOMPARE(edit->toPlainText(), QString("The file is not UTF-8 text."));
        language("ru");
        QCOMPARE(edit->toPlainText(), QStringLiteral("Файл не является текстом UTF-8."));
        QVERIFY(edit->isReadOnly());
        QVERIFY(!dialog.findChild<QPushButton *>("fileViewerSave")->isEnabled());
        language("-");
        QCOMPARE(edit->toPlainText(), QString("The file is not UTF-8 text."));
        QVERIFY(!dialog.wasSaved());
    }
};
QTEST_MAIN(LanguageViewerTests)
#include "language_viewer_tests.moc"
