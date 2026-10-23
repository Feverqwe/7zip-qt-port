#include "CPP/7zip/UI/FileManager/Panel.h"
#include "CPP/7zip/UI/FileManager/ProgressDialog2.h"
#include "archivebackend.h"
#include "fixtures/progress_dialog.h"
#include "fixtures/widget_dialogs.h"
#include "native_archive_fixture.h"
#include "native_property_failure_runtime.h"
#include "qtarchivefoldermanager.h"
#include "source_extract_fixture.h"
#include <QApplication>
#include <QMessageBox>
#include <QPushButton>
#include <QTemporaryDir>
#include <QTimer>
#include <QtTest>

using namespace SourceExtractFixture;

class NestedUpdateTests final : public QObject {
    Q_OBJECT
    WidgetDialogFixture widgetDialogs_;
    NativePropertyFailureRuntime runtime_;
  private slots:
    void initTestCase() {
        QVERIFY(runtime_.prepare(QString::fromUtf8(SEVENZIP_EXTERNAL_RUNTIME),
                                 QString::fromUtf8(SEVENZIP_EXTERNAL_FORMAT_FIXTURE)));
        QVERIFY(runtime_.enableWriter());
    }
    void init() { QVERIFY(runtime_.clear()); }
    void changedNestedArchiveReturnsThroughNativeUpdate_data() {
        QTest::addColumn<int>("outcome");
        QTest::newRow("success") << 0;
        QTest::newRow("failure-preserves-temp") << 1;
        QTest::newRow("cancel-preserves-temp") << 2;
    }
    void changedNestedArchiveReturnsThroughNativeUpdate() {
        QFETCH(int, outcome);
        QTemporaryDir inner, outer, changed, extracted, temporary;
        QVERIFY(inner.isValid() && outer.isValid() && changed.isValid() && extracted.isValid() &&
                temporary.isValid());
        const QString innerArchive = makeArchive(inner.path(), "original inner contents");
        const QString outerArchive = outer.filePath("outer.fixture7z");
        QVERIFY(NativeArchiveFixture::Write(outerArchive, {{kName, readFile(innerArchive)}}));
        const QString changedArchive =
            makeArchive(changed.path(), "updated inner contents and size");
        QVERIFY(!innerArchive.isEmpty() && !outerArchive.isEmpty() && !changedArchive.isEmpty());
        const QByteArray originalOuter = readFile(outerArchive);
        const QByteArray replacement = readFile(changedArchive);
        const QString temporaryFile = temporary.filePath(QStringLiteral("密碼 с пробелом.bin"));
        QVERIFY(writeFile(temporaryFile, readFile(innerArchive)));

        ArchiveBackend backend(nullptr, runtime_.engine());
        CMyComPtr<IFolderManager> manager = new QtArchiveFolderManager(backend);
        CPanel panel;
        panel.AttachFolderManager(manager, outer.filePath("settings.ini"));
        COpenResult opened;
        QCOMPARE(panel.BindToPath(ToUString(outerArchive + "/inside/"), UString(), opened), S_OK);
        QCOMPARE(panel.GetFolder()->LoadItems(), S_OK);
        CTempFileInfo temp;
        temp.FileIndex = 0;
        temp.RelPath = L"密碼 с пробелом.bin";
        temp.FilePath = us2fs(temporaryFile.toStdWString().c_str());
        temp.FolderPath = us2fs(temporary.path().toStdWString().c_str());
        temp.NeedDelete = true;
        QCOMPARE(panel.OpenAsArc(nullptr, temp, ToUString(outerArchive + '/' + kName),
                                 UString(L"zip"), opened),
                 S_OK);
        QCOMPARE(panel.GetParentFolders().Size(), 2u);
        QVERIFY(writeFile(temporaryFile, replacement));
        if (outcome)
            QVERIFY(runtime_.armWriter(outcome == 1 ? E_FAIL : E_ABORT, outcome == 2));
        bool cancelled = false;
        int prompts = 0, errors = 0;
        QTimer responder;
        widgetDialogs_.connectResponder(responder, this, [&] {
            if (auto *box = qobject_cast<QMessageBox *>(widgetDialogs_.activeDialog())) {
                if (box->objectName() == "progressCancelQuestion") {
                    box->button(QMessageBox::Yes)->click();
                } else if (box->objectName() == "modifiedArchiveQuestion") {
                    ++prompts;
                    box->button(QMessageBox::Yes)->click();
                } else {
                    ++errors;
                    box->accept();
                }
                return;
            }
            if (auto *progress = dynamic_cast<CProgressDialog *>(widgetDialogs_.activeDialog())) {
                auto *close = progress->findChild<QPushButton *>("IDCANCEL");
                if (TestProgressDialog::IsCloseButton(close))
                    close->click();
                else if (close && outcome == 2 && !cancelled &&
                         !runtime_.writerReceipts().isEmpty()) {
                    cancelled = true;
                    close->click();
                }
            }
        });
        responder.start(1);
        panel.OpenParentFolder();
        responder.stop();
        QCOMPARE(prompts, 1);
        QVERIFY(!runtime_.writerReceipts().isEmpty());
        QCOMPARE(cancelled, outcome == 2);
        QVERIFY(!backend.busy());
        QCOMPARE(panel.GetParentFolders().Size(), 1u);
        QCOMPARE(QDir::fromNativeSeparators(ToQString(GetFolderPath(panel.GetFolder()))),
                 QStringLiteral("inside/"));
        QCOMPARE(QFileInfo::exists(temporaryFile), outcome != 0);
        if (outcome != 0) {
            QVERIFY(errors > 0);
            QCOMPARE(readFile(outerArchive), originalOuter);
            QCOMPARE(readFile(temporaryFile), replacement);
        } else {
            QCOMPARE(errors, 0);
            QProcess process;
            process.start(QStandardPaths::findExecutable("7zz"),
                          {"x", "-y", "-o" + extracted.path(), "--", outerArchive});
            QVERIFY(process.waitForFinished(10000));
            QCOMPARE(process.exitCode(), 0);
            QCOMPARE(readFile(extracted.filePath(kName)), replacement);
        }
        panel.CloseOpenFolders();
    }
};
QTEST_MAIN(NestedUpdateTests)
#include "nested_update_tests.moc"
