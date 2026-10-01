#include "7zip/UI/FileManager/resourceGui.h"
#include "CPP/7zip/UI/FileManager/LangUtils.h"
#include "layout_inspection.h"
#include "qtlangcontrols.h"
#include "window_state_support.h"
#include <QTreeWidget>

class LanguageMainDialogTests final : public QObject {
    Q_OBJECT
    QTemporaryDir fixture;
    static QString messageText(const QTreeWidget *list) {
        QStringList lines;
        for (int i = 0; i < list->topLevelItemCount(); ++i)
            lines.append(list->topLevelItem(i)->text(1));
        return lines.join('\n');
    }
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
    void sourceOpenCancellationUsesSelectedLanguage() {
        for (const char *name : {"-", "ru", "-"}) {
            ControlledBackend backend;
            MainWindow window(&backend);
            language(name);
            const QString archive = fixture.filePath(QStringLiteral("cancel 密碼.zip"));
            QVERIFY(backend.prepareArchiveFixture(archive));
            backend.queueListAction([&] { backend.cancel(); });
            window.openArchive(archive);
            QCOMPARE(backend.lastResult(), E_ABORT);
            QCOMPARE(window.statusBar()->currentMessage(),
                     QString(name) == "ru" ? QStringLiteral("Операция отменена.")
                                           : QString("Operation canceled."));
            window.findChild<QAction *>("operationJournalAction")->trigger();
            auto *messages = window.findChild<QTreeWidget *>("IDL_MESSAGE");
            QVERIFY(messages);
            QVERIFY(messageText(messages).contains(QtLangText(IDS_OPENNING) + ": " + archive));
            QVERIFY(messageText(messages).contains(
                QString(name) == "ru" ? QStringLiteral("Ошибка: Операция отменена.")
                                      : QString("Error: Operation canceled.")));
        }
    }
    void cancellationUsesResultCodeInsteadOfMessageText() {
        ControlledBackend backend;
        MainWindow window(&backend);
        unsigned warningsShown = 0;
        QTimer answer;
        connect(&answer, &QTimer::timeout, &window, [&] {
            if (auto *warning = qobject_cast<QMessageBox *>(QApplication::activeModalWidget())) {
                ++warningsShown;
                warning->accept();
            }
        });
        answer.start(5);
        for (const char *name : {"-", "ru"}) {
            language(name);
            backend.fail(QStringLiteral("translated cancellation 密碼"), E_ABORT);
            QApplication::processEvents();
            QCOMPARE(warningsShown, 0u);
            QCOMPARE(window.statusBar()->currentMessage(),
                     QStringLiteral("translated cancellation 密碼"));
        }
        backend.fail(QStringLiteral("Операция отменена."), E_FAIL);
        QCOMPARE(warningsShown, 1u);
    }
    void archiveSummaryRetainsMetadataWithoutBackendReload() {
        ControlledBackend backend;
        MainWindow window(&backend);
        const QString archive = fixture.filePath(QStringLiteral("архив 密碼.zip"));
        QVERIFY(backend.prepareArchiveFixture(archive));
        backend.queueListAction([&] {
            ArchiveInfo info;
            info.format = "zip";
            info.physicalSize = 120;
            info.files = 1;
            info.unpackedSize = 3;
            emit backend.infoReady(info);
            backend.completeList({ArchiveEntry{QStringLiteral("δ space.txt"), 3, 7}});
        });
        window.openArchive(archive);
        window.findChild<QAction *>("archiveInfoAction")->trigger();
        auto *dialog = window.findChild<QDialog *>("archiveInfoDialog");
        QVERIFY(dialog);
        auto *text = dialog->findChild<QLabel *>("archiveInfoText");
        QVERIFY(text);
        const int calls = backend.listCalls;
        for (const char *name : {"-", "ru", "-"}) {
            language(name);
            const bool russian = QString(name) == "ru";
            QCOMPARE(dialog->windowTitle(), russian ? QStringLiteral("Информация об архиве")
                                                    : QString("Archive information"));
            QCOMPARE(text->text(),
                     russian ? QStringLiteral("Формат: zip\nРазмер архива: 120 байт\nФайлов: "
                                              "1\nПапок: 0\nРазмер файлов: 3 байт")
                             : QString("Format: zip\nArchive size: 120 bytes\nFiles: 1\nFolders: "
                                       "0\nUnpacked size: 3 bytes"));
            QCOMPARE(backend.listCalls, calls);
            QCOMPARE(backend.extractCalls, 0);
            for (QSize size : {QSize(420, 240), QSize(300, 180)}) {
                dialog->resize(size);
                QApplication::processEvents();
                QVERIFY(UiInspection::fullyVisible(*text, *dialog));
                QVERIFY(UiInspection::labelFits(*text));
                QVERIFY(UiInspection::writeReport(
                    *dialog, QString("language-archive-info-%1-%2").arg(name).arg(size.width())));
            }
        }
        dialog->close();
    }
};

QTEST_MAIN(LanguageMainDialogTests)
#include "language_main_dialog_tests.moc"
