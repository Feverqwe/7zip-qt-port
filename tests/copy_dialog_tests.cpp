#include "CPP/7zip/UI/FileManager/CopyDialog.h"
#include "CPP/7zip/UI/FileManager/LangUtils.h"
#include "CPP/7zip/UI/FileManager/Panel.h"
#include "CPP/7zip/UI/FileManager/RegistryUtils.h"
#include "CPP/7zip/UI/FileManager/ViewSettings.h"
#include "layout_inspection.h"
#include "qtdialogunits.h"

#include <QComboBox>
#include <QDialogButtonBox>
#include <QDir>
#include <QFileDialog>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QScopeGuard>
#include <QScreen>
#include <QSet>
#include <QSettings>
#include <QTemporaryDir>
#include <QTest>
#include <QTimer>
#include <initializer_list>

class CopyDialogTests : public QObject {
    Q_OBJECT

    static UStringVector SourceStrings(std::initializer_list<const wchar_t *> values) {
        UStringVector strings;
        for (const wchar_t *value : values)
            strings.Add(value);
        return strings;
    }
    static QStringList QtStrings(const UStringVector &values) {
        QStringList strings;
        FOR_VECTOR(i, values)
        strings.append(ToQString(values[i]));
        return strings;
    }
    static void inspectSourceFrame(CCopyDialog &dialog, const QString &tag) {
        auto *path = dialog.findChild<QComboBox *>("copyDestination");
        auto *browse = dialog.findChild<QPushButton *>("copyBrowseButton");
        auto *info = dialog.findChild<QPlainTextEdit *>("copySelectionInfo");
        auto *buttons = dialog.findChild<QDialogButtonBox *>("copyButtons");
        auto *label = dialog.findChild<QLabel *>("copyDestinationLabel");
        QVERIFY(path && browse && info && buttons && label);
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
        QVERIFY(UiInspection::writeReport(dialog, tag));
    }

  private slots:
    void initTestCase() { QApplication::setAttribute(Qt::AA_DontUseNativeDialogs); }

