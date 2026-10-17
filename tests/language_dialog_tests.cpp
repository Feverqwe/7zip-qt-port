#include "7zip/UI/FileManager/resource.h"
#include "CPP/7zip/UI/FileManager/ComboDialog.h"
#include "CPP/7zip/UI/FileManager/CopyDialog.h"
#include "CPP/7zip/UI/FileManager/LangUtils.h"
#include "CPP/7zip/UI/FileManager/MemDialog.h"
#include "CPP/7zip/UI/FileManager/MessagesDialog.h"
#include "CPP/7zip/UI/FileManager/OverwriteDialog.h"
#include "CPP/7zip/UI/FileManager/PasswordDialog.h"
#include "CPP/7zip/UI/FileManager/ProgressDialog2.h"
#include "CPP/7zip/UI/FileManager/RegistryUtils.h"
#include "CPP/7zip/UI/FileManager/SplitDialog.h"
#include "CPP/7zip/UI/FileManager/ViewSettings.h"
#include "CPP/7zip/UI/GUI/ExtractDialog.h"
#include "fixtures/split_dialog_prompt.h"
#include "layout_inspection.h"
#include "qtlangcontrols.h"
#include <QCheckBox>
#include <QComboBox>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QRadioButton>
#include <QScopeGuard>
#include <QStyle>
#include <QStyleOptionButton>
#include <QTemporaryDir>
#include <QTimer>
#include <QTreeWidget>
#include <QtTest>

namespace {
class LanguageProgressTransport final : public IQtProgressTransport {
  public:
    bool Paused = false;
    bool Background = false;
    int Requests = 0;
    HRESULT SetPaused(bool value) override {
        Paused = value;
        ++Requests;
        return S_OK;
    }
    HRESULT SetBackground(bool value) override {
        Background = value;
        ++Requests;
        return S_OK;
    }
};
class LanguageProgressDialog final : public CProgressDialog {
  public:
    using CProgressDialog::CProgressDialog;
    void attach(IQtProgressTransport &transport) { AttachTransport(&transport); }
};
} // namespace

class LanguageDialogTests final : public QObject {
    Q_OBJECT
    QTemporaryDir _settings;
    void language(const char *name) {
        const QString path = _settings.filePath("language.ini");
        SaveRegLang(UString(name), path);
        ReloadLang(path);
        QtNotifyLanguageChanged();
        QApplication::processEvents();
    }
    void inspect(QWidget &dialog, const QString &name, int width, int height) {
        const bool wasVisible = dialog.isVisible();
        if (!wasVisible)
            dialog.resize(width, height);
        dialog.show();
        if (QGuiApplication::platformName() != "offscreen") {
            dialog.raise();
            dialog.activateWindow();
            QVERIFY(QTest::qWaitForWindowActive(&dialog));
            QVERIFY(QTest::qWaitForWindowExposed(&dialog));
        }
        if (wasVisible)
            dialog.resize(width, height);
        QApplication::processEvents();
        // Retain current-build evidence even when a fitting assertion fails.
        QVERIFY(UiInspection::writeReport(dialog, name + '-' + QString::number(width)));
        QTRY_COMPARE(dialog.size(), QSize(width, height).expandedTo(dialog.minimumSizeHint()));
        // Publish the settled geometry after asynchronous native configure events.
        QVERIFY(UiInspection::writeReport(dialog, name + '-' + QString::number(width)));
        for (auto *label : dialog.findChildren<QLabel *>())
            if (label->isVisible() && !label->text().isEmpty()) {
                QVERIFY2(UiInspection::fullyVisible(*label, dialog),
                         qPrintable(label->objectName()));
                QVERIFY2(UiInspection::labelFits(*label), qPrintable(label->objectName()));
            }
        for (auto *button : dialog.findChildren<QPushButton *>())
            if (button->isVisible()) {
                QVERIFY(UiInspection::fullyVisible(*button, dialog));
                // Source Browse controls can be narrower than the style's
                // preferred footer button width. Check the displayed caption.
                QStyleOptionButton option;
                option.initFrom(button);
                option.text = button->text();
                const QRect contents =
                    button->style()->subElementRect(QStyle::SE_PushButtonContents, &option, button);
                QVERIFY2(button->fontMetrics().size(Qt::TextShowMnemonic, button->text()).width() <=
                             contents.width(),
                         qPrintable(button->objectName()));
            }
    }

