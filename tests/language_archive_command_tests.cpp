#include "7zip/UI/FileManager/resource.h"
#include "7zip/UI/FileManager/resourceGui.h"
#include "7zip/UI/GUI/CompressDialogRes.h"
#include "7zip/UI/GUI/ExtractRes.h"
#include "CPP/7zip/UI/FileManager/LangUtils.h"
#include "CPP/7zip/UI/FileManager/RegistryUtils.h"
#include "layout_inspection.h"
#include "qtlangcontrols.h"
#include "window_state_support.h"

class LanguageArchiveCommandTests final : public QObject {
    Q_OBJECT
    QTemporaryDir fixture;
    void language(const char *name) {
        const QString path = fixture.filePath("language.ini");
        SaveRegLang(UString(name), path);
        ReloadLang(path);
        QtNotifyLanguageChanged();
    }

  private slots:
    void init() {
        QFile::remove(qEnvironmentVariable("SEVENZIP_SETTINGS_PATH"));
        language("-");
    }
    void cleanup() { language("-"); }
    void compressionMemoryErrorSourceAndGeometry() {
        const QString path = fixture.filePath(QStringLiteral("memory 密碼 file.txt"));
        QFile file(path);
        QVERIFY(file.open(QIODevice::WriteOnly));
        QCOMPARE(file.write("memory fixture"), qint64(14));
        file.close();
        CreateArchiveDialog dialog(fixture.path(), {path});
        auto *limit = dialog.findChild<QComboBox *>("compressionMemoryLimit");
        QVERIFY(limit);
        limit->addItem("0 B", "0b");
        limit->setCurrentIndex(limit->count() - 1);
        for (const char *name : {"-", "ru", "-"}) {
            language(name);
            for (QSize size : {QSize(760, 720), QSize(640, 480)}) {
                dialog.resize(size);
                dialog.show();
                QString message;
                bool geometry = false, reported = false;
                QTimer::singleShot(0, &dialog, [&] {
                    auto *box = qobject_cast<QMessageBox *>(QApplication::activeModalWidget());
                    QVERIFY(box);
                    message = box->text();
                    auto *label = box->findChild<QLabel *>("qt_msgbox_label");
                    geometry = label && UiInspection::fullyVisible(*label, *box) &&
                               UiInspection::labelFits(*label);
                    for (auto *button : box->buttons())
                        geometry = geometry && UiInspection::fullyVisible(*button, *box);
                    reported = UiInspection::writeReport(
                        *box,
                        QString("language-compress-memory-%1-%2").arg(name).arg(size.width()));
                    box->accept();
                });
                dialog.findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Ok)->click();
                QVERIFY(geometry && reported);
                QVERIFY(message.startsWith(
                    QString::fromWCharArray(LangString(IDS_MEM_OPERATION_BLOCKED).Ptr())));
                QVERIFY(message.contains(
                    QString::fromWCharArray(LangString(IDS_MEM_REQUIRES_BIG_MEM).Ptr())));
                QVERIFY(
                    message.contains(QString::fromWCharArray(LangString(IDS_MEM_RAM_SIZE).Ptr())));
                QVERIFY(message.contains(
                    "0 MB : " +
                    QString::fromWCharArray(LangString(IDS_MEM_USAGE_LIMIT_SET_BY_7ZIP).Ptr())));
                QVERIFY(message.endsWith(QString::fromWCharArray(LangString(IDS_MEM_ERROR).Ptr())));
                QVERIFY(dialog.isVisible());
                QCOMPARE(limit->currentData().toString(), QString("0b"));
                QCOMPARE(dialog.result(), 0);
            }
        }
        QVERIFY(file.open(QIODevice::ReadOnly));
        QCOMPARE(file.readAll(), QByteArray("memory fixture"));
    }
    void externalEditorFailureUsesSourceMessageAndGeometry() {
        const QString path = fixture.filePath(QStringLiteral("large file 密碼.txt"));
        QFile file(path);
        QVERIFY(file.open(QIODevice::WriteOnly));
        QCOMPARE(file.write("source editor fixture"), qint64(21));
        file.close();
        const QString settings = qEnvironmentVariable("SEVENZIP_SETTINGS_PATH");
        const UString missing = ToUString('"' + fixture.filePath("missing editor") + '"');
        SaveRegEditor(false, missing, settings);
        SaveRegEditor(true, missing, settings);
        ControlledBackend backend;
        MainWindow window(&backend);
        auto *address = window.findChild<QLineEdit *>("archivePath");
        address->setText(fixture.path());
        QTest::keyClick(address, Qt::Key_Return);
        auto *table = window.findChild<QTableWidget *>("archiveTable");
        int row = -1;
        for (int i = 0; i < table->rowCount(); ++i)
            if (table->item(i, 0)->text() == QFileInfo(path).fileName())
                row = i;
        QVERIFY(row >= 0);
        table->selectRow(row);
        table->setCurrentCell(row, 0);
        for (const char *name : {"-", "ru", "-"}) {
            language(name);
            for (const char *command : {"viewFileAction", "editFileAction"}) {
                for (int width : {1100, 640}) {
                    window.resize(width, width == 640 ? 380 : 640);
                    window.show();
                    QString message;
                    bool geometry = false, reported = false;
                    QTimer::singleShot(0, &window, [&] {
                        auto *box = qobject_cast<QMessageBox *>(QApplication::activeModalWidget());
                        QVERIFY(box);
                        message = box->text();
                        auto *label = box->findChild<QLabel *>("qt_msgbox_label");
                        geometry = label && UiInspection::fullyVisible(*label, *box) &&
                                   UiInspection::labelFits(*label);
                        for (auto *button : box->buttons())
                            geometry = geometry && UiInspection::fullyVisible(*button, *box);
                        reported = UiInspection::writeReport(
                            *box, QString("language-source-editor-error-%1-%2-%3")
                                      .arg(name)
                                      .arg(command)
                                      .arg(width));
                        box->accept();
                    });
                    window.findChild<QAction *>(command)->trigger();
                    QCOMPARE(message, ToQString(LangString(IDS_CANNOT_START_EDITOR)));
                    QVERIFY(geometry && reported);
                    QCOMPARE(backend.extractCalls, 0);
                    QCOMPARE(backend.listCalls, 0);
                    QCOMPARE(table->item(row, 0)->text(), QFileInfo(path).fileName());
                    QCOMPARE(table->selectionModel()->selectedRows().size(), 1);
                }
            }
        }
    }
};

QTEST_MAIN(LanguageArchiveCommandTests)
#include "language_archive_command_tests.moc"
