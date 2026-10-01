#include "CPP/7zip/UI/FileManager/EditPage.h"
#include "CPP/7zip/UI/FileManager/RegistryUtils.h"
#include "layout_inspection.h"
#include "qtfmoptionsdialog.h"
#include "window_state_support.h"
#include <QFileDialog>
#include <QJsonArray>
#include <QJsonDocument>
#include <QTabWidget>

namespace {
bool writeFile(const QString &path, const QByteArray &bytes = "contents") {
    QFile file(path);
    return file.open(QIODevice::WriteOnly) && file.write(bytes) == bytes.size();
}
QString command(const QString &output) {
    return QString("\"%1\" --external-fixture \"%2\"")
        .arg(QCoreApplication::applicationFilePath(), output);
}
QStringList readArguments(const QString &path) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly))
        return {};
    QStringList result;
    for (const auto &value : QJsonDocument::fromJson(file.readAll()).array())
        result.append(value.toString().normalized(QString::NormalizationForm_C));
    return result;
}
void navigate(MainWindow &window, const QString &path) {
    auto *address = window.findChild<QLineEdit *>("archivePath");
    address->setText(path);
    QTest::keyClick(address, Qt::Key_Return);
}
} // namespace

class ExternalEditorsUiTests final : public QObject, protected MainWindowTestSupport {
    Q_OBJECT
  private slots:
    void init() { QFile::remove(qEnvironmentVariable("SEVENZIP_SETTINGS_PATH")); }

