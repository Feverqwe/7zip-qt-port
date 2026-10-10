#include "7zip/UI/FileManager/resource.h"
#include "CPP/7zip/UI/FileManager/LangUtils.h"
#include "createarchivedialog.h"
#include "layout_inspection.h"
#include "qtcompressoptionsdialog.h"
#include "qtfmoptionsdialog.h"
#include "qtlangcontrols.h"
#include "window_state_support.h"
#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QLabel>
#include <QListWidget>
#include <QScrollArea>
#include <QScrollBar>
#include <QStyleOptionButton>
#include <QTabWidget>
#include <QToolButton>
#include <QTreeWidget>

class LanguageUiTests : public QObject, protected MainWindowTestSupport {
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
    void toolbarTooltipsRetainSourceTextAfterActionChanges() {
        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        QFile file(directory.filePath(QStringLiteral("toolbar файл 空.txt")));
        QVERIFY(file.open(QIODevice::WriteOnly));
        QCOMPARE(file.write("protected toolbar bytes"), qint64(23));
        file.close();
        ControlledBackend backend;
        MainWindow window(&backend);
        window.show();
        auto *address = window.findChild<QLineEdit *>("archivePath");
        address->setText(directory.path());
        QTest::keyClick(address, Qt::Key_Return);
        auto *table = window.findChild<QTableWidget *>("archiveTable");
        QTRY_COMPARE(table->rowCount(), 1);
        auto *toolbar = window.findChild<QToolBar *>("archiveToolbar");
        QVERIFY(toolbar);
        const char *names[] = {"toolbarAddAction", "extractArchiveAction", "testArchiveAction",
                               "copyAction",       "moveAction",           "deleteAction",
                               "archiveInfoAction"};
        const UInt32 ids[] = {IDS_ADD,         IDS_EXTRACT,       IDS_TEST,       IDS_BUTTON_COPY,
                              IDS_BUTTON_MOVE, IDS_BUTTON_DELETE, IDS_BUTTON_INFO};
        for (const QString &language : {QString("-"), QString("ru"), QString("-")}) {
            SaveRegLang(ToUString(language), settingsPath());
            ReloadLang(settingsPath());
            QtNotifyLanguageChanged();
            for (bool selected : {false, true}) {
                table->clearSelection();
                table->setCurrentCell(-1, -1);
                if (selected) {
                    table->setCurrentCell(0, 0);
                    table->selectRow(0);
                }
                QApplication::processEvents();
                for (unsigned i = 0; i < 7; ++i) {
                    auto *action = window.findChild<QAction *>(names[i]);
                    QVERIFY(action);
                    auto *button = qobject_cast<QToolButton *>(toolbar->widgetForAction(action));
                    QVERIFY(button);
                    const QString expected = QString::fromWCharArray(LangString(ids[i]));
                    QCOMPARE(button->text(), expected);
                    QCOMPARE(button->toolTip(), expected);
                }
            }
            for (const QSize size : {QSize(980, 620), QSize(640, 400)}) {
                window.resize(size);
                QApplication::processEvents();
                QVERIFY(UiInspection::fullyVisible(*toolbar, window));
                QVERIFY(UiInspection::writeReport(
                    window, QString("toolbar-text-%1-%2").arg(language).arg(size.width())));
            }
        }
        QVERIFY(file.open(QIODevice::ReadOnly));
        QCOMPARE(file.readAll(), QByteArray("protected toolbar bytes"));
    }