    void sourceFrameAndKeyboard_data() {
        QTest::addColumn<bool>("move");
        QTest::addColumn<bool>("russian");
        QTest::addColumn<int>("points");
        for (bool move : {false, true})
            for (bool russian : {false, true})
                for (int points : {9, 12, 32})
                    QTest::newRow(
                        qPrintable(QString("%1-%2-%3").arg(move).arg(russian).arg(points)))
                        << move << russian << points;
    }
    void sourceFrameAndKeyboard() {
        QFETCH(bool, move);
        QFETCH(bool, russian);
        QFETCH(int, points);
        QTemporaryDir fixture;
        QVERIFY(fixture.isValid());
        const QFont original = QApplication::font();
        const auto restore = qScopeGuard([&] {
            QApplication::setFont(original);
            ReloadLang(qEnvironmentVariable("SEVENZIP_SETTINGS_PATH"));
        });
        QFont font = original;
        font.setPointSize(points);
        QApplication::setFont(font);
        const QString settings = fixture.filePath("language.ini");
        SaveRegLang(UString(russian ? L"ru" : L"-"), settings);
        ReloadLang(settings);
        const QString initial = QStringLiteral("/destination space 空/");
        const QString summary = QStringLiteral("/source/\nимя с пробелом 空.txt");
        CCopyDialog dialog(move, initial, {}, {"/one", "/two"}, nullptr, summary);
        QVERIFY(dialog.windowFlags().testFlag(Qt::WindowMinimizeButtonHint));
        QVERIFY(dialog.windowFlags().testFlag(Qt::WindowMaximizeButtonHint));
        QVERIFY(!dialog.windowFlags().testFlag(Qt::WindowContextHelpButtonHint));
        dialog.show();
        QVERIFY(QTest::qWaitForWindowActive(&dialog));
        const QSize initialSize = QtDialogUnits(dialog.font())
                                      .pixels(QSize(336, 160))
                                      .expandedTo(dialog.minimumSizeHint());
        const QSize frame = dialog.frameGeometry().size() - dialog.size();
        const QSize work = dialog.screen()->availableGeometry().size();
        if (initialSize.width() + frame.width() > work.width() ||
            initialSize.height() + frame.height() > work.height()) {
            // Original NormalizeSize(true), or a platform-enforced work-area fit.
            QTRY_VERIFY(dialog.width() <= work.width());
            QTRY_VERIFY(dialog.height() <= work.height());
        } else
            QCOMPARE(dialog.size(), initialSize);
        auto *path = dialog.findChild<QComboBox *>("copyDestination");
        auto *browse = dialog.findChild<QPushButton *>("copyBrowseButton");
        auto *info = dialog.findChild<QPlainTextEdit *>("copySelectionInfo");
        auto *buttons = dialog.findChild<QDialogButtonBox *>("copyButtons");
        QVERIFY(path && browse && info && buttons);
        QVERIFY(path->hasFocus() || path->lineEdit()->hasFocus());
        QCOMPARE(path->lineEdit()->selectedText(), initial);
        QCOMPARE(info->focusPolicy(), Qt::NoFocus); // Original static has no WS_TABSTOP.
        QCOMPARE(info->toPlainText(), summary);
        const QString tag = QString("copy-source-frame-%1").arg(QTest::currentDataTag());
        dialog.setProperty("uiInspection_language", russian ? "ru" : "en");
        dialog.setProperty("uiInspection_state", move ? "move-initial" : "copy-initial");
        inspectSourceFrame(dialog, tag + "-initial");
        dialog.showNormal();
        dialog.resize(340, 220);
        QApplication::processEvents();
        dialog.setProperty("uiInspection_state", move ? "move-compact" : "copy-compact");
        inspectSourceFrame(dialog, tag + "-compact");
        // GuiCommon.rc bxsDots: preserve the source utility-button width,
        // independently of the platform's standard push-button minimum.
        QCOMPARE(browse->width(), QtDialogUnits(dialog.font()).pixels(QSize(20, 0)).width());
        const QSet<QWidget *> allowed{path, path->lineEdit(), browse,
                                      buttons->button(QDialogButtonBox::Ok),
                                      buttons->button(QDialogButtonBox::Cancel)};
        for (int i = 0; i < 5; ++i) {
            QTest::keyClick(dialog.focusWidget(), Qt::Key_Tab);
            QVERIFY(allowed.contains(dialog.focusWidget()));
        }
        path->lineEdit()->setFocus();
        const QString changed = QStringLiteral("  /changed space 空/  ");
        path->setCurrentText(changed);
        QTest::keyClick(path->lineEdit(), Qt::Key_Return);
        QCOMPARE(dialog.result(), int(QDialog::Accepted));
        QCOMPARE(ToQString(dialog.Value), changed);
        CCopyDialog cancelled(move, initial, {}, {}, nullptr, summary);
        cancelled.show();
        QVERIFY(QTest::qWaitForWindowActive(&cancelled));
        cancelled.findChild<QComboBox *>("copyDestination")->setCurrentText(changed);
        QTest::keyClick(cancelled.focusWidget(), Qt::Key_Escape);
        QCOMPARE(cancelled.result(), int(QDialog::Rejected));
        QCOMPARE(ToQString(cancelled.Value), initial);
    }