    void editorPageApplyCancelAndGeometry() {
        const QString settings = qEnvironmentVariable("SEVENZIP_SETTINGS_PATH");
        SaveRegEditor(false, UString(L"viewer --old"), settings);
        for (bool apply : {false, true}) {
            QtFmOptionsDialog dialog(settings);
            auto *applyButton =
                dialog.findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Apply);
            QVERIFY(!applyButton->isEnabled());
            auto *pages = dialog.findChild<QTabWidget *>("optionsPages");
            QCOMPARE(pages->count(), 6);
            QCOMPARE(pages->tabText(3), QString("Редактор"));
            QCOMPARE(pages->widget(3)->property("sourcePageID").toUInt(), 2103u);
            QVERIFY(pages->isTabEnabled(2) && pages->isTabEnabled(3) && pages->isTabEnabled(4));
            QVERIFY(pages->isTabEnabled(1));
            for (int index : {0, 5})
                QVERIFY(!pages->isTabEnabled(index));
            pages->setCurrentIndex(3);
            dialog.findChild<QLineEdit *>("viewerCommand")->setText("new viewer");
            dialog.findChild<QLineEdit *>("editorCommand")->setText("editor --edit");
            dialog.findChild<QLineEdit *>("diffCommand")->setText("diff --compare");
            QVERIFY(applyButton->isEnabled());
            dialog.show();
            for (QSize size : {QSize(640, 390), QSize(520, 380)}) {
                dialog.resize(size);
                QApplication::processEvents();
                QCOMPARE(dialog.size(), size);
                for (const char *name : {"viewerCommand", "editorCommand", "diffCommand"}) {
                    auto *edit = dialog.findChild<QLineEdit *>(name);
                    auto *browse = dialog.findChild<QPushButton *>(QString(name) + "Browse");
                    QVERIFY(UiInspection::fullyVisible(*edit, dialog));
                    QVERIFY(UiInspection::fullyVisible(*browse, dialog));
                    QVERIFY(UiInspection::separate(*edit, *browse, dialog));
                }
                QVERIFY(
                    UiInspection::fullyVisible(*dialog.findChild<QDialogButtonBox *>(), dialog));
                QVERIFY(UiInspection::writeReport(
                    dialog, QString("external-editor-options-%1").arg(size.width())));
            }
            if (apply) {
                dialog.findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Apply)->click();
                QVERIFY(!applyButton->isEnabled());
                QVERIFY(dialog.isVisible());
                dialog.findChild<QLineEdit *>("viewerCommand")->setText("discarded after Apply");
            }
            dialog.reject();
            UString value;
            ReadRegEditor(false, value, settings);
            QCOMPARE(ToQString(value), apply ? QString("new viewer") : QString("viewer --old"));
            ReadRegDiff(value, settings);
            QCOMPARE(ToQString(value), apply ? QString("diff --compare") : QString());
        }
    }

    void editorBrowsePreservesCancelAndReplacesParameters() {
        QTemporaryDir fixture;
        const QString selected = fixture.filePath(QStringLiteral("программа с пробелом"));
        QVERIFY(writeFile(selected));
        CEditPage page(qEnvironmentVariable("SEVENZIP_SETTINGS_PATH"));
        auto *edit = page.findChild<QLineEdit *>("editorCommand");
        auto *browse = page.findChild<QPushButton *>("editorCommandBrowse");
        const QString original = "\"" + selected + "\" --old-parameter";
        edit->setText(original);
        QTimer::singleShot(0, [] {
            if (auto *dialog = qobject_cast<QFileDialog *>(QApplication::activeModalWidget()))
                dialog->reject();
        });
        browse->click();
        QCOMPARE(edit->text(), original);
        QTimer::singleShot(0, [&] {
            if (auto *dialog = qobject_cast<QFileDialog *>(QApplication::activeModalWidget())) {
                dialog->selectFile(selected);
                QMetaObject::invokeMethod(dialog, "accept", Qt::DirectConnection);
            }
        });
        browse->click();
        QCOMPARE(edit->text(), selected);
        page.OnApply();
        UString saved;
        ReadRegEditor(true, saved, qEnvironmentVariable("SEVENZIP_SETTINGS_PATH"));
        QCOMPARE(ToQString(saved), selected);
    }

    void keyboardDiskEditorsUseFocusedFileAndIndependentSettings() {
        QTemporaryDir fixture;
        const QString first = fixture.filePath(QStringLiteral("первый файл.txt"));
        const QString second = fixture.filePath(QStringLiteral("второй файл.txt"));
        QVERIFY(writeFile(first) && writeFile(second));
        const QString viewerOutput = fixture.filePath("viewer.json");
        const QString editorOutput = fixture.filePath("editor.json");
        const QString settings = qEnvironmentVariable("SEVENZIP_SETTINGS_PATH");
        SaveRegEditor(false, ToUString(command(viewerOutput)), settings);
        SaveRegEditor(true, ToUString(command(editorOutput)), settings);
        ControlledBackend backend;
        MainWindow window(&backend);
        navigate(window, fixture.path());
        window.show();
        window.activateWindow();
        QTRY_VERIFY(filesystemRow(window, QStringLiteral("второй файл.txt")) >= 0);
        auto *view = table(window);
        view->selectAll();
        const int focused = filesystemRow(window, QStringLiteral("второй файл.txt"));
        view->setCurrentCell(focused, 0, QItemSelectionModel::NoUpdate);
        view->setFocus();
        QTRY_VERIFY(view->hasFocus());
        QTRY_VERIFY(action(window, "editFileAction")->isEnabled());
        QTest::keyClick(view, Qt::Key_F3);
        QTRY_COMPARE(readArguments(viewerOutput), QStringList({second}));
        QTest::keyClick(view, Qt::Key_F4);
        QTRY_COMPARE(readArguments(editorOutput), QStringList({second}));
        QVERIFY(!window.findChild<FileViewerDialog *>("fileViewerDialog"));
    }

    void diffUsesSelectedPairAndReportsLaunchFailure() {
        QTemporaryDir fixture;
        const QString first = fixture.filePath("a.txt");
        const QString second = fixture.filePath(QStringLiteral("б с пробелом.txt"));
        QVERIFY(writeFile(first) && writeFile(second));
        const QString output = fixture.filePath("diff.json");
        const QString settings = qEnvironmentVariable("SEVENZIP_SETTINGS_PATH");
        SaveRegDiff(ToUString(command(output)), settings);
        ControlledBackend backend;
        MainWindow window(&backend);
        navigate(window, fixture.path());
        window.show();
        QTRY_VERIFY(filesystemRow(window, "a.txt") >= 0);
        auto *view = table(window);
        view->selectAll();
        QTRY_VERIFY(action(window, "diffAction")->isEnabled());
        const int firstRow = filesystemRow(window, "a.txt");
        const int secondRow = filesystemRow(window, QStringLiteral("б с пробелом.txt"));
        const bool firstInSourceOrder = view->item(firstRow, 0)->data(Qt::UserRole + 6).toUInt() <
                                        view->item(secondRow, 0)->data(Qt::UserRole + 6).toUInt();
        const QStringList expected =
            firstInSourceOrder ? QStringList({first, second}) : QStringList({second, first});
        action(window, "diffAction")->trigger();
        QTRY_COMPARE(readArguments(output), expected);
        SaveRegDiff(ToUString(fixture.filePath("missing executable")), settings);
        bool errorShown = false;
        QTimer::singleShot(0, [&] {
            if (auto *box = qobject_cast<QMessageBox *>(QApplication::activeModalWidget())) {
                errorShown = box->textFormat() == Qt::PlainText;
                box->accept();
            }
        });
        action(window, "diffAction")->trigger();
        QVERIFY(errorShown);
        QVERIFY(QFileInfo(first).isFile() && QFileInfo(second).isFile());
    }

    void diffUsesOppositeSelectionOrRelativeFallback_data() {
        QTest::addColumn<bool>("selected");
        QTest::addColumn<bool>("flat");
        QTest::newRow("opposite-selection") << true << false;
        QTest::newRow("relative-fallback") << false << false;
        QTest::newRow("flat-selection") << true << true;
        QTest::newRow("flat-basename-fallback") << false << true;
    }
    void diffUsesOppositeSelectionOrRelativeFallback() {
        QFETCH(bool, selected);
        QFETCH(bool, flat);
        QTemporaryDir fixture;
        const QString left = fixture.filePath("left");
        const QString nested = QDir(left).filePath("nested");
        const QString right = fixture.filePath("right");
        QVERIFY(QDir().mkpath(nested) && QDir().mkpath(right));
        const QString source = QDir(nested).filePath(QStringLiteral("файл с пробелом.txt"));
        const QString expected =
            QDir(right).filePath(selected ? QString("other.txt") : QFileInfo(source).fileName());
        QVERIFY(writeFile(source) && writeFile(expected));
        const QString output = fixture.filePath("diff.json");
        SaveRegDiff(ToUString(command(output)), qEnvironmentVariable("SEVENZIP_SETTINGS_PATH"));
        ControlledBackend backend;
        MainWindow window(&backend);
        navigate(window, flat ? left : nested);
        window.show();
        window.activateWindow();
        auto *leftPanel = window.findChild<QWidget *>("filePanel0");
        auto *leftTable = leftPanel->findChild<QTableWidget *>("archiveTable");
        QTRY_VERIFY(leftTable->rowCount() > 0);
        leftTable->setFocus();
        if (flat)
            action(window, "flatViewAction")->trigger();
        const QString displayed =
            (flat ? QString("nested/") : QString()) + QFileInfo(source).fileName();
        QTRY_VERIFY(filesystemRow(window, displayed) >= 0);
        QTRY_VERIFY(leftTable->isEnabled());
        leftTable->selectRow(filesystemRow(window, displayed));
        action(window, "twoPanelsAction")->trigger();
        QVERIFY(action(window, "twoPanelsAction")->isChecked());
        auto *rightPanel = window.findChild<QWidget *>("filePanel1");
        auto *rightTable = rightPanel->findChild<QTableWidget *>("archiveTable");
        auto *address = rightPanel->findChild<QLineEdit *>("archivePath");
        address->setFocus();
        address->setText(right);
        QTest::keyClick(address, Qt::Key_Return);
        QTRY_COMPARE(rightTable->rowCount(), 1);
        if (action(window, "flatViewAction")->isChecked())
            action(window, "flatViewAction")->trigger();
        if (selected)
            rightTable->selectRow(0);
        else
            rightTable->clearSelection();
        window.activateWindow();
        QApplication::processEvents();
        QTRY_VERIFY(leftTable->isEnabled());
        leftTable->setFocus();
        // Activating a retained flat panel can start its deferred rescan.
        // The source scan guard temporarily disables the list and drops focus.
        QTRY_VERIFY(leftTable->isEnabled());
        leftTable->setFocus();
        QTRY_VERIFY(leftTable->hasFocus());
        const auto sourceRow = [&] {
            for (int row = 0; row < leftTable->rowCount(); ++row)
                if (leftTable->item(row, 0)->text() == displayed)
                    return row;
            return -1;
        };
        QTRY_VERIFY(sourceRow() >= 0);
        leftTable->selectRow(sourceRow());
        QTRY_VERIFY(action(window, "diffAction")->isEnabled());
        action(window, "diffAction")->trigger();
        QTRY_COMPARE(readArguments(output), QStringList({source, expected}));
    }
};

int main(int argc, char **argv) {
    if (argc >= 3 && QByteArray(argv[1]) == "--external-fixture") {
        QCoreApplication app(argc, argv);
        QFile output(app.arguments()[2]);
        if (!output.open(QIODevice::WriteOnly))
            return 1;
        const QByteArray bytes =
            QJsonDocument(QJsonArray::fromStringList(app.arguments().mid(3))).toJson();
        return output.write(bytes) == bytes.size() ? 0 : 2;
    }
    QApplication app(argc, argv);
    ExternalEditorsUiTests tests;
    return QTest::qExec(&tests, argc, argv);
}
#include "external_editors_ui_tests.moc"
