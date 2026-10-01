#include "CPP/7zip/UI/FileManager/LangUtils.h"
#include "layout_inspection.h"
#include "qtfmoptionsdialog.h"
#include "qtlangcontrols.h"
#include "window_state_support.h"
#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QLabel>
#include <QListWidget>
#include <QTabWidget>
#include <QToolButton>

class LanguageUiTests : public QObject {
    Q_OBJECT
    QString settingsPath() const { return qEnvironmentVariable("SEVENZIP_SETTINGS_PATH"); }
    void chooseLanguage(QDialog &dialog, int index) {
        auto *choice = dialog.findChild<QComboBox *>("languageChoice");
        QVERIFY(choice);
        choice->setCurrentIndex(index);
    }
    void apply(QDialog &dialog) {
        auto *buttons = dialog.findChild<QDialogButtonBox *>("settingsButtons");
        QVERIFY(buttons && buttons->button(QDialogButtonBox::Apply)->isEnabled());
        buttons->button(QDialogButtonBox::Apply)->click();
    }

  private slots:
    void init() {
        QFile::remove(settingsPath());
        SaveRegLang(UString("-"), settingsPath());
        ReloadLang(settingsPath());
    }
    void cleanup() {
        SaveRegLang(UString("-"), settingsPath());
        ReloadLang(settingsPath());
    }
    void applyCancelRestartAndMainWindowState() {
        QTemporaryDir directory;
        QFile file(directory.filePath(QStringLiteral("имя с пробелом.txt")));
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.close();
        ControlledBackend backend;
        MainWindow window(&backend);
        window.show();
        auto *address = window.findChild<QLineEdit *>("archivePath");
        address->setText(directory.path());
        QTest::keyClick(address, Qt::Key_Return);
        auto *table = window.findChild<QTableWidget *>("archiveTable");
        QTRY_COMPARE(table->rowCount(), 1);
        table->selectRow(0);
        table->setCurrentCell(0, 0);
        auto *rename = window.findChild<QAction *>("renameAction");
        const QKeySequence shortcut = rename->shortcut();
        const bool enabled = rename->isEnabled();
        const QStringList fileNames = {table->item(0, 0)->text()};
        auto *fileMenu = window.findChild<QMenu *>("fileMenu");
        auto *parentButton = window.findChild<QToolButton *>("panelParentButton0");
        QVERIFY(parentButton && !parentButton->icon().isNull());
        QCOMPARE(parentButton->toolButtonStyle(), Qt::ToolButtonIconOnly);
        QCOMPARE(fileMenu->title(), QString("&File"));
        bool applied = false;
        QTimer::singleShot(0, &window, [&] {
            auto *dialog = window.findChild<QDialog *>("settingsDialog");
            QVERIFY(dialog);
            chooseLanguage(*dialog, 1);
            apply(*dialog);
            QCOMPARE(dialog->windowTitle(), QStringLiteral("Настройки"));
            QCOMPARE(fileMenu->title(), QStringLiteral("&Файл"));
            QVERIFY(!parentButton->icon().isNull());
            auto *choice = dialog->findChild<QComboBox *>("languageChoice");
            QCOMPARE(choice->currentIndex(), 1);
            chooseLanguage(*dialog, 0);
            dialog->reject();
            applied = true;
        });
        window.findChild<QAction *>("settingsAction")->trigger();
        QVERIFY(applied);
        UString saved;
        ReadRegLang(saved, settingsPath());
        QCOMPARE(QString::fromWCharArray(saved), QString("ru"));
        QCOMPARE(window.findChild<QAction *>("renameAction"), rename);
        QCOMPARE(rename->shortcut(), shortcut);
        QCOMPARE(rename->isEnabled(), enabled);
        QTRY_COMPARE(table->rowCount(), 1);
        QCOMPARE(table->item(0, 0)->text(), fileNames.first());
        QCOMPARE(table->currentRow(), 0);
        QCOMPARE(table->selectionModel()->selectedRows().size(), 1);
        QCOMPARE(table->horizontalHeaderItem(0)->text(), QStringLiteral("Имя"));
        auto *toolbar = window.findChild<QToolBar *>("archiveToolbar");
        auto *copy = window.findChild<QAction *>("copyAction");
        auto *button = qobject_cast<QToolButton *>(toolbar->widgetForAction(copy));
        QVERIFY(button);
        QCOMPARE(button->text(), QStringLiteral("Копировать в"));
        QtFmOptionsDialog restarted(settingsPath());
        QCOMPARE(restarted.findChild<QComboBox *>("languageChoice")->currentIndex(), 1);
        QCOMPARE(restarted.windowTitle(), QStringLiteral("Настройки"));
        MainWindow restartedWindow(&backend);
        QCOMPARE(restartedWindow.findChild<QMenu *>("fileMenu")->title(), QStringLiteral("&Файл"));
        chooseLanguage(restarted, 0);
        apply(restarted);
        QCOMPARE(fileMenu->title(), QString("&File"));
        QVERIFY(!parentButton->icon().isNull());
        QCOMPARE(button->text(), QString("Copy"));
        QCOMPARE(table->horizontalHeaderItem(0)->text(), QString("Name"));
    }
    void retranslationPreservesPendingInputAndGeometry() {
        QtFmOptionsDialog dialog(settingsPath());
        dialog.show();
        auto *pages = dialog.findChild<QTabWidget *>("optionsPages");
        auto *editor = dialog.findChild<QLineEdit *>("editorCommand");
        auto *items = dialog.findChild<QListWidget *>("menuItems");
        auto *grid = dialog.findChild<QCheckBox *>("gridOption");
        const QString command = QStringLiteral("\"/tmp/редактор с пробелом\" --wait");
        editor->setText(command);
        grid->setChecked(!grid->isChecked());
        const bool checked = grid->isChecked();
        const Qt::CheckState itemState = items->item(0)->checkState();
        for (const char *language : {"ru", "-"}) {
            SaveRegLang(UString(language), settingsPath());
            ReloadLang(settingsPath());
            QtNotifyLanguageChanged();
            QCOMPARE(editor->text(), command);
            QCOMPARE(grid->isChecked(), checked);
            QCOMPARE(items->item(0)->checkState(), itemState);
            QVERIFY(dialog.findChild<QDialogButtonBox *>("settingsButtons")
                        ->button(QDialogButtonBox::Apply)
                        ->isEnabled());
            for (QSize size : {QSize(640, 390), QSize(520, 380)}) {
                dialog.resize(size);
                for (int page = 1; page < pages->count(); page++) {
                    pages->setCurrentIndex(page);
                    QApplication::processEvents();
                    for (auto *control : pages->currentWidget()->findChildren<QWidget *>()) {
                        if (!control->isVisible() ||
                            control->parentWidget() != pages->currentWidget())
                            continue;
                        QVERIFY2(UiInspection::fullyVisible(*control, dialog),
                                 qPrintable(control->objectName()));
                        if (qobject_cast<QCheckBox *>(control))
                            QVERIFY2(control->sizeHint().width() <= control->width(),
                                     qPrintable(control->objectName()));
                        if (qobject_cast<QLabel *>(control))
                            QVERIFY2(UiInspection::labelFits(*control),
                                     qPrintable(control->objectName()));
                    }
                    QVERIFY(UiInspection::writeReport(dialog, QString("options-language-%1-%2-%3")
                                                                  .arg(language)
                                                                  .arg(size.width())
                                                                  .arg(page)));
                }
            }
        }
        dialog.reject();
        UString savedEditor;
        ReadRegEditor(true, savedEditor, settingsPath());
        QVERIFY(savedEditor.IsEmpty());
    }
    void invalidSettingsKeepLanguagePending() {
        QtFmOptionsDialog dialog(settingsPath());
        dialog.show();
        chooseLanguage(dialog, 1);
        dialog.findChild<QCheckBox *>("extractMemorySet")->setChecked(true);
        auto *memory = dialog.findChild<QLineEdit *>("extractMemoryEdit");
        memory->setText("3GB");
        bool errorShown = false;
        QTimer::singleShot(0, &dialog, [&] {
            auto *error = qobject_cast<QMessageBox *>(QApplication::activeModalWidget());
            QVERIFY(error);
            errorShown = true;
            error->accept();
        });
        apply(dialog);
        QVERIFY(errorShown);
        QCOMPARE(dialog.windowTitle(), QString("Options"));
        QCOMPARE(dialog.findChild<QComboBox *>("languageChoice")->currentIndex(), 1);
        QCOMPARE(memory->text(), QString("3GB"));
        UString saved;
        ReadRegLang(saved, settingsPath());
        QCOMPARE(QString::fromWCharArray(saved), QString("-"));
        memory->setText("2");
        apply(dialog);
        QCOMPARE(dialog.windowTitle(), QStringLiteral("Настройки"));
        ReadRegLang(saved, settingsPath());
        QCOMPARE(QString::fromWCharArray(saved), QString("ru"));
    }
    void partialLanguageResetsMenusAndKeepsSpecialItems() {
        ControlledBackend backend;
        MainWindow window(&backend);
        auto *openInside = window.findChild<QAction *>("openInsideAction");
        openInside->setText(openInside->text() + "\tCtrl+PgDown");
        auto *timestamp = window.findChild<QMenu *>("timestampMenu");
        const QString timestampTitle = timestamp->title();
        auto *hash = window.findChild<QMenu *>("hashMenu");
        const QString hashTitle = hash->title();
        auto *favorite = window.findChild<QMenu *>("addFavoriteMenu");
        const QString favoriteSlot = favorite->actions().first()->text();
        const QString originalRename = window.findChild<QAction *>("renameAction")->text();
        QTemporaryDir directory;
        QFile file(directory.filePath("partial.txt"));
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.write(";!@Lang2@!UTF-8!\n0\n7-Zip\nPartial\nPartial\n541\n&Inside\n");
        file.close();
        SaveRegLang(UString("partial"), settingsPath());
        ReloadLang(settingsPath(), directory.path());
        QtNotifyLanguageChanged();
        QCOMPARE(openInside->text(), QString("&Inside\tCtrl+PgDown"));
        QCOMPARE(window.findChild<QAction *>("openInsideOneAction")->text(), QString("Inside *"));
        QCOMPARE(window.findChild<QAction *>("openInsideParserAction")->text(),
                 QString("Inside #"));
        QCOMPARE(window.findChild<QAction *>("renameAction")->text(), originalRename);
        QCOMPARE(timestamp->title(), timestampTitle);
        QCOMPARE(hash->title(), hashTitle);
        QCOMPARE(favorite->actions().first()->text(), favoriteSlot);
        SaveRegLang(UString("ru"), settingsPath());
        ReloadLang(settingsPath());
        QtNotifyLanguageChanged();
        QVERIFY(window.findChild<QAction *>("renameAction")->text() != originalRename);
        SaveRegLang(UString("missing"), settingsPath());
        ReloadLang(settingsPath(), directory.path());
        QtNotifyLanguageChanged();
        QCOMPARE(window.findChild<QAction *>("renameAction")->text(), originalRename);
    }
};
QTEST_MAIN(LanguageUiTests)
#include "language_ui_tests.moc"
