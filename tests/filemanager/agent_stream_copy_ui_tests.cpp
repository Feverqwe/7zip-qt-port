#include "archive/agent/Agent.h"
#include "archive/support/source_extract_fixture.h"
#include "dialogs/copy/alias.h"
#include "filemanager/support/window_state_support.h"
#include "filemanager/views/filetable.h"
#include "localization/LangUtils.h"
#include "ui/support/layout_inspection.h"
#include <QScopeGuard>

using namespace SourceExtractFixture;

class AgentStreamCopyUiTests final : public QObject, protected MainWindowTestSupport {
    Q_OBJECT
    static int row(FileTable &view, const QString &name) {
        for (int i = 0; i < view.rowCount(); ++i)
            if (view.model()->index(i, 0).data().toString().normalized(
                    QString::NormalizationForm_C) == name)
                return i;
        return -1;
    }
    static FileTable *openPanel(MainWindow &window, unsigned index, const QString &path) {
        auto *panel = window.findChild<QWidget *>(QString("filePanel%1").arg(index));
        auto *address = panel->findChild<QLineEdit *>("archivePath");
        address->setFocus();
        address->setText(path);
        QTest::keyClick(address, Qt::Key_Return);
        return static_cast<FileTable *>(panel->findChild<FileTable *>("archiveTable"));
    }
    static void inspectWindow(MainWindow &window, const QString &tag) {
        for (const QSize size : {QSize(980, 620), QSize(640, 400)}) {
            window.resize(size);
            QApplication::processEvents();
            for (unsigned pane : {0u, 1u}) {
                auto *ui = window.findChild<QWidget *>(QString("filePanel%1").arg(pane));
                QVERIFY(UiInspection::fullyVisible(*ui->findChild<FileTable *>("archiveTable"),
                                                   window));
                QVERIFY(
                    UiInspection::fullyVisible(*ui->findChild<QLineEdit *>("archivePath"), window));
            }
            QVERIFY(UiInspection::writeReport(
                window, QString("%1-%2x%3").arg(tag).arg(size.width()).arg(size.height())));
        }
    }
    static void transfer(MainWindow &window, FileTable &source, const QString &name,
                         const QString &destination, bool move, bool cancel, const QString &tag,
                         bool *deleteConfirmed = nullptr,
                         QMessageBox::StandardButton deleteAnswer = QMessageBox::Yes) {
        QTRY_VERIFY(source.isEnabled());
        const int index = row(source, name);
        QVERIFY(index >= 0);
        source.clearSelection();
        source.setCurrentCell(index, 0);
        source.selectRow(index);
        window.activateWindow();
        QVERIFY(QTest::qWaitForWindowActive(&window));
        source.setFocus();
        QTRY_VERIFY(source.hasFocus());
        bool seen = false;
        QStringList errors;
        QTimer driver;
        connect(&driver, &QTimer::timeout, &window, [&] {
            if (auto *box = qobject_cast<QMessageBox *>(QApplication::activeModalWidget())) {
                if (move && !cancel && box->button(QMessageBox::Yes)) {
                    if (deleteConfirmed)
                        *deleteConfirmed = true;
                    QVERIFY(box->button(deleteAnswer));
                    box->button(deleteAnswer)->click();
                    return;
                }
                errors << box->text();
                box->accept();
                return;
            }
            auto *dialog = window.findChild<CopyDialog *>("copyDialog");
            if (!dialog || !dialog->isVisible() || seen)
                return;
            seen = true;
            const auto close = qScopeGuard([&] {
                if (dialog->isVisible())
                    dialog->reject();
            });
            QCOMPARE(QDir::cleanPath(dialog->destination()), QDir::cleanPath(destination));
            auto *summary = dialog->findChild<QPlainTextEdit *>("copySelectionInfo");
            QVERIFY(summary);
            QVERIFY(summary->toPlainText().normalized(QString::NormalizationForm_C).contains(name));
            for (const QSize size : {QSize(440, 250), QSize(340, 220)}) {
                dialog->resize(size);
                QApplication::processEvents();
                for (const char *control :
                     {"copyDestination", "copyBrowseButton", "copySelectionInfo", "copyButtons"})
                    QVERIFY(UiInspection::fullyVisible(*dialog->findChild<QWidget *>(control),
                                                       *dialog));
                QVERIFY(UiInspection::writeReport(*dialog,
                                                  QString("%1-%2").arg(tag).arg(size.width())));
            }
            cancel ? dialog->reject() : dialog->accept();
        });
        driver.start(1);
        QTest::keyClick(&source, move ? Qt::Key_F6 : Qt::Key_F5);
        driver.stop();
        QVERIFY(seen);
        QVERIFY2(errors.isEmpty(), qPrintable(errors.join('\n')));
        QTRY_VERIFY(action(window, "refreshAction")->isEnabled());
    }
    static void verifySelection(FileTable &target, const CSelectedState &before,
                                const char *stage) {
        QTRY_VERIFY(target.isEnabled());
        CSelectedState actual;
        target.sourcePanel->SaveSelectedState(actual);
        QVERIFY2(actual.SelectedNames.Size() == before.SelectedNames.Size(), stage);
        QCOMPARE(actual.SelectedNames.Size(), 1u);
        QCOMPARE(ToQString(actual.SelectedNames[0]), ToQString(before.SelectedNames[0]));
        QCOMPARE(ToQString(actual.FocusedName), ToQString(before.FocusedName));
    }