    void init() {
        QFile::remove(settingsPath());
        SaveRegLang(UString("-"), settingsPath());
        ReloadLang(settingsPath());
    }
    void cleanup() {
        SaveRegLang(UString("-"), settingsPath());
        ReloadLang(settingsPath());
    }
    void addDialogRetainsPendingConfiguration() {
        QTemporaryDir directory;
        CreateArchiveDialog dialog(directory.path());
        auto *name = dialog.findChild<QLineEdit *>("archiveName");
        auto *password = dialog.findChild<QLineEdit *>("archivePassword");
        auto *parameters = dialog.findChild<QLineEdit *>("archiveParameters");
        auto *level = dialog.findChild<QComboBox *>("compressionLevel");
        auto *dictionary = dialog.findChild<QComboBox *>("compressionDictionary");
        auto *volume = dialog.findChild<QComboBox *>("archiveVolumeSize");
        QVERIFY(name && password && parameters && level && dictionary && volume);
        name->setText(QStringLiteral("pending archive 密碼.7z"));
        password->setText(QStringLiteral("pending secret 密碼"));
        volume->setEditText("10M 20M");
        level->setCurrentIndex(level->count() - 1);
        dictionary->setCurrentIndex(dictionary->count() - 1);
        parameters->setText("mt=2 tm=off");
        const QVariant levelData = level->currentData();
        const QVariant dictionaryData = dictionary->currentData();
        const QString englishLevel = level->currentText();
        for (const char *lang : {"ru", "-"}) {
            SaveRegLang(UString(lang), settingsPath());
            ReloadLang(settingsPath());
            QtNotifyLanguageChanged();
            QCOMPARE(name->text(), QStringLiteral("pending archive 密碼.7z"));
            QCOMPARE(password->text(), QStringLiteral("pending secret 密碼"));
            QCOMPARE(parameters->text(), QString("mt=2 tm=off"));
            QCOMPARE(volume->currentText(), QString("10M 20M"));
            QCOMPARE(level->currentData(), levelData);
            QCOMPARE(dictionary->currentData(), dictionaryData);
            if (QString(lang) == "ru") {
                QCOMPARE(dialog.windowTitle(), QStringLiteral("Добавить к архиву"));
                QVERIFY(level->currentText() != englishLevel);
            } else {
                QCOMPARE(dialog.windowTitle(), QString("Add to Archive"));
                QCOMPARE(level->currentText(), englishLevel);
            }
            for (QSize size : {QSize(760, 720), QSize(640, 480)}) {
                dialog.resize(size);
                dialog.show();
                QApplication::processEvents();
                auto *buttons = dialog.findChild<QDialogButtonBox *>();
                QVERIFY(UiInspection::fullyVisible(*buttons, dialog));
                auto *scroll = dialog.findChild<QScrollArea *>("archiveOptionsScrollArea");
                QVERIFY(scroll && scroll->widget());
                QCOMPARE(scroll->horizontalScrollBar()->maximum(), 0);
                for (QWidget *control : scroll->widget()->findChildren<QWidget *>()) {
                    if (control->objectName().isEmpty() ||
                        control->objectName().startsWith("qt_") || !control->isVisible())
                        continue;
                    if (!qobject_cast<QAbstractButton *>(control) &&
                        !qobject_cast<QComboBox *>(control) &&
                        !qobject_cast<QAbstractSpinBox *>(control) &&
                        !qobject_cast<QLineEdit *>(control) &&
                        !qobject_cast<QListWidget *>(control) && !qobject_cast<QLabel *>(control))
                        continue;
                    QApplication::processEvents();
                    // ensureWidgetVisible() uses the input-method cursor rect
                    // for editable controls. Geometry coverage needs the whole
                    // control, including a combo's frame and arrow.
                    const QPoint center =
                        control->mapTo(scroll->widget(), control->rect().center());
                    scroll->ensureVisible(center.x(), center.y(), control->width() / 2 + 5,
                                          control->height() / 2 + 30);
                    QApplication::processEvents();
                    if (!UiInspection::fullyVisible(*control, dialog))
                        QVERIFY(UiInspection::writeReport(dialog,
                                                          QString("language-add-clipped-%1-%2-%3")
                                                              .arg(lang)
                                                              .arg(size.width())
                                                              .arg(control->objectName())));
                    QVERIFY2(UiInspection::fullyVisible(*control, dialog),
                             qPrintable(control->objectName()));
                    if (qobject_cast<QLabel *>(control))
                        QVERIFY2(UiInspection::labelFits(*control),
                                 qPrintable(control->objectName()));
                    if (auto *button = qobject_cast<QAbstractButton *>(control)) {
                        if (button->objectName() == "archiveBrowse") {
                            // Source bxsDots is a narrow "..." button. The
                            // platform's footer-button sizeHint has an unrelated
                            // minimum width; check the actual caption bounds.
                            QStyleOptionButton option;
                            option.initFrom(button);
                            option.text = button->text();
                            const QRect contents = button->style()->subElementRect(
                                QStyle::SE_PushButtonContents, &option, button);
                            QVERIFY(button->fontMetrics().horizontalAdvance(button->text()) <=
                                    contents.width());
                        } else
                            QVERIFY2(control->sizeHint().width() <= control->width(),
                                     qPrintable(control->objectName()));
                    }
                    QVERIFY(UiInspection::fullyVisible(*buttons, dialog));
                }
                QVERIFY(UiInspection::writeReport(
                    dialog, QString("language-add-%1-%2").arg(lang).arg(size.width())));
                QVERIFY(!UiInspection::dump(dialog).contains("pending secret"));
            }
        }
    }
    void advancedDialogRetainsPendingTimePolicy() {
        NCompression::CFormatOptions profile;
        CCompressDialog source;
        source.Format = "zip";
        source.FormatOptions = &profile;
        source.ArcInfo.Name = L"zip";
        source.ArcInfo.Flags = NArcInfoFlags::kCTime | NArcInfoFlags::kATime |
                               NArcInfoFlags::kMTime | NArcInfoFlags::kMTime_Default;
        source.ArcInfo.TimeFlags = TIME_PREC_TO_ARC_FLAGS_MASK(NFileTimeType::kWindows) |
                                   TIME_PREC_TO_ARC_FLAGS_MASK(NFileTimeType::kUnix);
        COptionsDialog dialog(&source);
        dialog.show();
        auto *precision = dialog.findChild<QComboBox *>("IDC_COMPRESS_TIME_PREC");
        auto *explicitPrecision = dialog.findChild<QCheckBox *>("IDX_COMPRESS_PREC_SET");
        auto *explicitArchiveTime = dialog.findChild<QCheckBox *>("IDX_COMPRESS_ZTIME_SET");
        explicitPrecision->click();
        precision->setCurrentIndex(precision->findData(UInt32(NFileTimeType::kUnix)));
        QMetaObject::invokeMethod(precision, "activated", Qt::DirectConnection,
                                  Q_ARG(int, precision->currentIndex()));
        explicitArchiveTime->click();
        const QString englishPrecision = precision->currentText();
        const QString englishInfo = dialog.findChild<QLabel *>("IDT_COMPRESS_TIME_INFO")->text();
        for (const char *lang : {"ru", "-"}) {
            SaveRegLang(UString(lang), settingsPath());
            ReloadLang(settingsPath());
            QtNotifyLanguageChanged();
            QCOMPARE(precision->currentData().toUInt(), UInt32(NFileTimeType::kUnix));
            QVERIFY(explicitPrecision->isChecked());
            QVERIFY(explicitArchiveTime->isChecked());
            QVERIFY(!profile.IsSet_TimePrec());
            if (QString(lang) == "ru") {
                QVERIFY(precision->currentText() != englishPrecision);
                QVERIFY(dialog.findChild<QLabel *>("IDT_COMPRESS_TIME_INFO")->text() !=
                        englishInfo);
            } else {
                QCOMPARE(precision->currentText(), englishPrecision);
                QCOMPARE(dialog.findChild<QLabel *>("IDT_COMPRESS_TIME_INFO")->text(), englishInfo);
            }
            for (QSize size : {QSize(520, 400), QSize(440, 340)}) {
                dialog.resize(size);
                QApplication::processEvents();
                QVERIFY(UiInspection::writeReport(
                    dialog, QString("language-advanced-%1-%2").arg(lang).arg(size.width())));
                QCOMPARE(dialog.size(), size);
                for (auto *label : dialog.findChildren<QLabel *>())
                    if (label->isVisible()) {
                        QVERIFY(UiInspection::fullyVisible(*label, dialog));
                        QVERIFY(UiInspection::labelFits(*label));
                    }
                QVERIFY(UiInspection::writeReport(
                    dialog, QString("language-advanced-%1-%2").arg(lang).arg(size.width())));
            }
        }
    }
    void directoryTooltipsPreservePathsAcrossLanguageChange() {
        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        const QString child = directory.filePath(QStringLiteral("child 目录"));
        QVERIFY(QDir().mkpath(child));
        ControlledBackend backend;
        MainWindow window(&backend);
        window.show();
        auto *address = window.findChild<QLineEdit *>("archivePath");
        address->setText(directory.path());
        QTest::keyClick(address, Qt::Key_Return);
        auto *table = window.findChild<QTableWidget *>("archiveTable");
        QTRY_COMPARE(table->rowCount(), 1);
        auto *item = table->item(0, 0);
        QCOMPARE(table->horizontalHeaderItem(1)->text(), QString("Path"));
        QCOMPARE(table->horizontalHeaderItem(6)->text(), QString("Order"));
        const QString tooltip = item->toolTip();
        QCOMPARE(tooltip, QFileInfo(child).canonicalFilePath());
        table->selectRow(0);
        for (const char *lang : {"ru", "-"}) {
            SaveRegLang(UString(lang), settingsPath());
            ReloadLang(settingsPath());
            QtNotifyLanguageChanged();
            QCOMPARE(table->rowCount(), 1);
            QCOMPARE(table->item(0, 0), item);
            QCOMPARE(item->toolTip(), tooltip);
            QVERIFY(item->isSelected());
            QCOMPARE(address->text(), QFileInfo(directory.path()).canonicalFilePath());
            QCOMPARE(address->placeholderText(), QString(lang) == "ru"
                                                     ? QStringLiteral("Путь к папке или архиву")
                                                     : QString("Path to folder or archive"));
        }
    }
    void propertiesPreserveRowsValuesAndSelection() {
        QTemporaryDir directory;
        QFile file(directory.filePath(QStringLiteral("property 密碼 file.txt")));
        QVERIFY(file.open(QIODevice::WriteOnly));
        QCOMPARE(file.write("fixture"), qint64(7));
        file.close();
        ControlledBackend backend;
        MainWindow window(&backend);
        window.show();
        auto *address = window.findChild<QLineEdit *>("archivePath");
        address->setText(directory.path());
        QTest::keyClick(address, Qt::Key_Return);
        auto *table = window.findChild<QTableWidget *>("archiveTable");
        QTRY_COMPARE(table->rowCount(), 1);
#ifdef Q_OS_WIN
        // Windows filesystem Properties belongs to the OS. Qt language changes
        // apply to the source archive collector, whose rows this test inspects.
        openListedArchive(window, backend, "language-properties.zip",
                          {{QFileInfo(file.fileName()).fileName(), 7, 7}});
#endif
        table->selectRow(0);
        bool inspected = false;
        QTimer::singleShot(0, &window, [&] {
            auto *dialog = window.findChild<QDialog *>("selectedPropertiesDialog");
            QVERIFY(dialog);
            auto *list = dialog->findChild<QTreeWidget *>("propertiesList");
            QVERIFY(list && list->topLevelItemCount() > 0);
            const int rows = list->topLevelItemCount();
            QStringList english, values;
            for (int i = 0; i < rows; ++i) {
                english << list->topLevelItem(i)->text(0);
                values << list->topLevelItem(i)->text(1);
            }
            list->setCurrentItem(list->topLevelItem(0));
            list->topLevelItem(0)->setSelected(true);
            auto *selected = list->currentItem();
            for (const char *lang : {"ru", "-"}) {
                SaveRegLang(UString(lang), settingsPath());
                ReloadLang(settingsPath());
                QtNotifyLanguageChanged();
                QCOMPARE(list->topLevelItemCount(), rows);
                QCOMPARE(list->currentItem(), selected);
                QVERIFY(selected->isSelected());
                for (int i = 0; i < rows; ++i)
                    QCOMPARE(list->topLevelItem(i)->text(1), values[i]);
                if (QString(lang) == "ru") {
                    QCOMPARE(dialog->windowTitle(), QStringLiteral("Свойства"));
                    QCOMPARE(list->topLevelItem(0)->text(0), QStringLiteral("Имя"));
                } else {
                    QCOMPARE(dialog->windowTitle(), QString("Properties"));
                    for (int i = 0; i < rows; ++i)
                        QCOMPARE(list->topLevelItem(i)->text(0), english[i]);
                }
                for (QSize size : {QSize(540, 360), QSize(360, 240)}) {
                    dialog->resize(size);
                    QApplication::processEvents();
                    QVERIFY(UiInspection::fullyVisible(*list, *dialog));
                    QVERIFY(UiInspection::writeReport(
                        *dialog, QString("language-properties-%1-%2").arg(lang).arg(size.width())));
                }
            }
            inspected = true;
            dialog->reject();
        });
        window.findChild<QAction *>("selectedPropertiesAction")->trigger();
        QVERIFY(inspected);
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
        QCOMPARE(address->placeholderText(), QStringLiteral("Путь к папке или архиву"));
        QCOMPARE(address->text(), QFileInfo(directory.path()).canonicalFilePath());
        QCOMPARE(table->accessibleName(), QStringLiteral("Список файлов"));
        QCOMPARE(window.findChild<QListView *>("iconList")->accessibleName(),
                 QStringLiteral("Список файлов"));
        auto *toolbar = window.findChild<QToolBar *>("archiveToolbar");
        auto inspectWindow = [&](const QString &language) {
            for (QSize size : {QSize(1100, 640), QSize(640, 380)}) {
                window.resize(size);
                QApplication::processEvents();
                QVERIFY(UiInspection::fullyVisible(*address, window));
                QVERIFY(UiInspection::fullyVisible(*table, window));
                QVERIFY(UiInspection::fullyVisible(*toolbar, window));
                QVERIFY(UiInspection::writeReport(
                    window, QString("language-main-%1-%2").arg(language).arg(size.width())));
            }
        };
        inspectWindow("ru");
        QCOMPARE(toolbar->windowTitle(), QStringLiteral("Архив"));
        QCOMPARE(window.findChild<QAction *>("backAction")->text(), QStringLiteral("Назад"));
        QCOMPARE(window.findChild<QAction *>("openItemAction")->text(), QStringLiteral("&Открыть"));
        QCOMPARE(window.findChild<QAction *>("editFileAction")->text(),
                 QStringLiteral("&Редактировать"));
        QVERIFY(!window.findChild<QAction *>("operationJournalAction"));
        auto *copy = window.findChild<QAction *>("copyAction");
        auto *button = qobject_cast<QToolButton *>(toolbar->widgetForAction(copy));
        QVERIFY(button);
        QCOMPARE(button->text(), QStringLiteral("Копировать"));
        QtFmOptionsDialog restarted(settingsPath());
        QCOMPARE(restarted.findChild<QComboBox *>("languageChoice")->currentIndex(), 1);
        QCOMPARE(restarted.windowTitle(), QStringLiteral("Настройки"));
        MainWindow restartedWindow(&backend);
        QCOMPARE(restartedWindow.findChild<QMenu *>("fileMenu")->title(), QStringLiteral("&Файл"));
        chooseLanguage(restarted, 0);
        apply(restarted);
        QCOMPARE(fileMenu->title(), QString("&File"));
        QCOMPARE(toolbar->windowTitle(), QString("Archive"));
        QCOMPARE(window.findChild<QAction *>("backAction")->text(), QString("Back"));
        QCOMPARE(window.findChild<QAction *>("openItemAction")->text(), QString("&Open"));
        QCOMPARE(window.findChild<QAction *>("editFileAction")->text(), QString("&Edit"));
        QVERIFY(!window.findChild<QAction *>("operationJournalAction"));
        QVERIFY(!parentButton->icon().isNull());
        QCOMPARE(button->text(), QString("Copy"));
        QCOMPARE(table->horizontalHeaderItem(0)->text(), QString("Name"));
        QCOMPARE(address->placeholderText(), QString("Path to folder or archive"));
        QCOMPARE(table->accessibleName(), QString("File list"));
        QCOMPARE(window.findChild<QListView *>("iconList")->accessibleName(), QString("File list"));
        inspectWindow("en");
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
            QVERIFY(items->isEnabled());
            QVERIFY(items->toolTip().isEmpty());
#ifndef _WIN32
            QCOMPARE(dialog.findChild<QCheckBox *>("workRemovableOnly")->toolTip(),
                     QString(language) == "ru"
                         ? QStringLiteral("Определение сменных носителей доступно в Windows.")
                         : QString("Removable-drive detection is available on Windows."));
#endif
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
