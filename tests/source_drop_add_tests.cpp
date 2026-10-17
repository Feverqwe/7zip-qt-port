#include "7zip/UI/FileManager/ProgressDialog2Res.h"
#include "CPP/7zip/UI/FileManager/ProgressDialog2.h"
#include "archivecodeccatalog.h"
#include "createarchivedialog.h"
#include "fixtures/drag_event_position.h"
#include "fixtures/widget_dialogs.h"
#include "source_extract_fixture.h"
#include "window_state_support.h"
#include <QScopeGuard>

class SourceDropAddTests final : public QObject, protected MainWindowTestSupport {
    Q_OBJECT
    WidgetDialogFixture dialogs_;
    QString module_, engine_;
    quint32 version_ = 0;
  private slots:
    void initTestCase() {
#ifndef SEVENZIP_CODEC_MODULE
        QSKIP("Official native module required for source drop Add");
#else
        module_ = QString::fromUtf8(SEVENZIP_CODEC_MODULE);
        engine_ = QStandardPaths::findExecutable("7zz");
        QVERIFY(!engine_.isEmpty());
        ArchiveCodecCatalog probe(module_, 0);
        QCOMPARE(probe.result(), S_OK);
        version_ = probe.version();
#endif
    }
    void dropUsesSourceUpdate_data() {
        QTest::addColumn<QString>("format");
        QTest::addColumn<bool>("cancel");
        QTest::addColumn<bool>("broken");
        QTest::newRow("7z") << QString("7z") << false << false;
        QTest::newRow("zip") << QString("zip") << false << false;
        QTest::newRow("cancel") << QString("7z") << true << false;
        QTest::newRow("broken-existing") << QString("7z") << false << true;
    }
    void dropUsesSourceUpdate() {
        QFETCH(QString, format);
        QFETCH(bool, cancel);
        QFETCH(bool, broken);
        QFile::remove(qEnvironmentVariable("SEVENZIP_SETTINGS_PATH"));
        QTemporaryDir fixture;
        const QString name = QStringLiteral("данные 空 with spaces.bin");
        const QString input = fixture.filePath("source/" + name);
        const QString output = fixture.filePath("output/result." + format);
        QVERIFY(SourceExtractFixture::writeFile(input, "source drop bytes"));
        QVERIFY(QDir().mkpath(QFileInfo(output).absolutePath()));
        if (broken)
            QVERIFY(SourceExtractFixture::writeFile(output, "sentinel"));
        ArchiveBackend backend(nullptr, engine_);
        QCOMPARE(backend.loadCodecCatalog(module_, version_), S_OK);
        MainWindow window(&backend);
        window.show();
        window.activateWindow();
        QVERIFY(QTest::qWaitForWindowActive(&window));
        auto *address = window.findChild<QLineEdit *>("archivePath");
        address->setText(QFileInfo(output).absolutePath());
        emit address->returnPressed();
        QTRY_VERIFY(table(window)->isEnabled());
        QSignalSpy processes(&backend, &ArchiveBackend::engineStarted);
        bool shown = false;
        QStringList errors;
        QTimer answer;
        dialogs_.connectResponder(answer, &window, [&] {
            if (auto *dialog = qobject_cast<CreateArchiveDialog *>(dialogs_.activeDialog())) {
                shown = true;
                const auto reject = qScopeGuard([&] {
                    if (dialog->isVisible())
                        dialog->reject();
                });
                QVERIFY(!dialog->findChild<QListWidget *>("availableSources"));
                QCOMPARE(dialog->sources(), QStringList{input});
                if (cancel)
                    dialog->reject();
                else {
                    dialog->findChild<QComboBox *>("archiveFormat")->setCurrentText(format);
                    dialog->findChild<QLineEdit *>("archiveName")->setText(output);
                    dialog->findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Ok)->click();
                }
            } else if (auto *box = qobject_cast<QMessageBox *>(dialogs_.activeDialog())) {
                errors.append(box->text());
                box->accept();
            } else if (auto *progress = dynamic_cast<CProgressDialog *>(dialogs_.activeDialog())) {
                auto *button = progress->findChild<QPushButton *>("IDCANCEL");
                if (button && button->property("languageID").toUInt() == IDS_CLOSE)
                    button->click();
            }
        });
        answer.start(1);
        QMimeData mime;
        mime.setUrls({QUrl::fromLocalFile(input)});
        QDragEnterEvent enter(DragEventPosition(QPoint(5, 5)), Qt::CopyAction, &mime,
                              Qt::LeftButton, Qt::NoModifier);
        QCoreApplication::sendEvent(table(window)->viewport(), &enter);
        QVERIFY(enter.isAccepted());
        QDropEvent drop(QPointF(5, 5), Qt::CopyAction, &mime, Qt::LeftButton, Qt::NoModifier);
        QCoreApplication::sendEvent(table(window)->viewport(), &drop);
        answer.stop();
        QVERIFY(shown);
        QCOMPARE(processes.size(), 0);
        QCOMPARE(SourceExtractFixture::readFile(input), QByteArray("source drop bytes"));
        QVERIFY(!backend.busy());
        if (cancel) {
            QVERIFY(errors.isEmpty());
            QVERIFY(!QFileInfo::exists(output));
        } else if (broken) {
            QCOMPARE(errors.size(), 1);
            QCOMPARE(SourceExtractFixture::readFile(output), QByteArray("sentinel"));
        } else {
            QVERIFY2(errors.isEmpty(), qPrintable(errors.join('\n')));
            QProcess verify;
            const QString extracted = fixture.filePath("extracted");
            verify.start(engine_, {"x", "-y", "-o" + extracted, "--", output});
            QVERIFY(verify.waitForFinished(10000));
            QCOMPARE(verify.exitCode(), 0);
            QCOMPARE(SourceExtractFixture::readFile(QDir(extracted).filePath(name)),
                     QByteArray("source drop bytes"));
        }
    }
};
QTEST_MAIN(SourceDropAddTests)
#include "source_drop_add_tests.moc"