  private slots:
    void copyAndMoveToPeerStreams_data() {
        QTest::addColumn<unsigned>("targetPane");
        QTest::addColumn<QString>("language");
        QTest::addColumn<bool>("sourceArchive");
        for (unsigned pane : {0u, 1u})
            for (const QString &language : {QString("-"), QString("ru")})
                for (bool archive : {false, true})
                    QTest::newRow(
                        qPrintable(QString("%1-%2-%3").arg(pane).arg(language).arg(archive)))
                        << pane << language << archive;
    }
    void copyAndMoveToPeerStreams() {
#ifndef SEVENZIP_CODEC_MODULE
        QSKIP("Official module unavailable; source ADS Copy/Move UI unverified");
#else
        QFETCH(unsigned, targetPane);
        QFETCH(QString, language);
        QFETCH(bool, sourceArchive);
        const QString settings = qEnvironmentVariable("SEVENZIP_SETTINGS_PATH");
        QVERIFY(!settings.isEmpty());
        QFile::remove(settings);
        SaveRegLang(ToUString(language), settings);
        ReloadLang(settings);
        QTemporaryDir fixture;
        QVERIFY(fixture.isValid());
        const QString archive = fixture.filePath("archive streams.wim");
        QVERIFY(QFile::copy(QString::fromUtf8(ALT_STREAMS_FIXTURE), archive));
        const QByteArray original = readFile(archive);
        const QString input = fixture.filePath("source 空 with spaces");
        const QString copied = QStringLiteral("копия 空.bin"),
                      moved = QStringLiteral("перенос 空.bin");
        const QByteArray bytes("binary\0stream payload", 21);
        QVERIFY(writeFile(QDir(input).filePath(copied), bytes));
        QVERIFY(writeFile(QDir(input).filePath(moved), bytes));
        const QString inputArchive = fixture.filePath("source archive.zip");
        if (sourceArchive) {
            QProcess creator;
            creator.setWorkingDirectory(input);
            creator.start(QStandardPaths::findExecutable("7z"),
                          {"a", "-tzip", "--", inputArchive, copied, moved});
            QVERIFY(creator.waitForFinished(10000));
            QCOMPARE(creator.exitCode(), 0);
        }
        const QByteArray sourceBefore = readFile(inputArchive);
        ArchiveBackend backend;
        QSignalSpy ready(&backend, &ArchiveBackend::codecCatalogReady);
        QVERIFY(backend.discoverCodecCatalog(QString::fromUtf8(SEVENZIP_CODEC_MODULE)));
        QTRY_COMPARE(ready.size(), 1);
        QVERIFY(ready[0][0].toBool());
        MainWindow window(&backend);
        window.show();
        QVERIFY(QTest::qWaitForWindowActive(&window));
        if (!action(window, "twoPanelsAction")->isChecked())
            action(window, "twoPanelsAction")->trigger();
        auto *target = openPanel(window, targetPane, archive);
        QTRY_VERIFY(action(window, "flatViewAction")->isEnabled());
        action(window, "flatViewAction")->setChecked(true);
        const QString owner = QStringLiteral("файл δ with space.bin");
        const QString oldStream = QStringLiteral("поток δ");
        QTRY_VERIFY(row(*target, owner) >= 0);
        QTRY_VERIFY(target->isEnabled());
        target->selectRow(row(*target, owner));
        target->setCurrentCell(row(*target, owner), 0);
        QTRY_VERIFY(action(window, "altStreamsAction")->isEnabled());
        action(window, "altStreamsAction")->trigger();
        QTRY_VERIFY(row(*target, oldStream) >= 0);
        QTRY_VERIFY(target->isEnabled());
        CMyComPtr<IFolderFolder> provider = target->sourcePanel->GetFolder();
        QVERIFY(dynamic_cast<CAgentFolder *>(provider.Interface()));
        target->sourcePanel->SelectAll(false);
        target->sourcePanel->SelectSpec(ToUString(oldStream), true);
        target->setCurrentCell(row(*target, oldStream), 0, QItemSelectionModel::NoUpdate);
        CSelectedState targetSelection;
        target->sourcePanel->SaveSelectedState(targetSelection);
        QCOMPARE(targetSelection.SelectedNames.Size(), 1u);
        const QString destination = ToQString(target->sourcePanel->GetFsPath());
        QVERIFY(destination.endsWith(':'));
        auto *source = openPanel(window, 1 - targetPane, sourceArchive ? inputArchive : input);
        QTRY_VERIFY(row(*source, copied) >= 0 && row(*source, moved) >= 0);
        const int sourceRows = source->rowCount();
        const QString tag =
            QString("agent-stream-copy-%1-%2-%3").arg(targetPane).arg(language).arg(sourceArchive);
        inspectWindow(window, tag + "-before");
        transfer(window, *source, copied, destination, false, true, tag + "-cancel");
        if (QTest::currentTestFailed())
            return;
        QCOMPARE(readFile(archive), original);
        QCOMPARE(readFile(QDir(input).filePath(copied)), bytes);
        QCOMPARE(readFile(inputArchive), sourceBefore);
        verifySelection(*target, targetSelection, "after Copy dialog Cancel");
        if (QTest::currentTestFailed())
            return;
        if (sourceArchive) {
            const QByteArray targetBeforeMove = readFile(archive);
            bool asked = false;
            transfer(window, *source, moved, destination, true, false, tag + "-move-cancel", &asked,
                     QMessageBox::Cancel);
            if (QTest::currentTestFailed())
                return;
            QVERIFY(asked);
            QCOMPARE(readFile(archive), targetBeforeMove);
            QCOMPARE(readFile(inputArchive), sourceBefore);
            QTRY_VERIFY(row(*target, moved) < 0);
            QTRY_VERIFY(row(*source, moved) >= 0);
            verifySelection(*target, targetSelection, "after Move rollback");
            if (QTest::currentTestFailed())
                return;
            // Rollback deliberately reopens the restored physical archive.
            provider = target->sourcePanel->GetFolder();
        }
        transfer(window, *source, copied, destination, false, false, tag + "-copy");
        if (QTest::currentTestFailed())
            return;
        QTRY_VERIFY(row(*target, copied) >= 0);
        QCOMPARE(readFile(QDir(input).filePath(copied)), bytes);
        QCOMPARE(readFile(inputArchive), sourceBefore);
        bool deleteConfirmed = false;
        transfer(window, *source, moved, destination, true, false, tag + "-move", &deleteConfirmed);
        if (QTest::currentTestFailed())
            return;
        QTRY_VERIFY(row(*target, moved) >= 0);
        if (!sourceArchive)
            QTRY_VERIFY(!QFileInfo::exists(QDir(input).filePath(moved)));
        else {
            QVERIFY(deleteConfirmed);
            QTRY_VERIFY(row(*source, moved) < 0);
            QProcess extractor;
            const QString output = fixture.filePath("source output");
            extractor.start(QStandardPaths::findExecutable("7z"),
                            {"x", "-y", "-o" + output, "--", inputArchive});
            QVERIFY(extractor.waitForFinished(10000));
            QCOMPARE(extractor.exitCode(), 0);
            QCOMPARE(readFile(QDir(output).filePath(copied)), bytes);
            QVERIFY(!QFileInfo::exists(QDir(output).filePath(moved)));
        }
        QCOMPARE(target->sourcePanel->GetFolder(), provider.Interface());
        QVERIFY(row(*target, oldStream) >= 0);
        QTRY_COMPARE(source->rowCount(), sourceRows - 1);
        verifySelection(*target, targetSelection, "after Copy and Move");
        if (QTest::currentTestFailed())
            return;
        QVERIFY(source->sourcePanel->ShouldProcessNotifications() &&
                target->sourcePanel->ShouldProcessNotifications());
        QVERIFY(source->isEnabled() && target->isEnabled());
        QVERIFY(source->updatesEnabled() && target->updatesEnabled());
        QCOMPARE(window.focusWidget(), source);
        // Offscreen modal progress can leave the parent inactive. Verify its
        // remembered source focus, then reactivate without setting a new focus.
        window.activateWindow();
        QVERIFY(QTest::qWaitForWindowActive(&window));
        QTRY_VERIFY(source->hasFocus());
        inspectWindow(window, tag + "-after");
        const QString output = fixture.filePath("output");
        QProcess extractor;
        extractor.start(QStandardPaths::findExecutable("7z"),
                        {"x", "-y", "-sns", "-o" + output, "--", archive});
        QVERIFY(extractor.waitForFinished(10000));
        QCOMPARE(extractor.exitCode(), 0);
        QCOMPARE(readFile(QDir(output).filePath(owner)), QByteArray("main bytes"));
        QCOMPARE(readFile(QDir(output).filePath(owner + ':' + oldStream)),
                 QByteArray("archive stream bytes"));
        for (const QString &name : {copied, moved})
            QCOMPARE(readFile(QDir(output).filePath(owner + ':' + name)), bytes);
#endif
    }
};
QTEST_MAIN(AgentStreamCopyUiTests)
#include "agent_stream_copy_ui_tests.moc"