    void copiesFieldsAndCommitsOnlyOnOK() {
        CCopyDialog dialog(false, QStringLiteral("/начальная папка/"),
                           {QStringLiteral("/tmp/имя с пробелом.txt")}, {"/one", "/two"});
        QCOMPARE(dialog.windowTitle(), QString("Copy"));
        auto *path = dialog.findChild<QComboBox *>("copyDestination");
        QVERIFY(path);
        QCOMPARE(path->count(), 2);
        QCOMPARE(path->itemText(1), QString("/two"));
        auto *info = dialog.findChild<QPlainTextEdit *>("copySelectionInfo");
        QVERIFY(info && info->isReadOnly());
        QCOMPARE(info->toPlainText(), QStringLiteral("имя с пробелом.txt"));
        path->setCurrentText(QStringLiteral("  /новая папка/  "));
        dialog.reject();
        QCOMPARE(ToQString(dialog.Value), QStringLiteral("/начальная папка/"));
        dialog.accept();
        QCOMPARE(ToQString(dialog.Value), QStringLiteral("  /новая папка/  "));
        CCopyDialog move(true, "/initial", {}, {});
        QCOMPARE(move.windowTitle(), QString("Move"));
        QCOMPARE(move.findChild<QLabel *>("copyDestinationLabel")->text(), QString("Move to:"));
        move.findChild<QComboBox *>("copyDestination")->setCurrentText("/by-button");
        move.findChild<QDialogButtonBox *>("copyButtons")->button(QDialogButtonBox::Ok)->click();
        QCOMPARE(ToQString(move.Value), QString("/by-button"));
        QCOMPARE(move.result(), int(QDialog::Accepted));
    }

    void historyRemovesAllCaseVariantsAndIgnoresMvpINI() {
        QTemporaryDir temp;
        QVERIFY(temp.isValid());
        const QString ini = temp.filePath(QStringLiteral("настройки с пробелом.ini"));
        {
            QSettings settings(ini, QSettings::IniFormat);
            settings.setValue("copy/history", QStringList{"/old", "/OLDER"});
            settings.setValue("unrelated/value", 17);
        }
        UStringVector history = SourceStrings({L"stale"});
        ReadCopyHistory(history, ini);
        QVERIFY(history.IsEmpty());
        SaveCopyHistory(SourceStrings({L"/old", L"/OLDER"}), ini);
        ReadCopyHistory(history, ini);
        QCOMPARE(QtStrings(history), (QStringList{"/old", "/OLDER"}));
        history.Add(L"/NEW");
        history.Add(L"/new");
        history.Add(L"/NeW");
        AddUniqueStringToHeadOfList(history, L"/new");
        QCOMPARE(QtStrings(history), (QStringList{"/new", "/old", "/OLDER"}));
        UStringVector unicode = SourceStrings({L"/ПАПКА", L"/keep", L"/папка"});
        AddUniqueStringToHeadOfList(unicode, L"/Папка");
        QCOMPARE(QtStrings(unicode), (QStringList{QStringLiteral("/Папка"), "/keep"}));
        UStringVector sourceCase = SourceStrings({L"/\u212A", L"/k", L"/K", L"/keep"});
        AddUniqueStringToHeadOfList(sourceCase, L"/K");
        QCOMPARE(QtStrings(sourceCase), (QStringList{"/K", QStringLiteral("/\u212A"), "/keep"}));
        SaveCopyHistory(sourceCase, ini);
        UStringVector restarted;
        ReadCopyHistory(restarted, ini);
        QCOMPARE(QtStrings(restarted), QtStrings(sourceCase));
        CCopyDialog sourceHistory(false, "/destination", {}, QtStrings(restarted));
        auto *destinations = sourceHistory.findChild<QComboBox *>("copyDestination");
        QCOMPARE(destinations->count(), 3);
        QCOMPARE(destinations->itemText(1), QStringLiteral("/\u212A"));
        SaveCopyHistory(history, ini);
        history.Clear();
        ReadCopyHistory(history, ini);
        QCOMPARE(QtStrings(history), (QStringList{"/new", "/old", "/OLDER"}));
        QSettings settings(ini, QSettings::IniFormat);
        QCOMPARE(settings.value("unrelated/value").toInt(), 17);
        QCOMPARE(settings.value("copy/history").toStringList(), (QStringList{"/old", "/OLDER"}));
        SaveCopyHistory({}, ini);
        ReadCopyHistory(history, ini);
        QVERIFY(history.IsEmpty());
        ReadCopyHistory(history, temp.filePath("missing.ini"));
        QVERIFY(history.IsEmpty());
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
        QCOMPARE(QDir::fromNativeSeparators(path->currentText()), chosen + '/');
        dialog.accept();
        QCOMPARE(QDir::fromNativeSeparators(ToQString(dialog.Value)), chosen + '/');
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