  private slots:
    void init() { language("-"); }
    void cleanup() { language("-"); }
    void passwordPreservesPendingSecretAndFocus() {
        CPasswordDialog dialog;
        dialog.OnInit();
        dialog.show();
        auto *edit = dialog.findChild<QLineEdit *>("passwordEdit");
        auto *show = dialog.findChild<QCheckBox *>("passwordShow");
        edit->setText(QStringLiteral("密碼 pending secret"));
        show->click();
        edit->setFocus();
        edit->setSelection(2, 4);
        const QString selected = edit->selectedText();
        QApplication::processEvents();
        QCOMPARE(dialog.windowTitle(), QString("Enter password"));
        language("ru");
        QCOMPARE(dialog.windowTitle(), QStringLiteral("Ввод пароля"));
        QCOMPARE(edit->text(), QStringLiteral("密碼 pending secret"));
        QCOMPARE(edit->selectedText(), selected);
        QVERIFY(edit->hasFocus());
        QVERIFY(show->isChecked());
        QCOMPARE(edit->echoMode(), QLineEdit::Normal);
        QVERIFY(!UiInspection::dump(dialog).contains("pending secret"));
        for (int width : {360, 260})
            inspect(dialog, "language-password-ru", width, 170);
        language("-");
        QCOMPARE(dialog.windowTitle(), QString("Enter password"));
        for (int width : {360, 260})
            inspect(dialog, "language-password-en", width, 170);
    }
    void copySplitAndComboPreserveRawValues() {
        CCopyDialog copy(false, "/initial", {"Unicode file 密碼"}, {"/history"});
        CSplitDialog split("file 密碼.bin", "/initial");
        CComboDialog folder(QtLangText(IDS_CREATE_FOLDER), QtLangText(IDS_CREATE_FOLDER_NAME),
                            QtLangText(IDS_CREATE_FOLDER_DEFAULT_NAME));
        folder.BindLanguage(IDS_CREATE_FOLDER, IDS_CREATE_FOLDER_NAME);
        auto *destination = copy.findChild<QComboBox *>("copyDestination");
        auto *volume = split.findChild<QComboBox *>("volumeSize");
        auto *name = folder.findChild<QComboBox *>("comboValue");
        destination->setEditText(QStringLiteral(" /pending path 密碼 "));
        volume->setEditText("bad-volume");
        name->setEditText(QStringLiteral(" pending folder 密碼 "));
        split.show();
        QVERIFY(QTest::qWaitForWindowActive(&split));
        const auto englishPrompt = SubmitInvalidSplit(split, "language-split-error-en");
        QVERIFY(englishPrompt.shown);
        QVERIFY(englishPrompt.sourcePolicy);
        QVERIFY(englishPrompt.geometry);
        QCOMPARE(split.result(), int(QDialog::Rejected));
        const QString englishError = englishPrompt.text;
        language("ru");
        QCOMPARE(copy.windowTitle(), QStringLiteral("Копировать"));
        QCOMPARE(destination->currentText(), QStringLiteral(" /pending path 密碼 "));
        QCOMPARE(destination->count(), 1);
        QCOMPARE(volume->currentText(), QString("bad-volume"));
        QCOMPARE(name->currentText(), QStringLiteral(" pending folder 密碼 "));
        QVERIFY(split.windowTitle().endsWith(" file 密碼.bin"));
        const auto russianPrompt = SubmitInvalidSplit(split, "language-split-error-ru");
        QVERIFY(russianPrompt.shown);
        QVERIFY(russianPrompt.sourcePolicy);
        QVERIFY(russianPrompt.geometry);
        QVERIFY(russianPrompt.text != englishError);
        // The source opens these as separate modal forms. Keep the geometry
        // probe's inspected view exposed instead of stacking three dialogs.
        split.hide();
        for (int width : {480, 300}) {
            inspect(copy, "language-copy-ru", width, 280);
            copy.hide();
            inspect(split, "language-split-ru", width, 280);
            split.hide();
            inspect(folder, "language-folder-ru", width, 170);
            folder.hide();
        }
        language("-");
        split.show();
        QVERIFY(QTest::qWaitForWindowActive(&split));
        const auto restoredPrompt = SubmitInvalidSplit(split);
        QVERIFY(restoredPrompt.shown);
        QCOMPARE(restoredPrompt.text, englishError);
        split.hide();
        for (int width : {480, 300}) {
            inspect(copy, "language-copy-en", width, 280);
            copy.hide();
            inspect(split, "language-split-en", width, 280);
            split.hide();
            inspect(folder, "language-folder-en", width, 170);
            folder.hide();
        }
        copy.accept();
        QCOMPARE(ToQString(copy.Value), QStringLiteral(" /pending path 密碼 "));
    }
    void overwriteMemoryPreserveDecisions() {
        COverwriteDialog overwrite;
        overwrite.DefaultButton_is_NO = true;
        overwrite.OldFileInfo.Path = L"/folder 密碼/old.bin";
        overwrite.OldFileInfo.SetSize(12345);
        overwrite.NewFileInfo.Path = L"/folder 密碼/new.bin";
        overwrite.OnInit();
        const QString oldInfo = overwrite.findChild<QLabel *>("overwriteOldFile")->text();
        CMemDialog memory;
        memory.Required_GB = 0;
        memory.OnInit();
        memory.findChild<QCheckBox *>("memSaveLimit")->click();
        auto *limit = memory.findChild<QLineEdit *>("memLimit");
        limit->setText(" 007 pending ");
        memory.findChild<QRadioButton *>("memSkip")->click();
        memory.findChild<QCheckBox *>("memRemember")->click();
        language("ru");
        QCOMPARE(overwrite.windowTitle(), QStringLiteral("Подтверждение замены файла"));
        QVERIFY(overwrite.findChild<QPushButton *>("overwriteNo")->isDefault());
        QVERIFY(overwrite.findChild<QLabel *>("overwriteOldFile")->text() != oldInfo);
        QVERIFY(overwrite.findChild<QLabel *>("overwriteOldFile")->text().contains("old.bin"));
        QCOMPARE(limit->text(), QString(" 007 pending "));
        QVERIFY(limit->isEnabled());
        QVERIFY(memory.findChild<QRadioButton *>("memSkip")->isChecked());
        QVERIFY(memory.findChild<QCheckBox *>("memRemember")->isChecked());
        for (int width : {700, 560}) {
            inspect(overwrite, "language-overwrite-ru", width, 440);
            inspect(memory, "language-memory-ru", width, 460);
        }
        language("-");
        QCOMPARE(overwrite.findChild<QLabel *>("overwriteOldFile")->text(), oldInfo);
        for (int width : {700, 560}) {
            inspect(overwrite, "language-overwrite-en", width, 440);
            inspect(memory, "language-memory-en", width, 460);
        }
    }
    void modalMessagesPreserveRowsAcrossLanguageChanges() {
        UStringVector values;
        values.Add(L"message 密碼\nnext row");
        CMessagesDialog messages;
        messages.Messages = &values;
        bool visited = false;
        QTimer::singleShot(0, &messages, [&] {
            const auto close = qScopeGuard([&] { messages.accept(); });
            visited = true;
            auto *list = messages.findChild<QTreeWidget *>("IDL_MESSAGE");
            const int rows = list->topLevelItemCount();
            QCOMPARE(rows, 2);
            for (const char *name : {"ru", "-"}) {
                language(name);
                QCOMPARE(list->topLevelItemCount(), rows);
                QCOMPARE(list->topLevelItem(0)->text(1), QString("message 密碼"));
                QCOMPARE(list->headerItem()->text(1),
                         QString(name) == "ru" ? QStringLiteral("Сообщение") : QString("Message"));
                for (int width : {700, 560})
                    inspect(messages, "language-messages-" + QString(name), width, 400);
            }
        });
        QCOMPARE(messages.Create(), INT_PTR(IDOK));
        QVERIFY(visited);
        QVERIFY(!messages.isVisible());
    }
    void extractPreservesModesHistoryAndSecret() {
        CExtractDialog dialog("/initial path", {"/history"});
        const QString archive = QStringLiteral("/archives/исходный 空.zip");
        dialog.ArcPath = archive.toStdWString().c_str();
        dialog.show();
        dialog.SetInitialOptions(NExtract::NPathMode::kNoPaths,
                                 NExtract::NOverwriteMode::kRenameExisting,
                                 QStringLiteral("密碼 pending secret"));
        auto *destination = dialog.findChild<QComboBox *>("extractDestination");
        auto *pathMode = dialog.findChild<QComboBox *>("extractPathMode");
        auto *overwriteMode = dialog.findChild<QComboBox *>("extractOverwriteMode");
        auto *name = dialog.findChild<QLineEdit *>("extractPathName");
        dialog.findChild<QCheckBox *>("extractNameEnable")->setChecked(true);
        destination->setEditText("/pending path");
        name->setText(QStringLiteral(" pending name 密碼 "));
        QCOMPARE(dialog.windowTitle(), QString("Extract : ") + archive);
        QVERIFY(dialog.findChild<QCheckBox *>("extractElimDup")->isEnabled());
        const int pathIndex = pathMode->currentIndex();
        const int overwriteIndex = overwriteMode->currentIndex();
        const QString englishMode = pathMode->currentText();
        for (const char *lang : {"ru", "-"}) {
            language(lang);
            QCOMPARE(destination->currentText(), QString("/pending path"));
            QCOMPARE(destination->count(), 1);
            QCOMPARE(name->text(), QStringLiteral(" pending name 密碼 "));
            QCOMPARE(dialog.password(), QStringLiteral("密碼 pending secret"));
            QCOMPARE(pathMode->currentIndex(), pathIndex);
            QCOMPARE(overwriteMode->currentIndex(), overwriteIndex);
            if (QString(lang) == "ru") {
                QCOMPARE(dialog.windowTitle(), QStringLiteral("Извлечь : ") + archive);
                QVERIFY(pathMode->currentText() != englishMode);
                QCOMPARE(name->placeholderText(), QStringLiteral("Имя папки назначения"));
            } else {
                QCOMPARE(dialog.windowTitle(), QString("Extract : ") + archive);
                QCOMPARE(pathMode->currentText(), englishMode);
                QCOMPARE(name->placeholderText(), QString("Destination folder name"));
            }
            for (int width : {600, 470})
                inspect(dialog, "language-extract-" + QString(lang), width, 330);
        }
    }
    void progressPreservesPausedOperationAndFinishedClose() {
        LanguageProgressTransport transport;
        LanguageProgressDialog dialog(nullptr, L"operation 密碼");
        dialog.attach(transport);
        dialog.Sync.Set_NumBytesTotal(100);
        QCOMPARE(dialog.Sync.Set_NumBytesCur(25), S_OK);
        dialog.Sync.AddError_Message(L"diagnostic 密碼");
        dialog.show();
        auto *pause = dialog.findChild<QPushButton *>("IDB_PAUSE");
        auto *background = dialog.findChild<QPushButton *>("IDB_PROGRESS_BACKGROUND");
        auto *cancel = dialog.findChild<QPushButton *>("IDCANCEL");
        QTRY_VERIFY(dialog.windowTitle().startsWith("25% "));
        background->click();
        QVERIFY(transport.Background);
        pause->click();
        QVERIFY(transport.Paused);
        QVERIFY(dialog.Sync.Get_Paused());
        auto *list = dialog.findChild<QTreeWidget *>("IDL_PROGRESS_MESSAGES");
        const int rows = list->topLevelItemCount();
        QVERIFY(rows > 0);
        const QString englishPause = pause->text();
        for (const char *lang : {"ru", "-"}) {
            language(lang);
            QCOMPARE(transport.Requests, 2);
            QVERIFY(transport.Paused && transport.Background);
            QVERIFY(dialog.Sync.Get_Paused());
            QCOMPARE(list->topLevelItemCount(), rows);
            QCOMPARE(list->topLevelItem(0)->text(1), QString("diagnostic 密碼"));
            QVERIFY(dialog.windowTitle().endsWith("operation 密碼"));
            QVERIFY(dialog.windowTitle().contains("25%"));
            if (QString(lang) == "ru") {
                QVERIFY(pause->text() != englishPause);
                QVERIFY(dialog.windowTitle().startsWith(QStringLiteral("На паузе ")));
                QCOMPARE(background->text(), QStringLiteral("&На передний план"));
            } else {
                QCOMPARE(pause->text(), englishPause);
                QVERIFY(dialog.windowTitle().startsWith("Paused "));
                QCOMPARE(background->text(), QString("&Foreground"));
            }
            for (int width : {760, 640})
                inspect(dialog, "language-progress-" + QString(lang), width,
                        width == 760 ? 450 : 380);
        }
        pause->click();
        background->click();
        QVERIFY(!transport.Paused && !transport.Background);
        QVERIFY(!dialog.Sync.Get_Paused());
        dialog.ProcessWasFinished();
        QTRY_COMPARE(cancel->text(), QString("&Close"));
        QVERIFY(!background->isVisible());
        QVERIFY(!pause->isVisible());
        language("ru");
        QCOMPARE(cancel->text(), QStringLiteral("&Закрыть"));
        QVERIFY(!background->isVisible());
        QVERIFY(!pause->isVisible());
        QCOMPARE(list->topLevelItemCount(), rows);
        language("-");
        QCOMPARE(cancel->text(), QString("&Close"));
        cancel->click();
    }
};
QTEST_MAIN(LanguageDialogTests)
#include "language_dialog_tests.moc"
