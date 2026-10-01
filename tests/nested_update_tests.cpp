#include "CPP/7zip/UI/FileManager/Panel.h"
#include "CPP/7zip/UI/FileManager/ProgressDialog2.h"
#include "archivebackend.h"
#include "qtarchivefoldermanager.h"
#include "source_extract_fixture.h"
#include <QApplication>
#include <QMessageBox>
#include <QPushButton>
#include <QTemporaryDir>
#include <QTimer>
#include <QtTest>

using namespace SourceExtractFixture;

class NestedUpdateBackend final : public ArchiveBackend {
  public:
    bool FailUpdate = false;
    bool UpdateStarted = false;
    void updateSourceFile(const QString &archive, const QString &entry, const QString &root,
                          const QString &output, const QString &format,
                          const QString &password) override {
        UpdateStarted = true;
        if (FailUpdate)
            emit finished(false, QStringLiteral("Fixture nested update failed."));
        else
            ArchiveBackend::updateSourceFile(archive, entry, root, output, format, password);
    }
};

class NestedUpdateTests final : public QObject {
    Q_OBJECT
  private slots:
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
        const QString outerArchive = makeArchive(outer.path(), readFile(innerArchive));
        const QString changedArchive =
            makeArchive(changed.path(), "updated inner contents and size");
        QVERIFY(!innerArchive.isEmpty() && !outerArchive.isEmpty() && !changedArchive.isEmpty());
        const QByteArray originalOuter = readFile(outerArchive);
        const QByteArray replacement = readFile(changedArchive);
        const QString temporaryFile = temporary.filePath(QStringLiteral("密碼 с пробелом.bin"));
        QVERIFY(writeFile(temporaryFile, readFile(innerArchive)));

        NestedUpdateBackend backend;
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
        backend.FailUpdate = outcome == 1;
        connect(&backend, &ArchiveBackend::engineStarted, &backend, [&] {
            if (outcome == 2 && backend.UpdateStarted)
                backend.cancel();
        });
        int prompts = 0, errors = 0;
        QTimer responder;
        connect(&responder, &QTimer::timeout, this, [&] {
            if (auto *box = qobject_cast<QMessageBox *>(QApplication::activeModalWidget())) {
                if (box->objectName() == "modifiedArchiveQuestion") {
                    ++prompts;
                    box->button(QMessageBox::Yes)->click();
                } else {
                    ++errors;
                    box->accept();
                }
                return;
            }
            if (auto *progress =
                    dynamic_cast<CProgressDialog *>(QApplication::activeModalWidget())) {
                auto *close = progress->findChild<QPushButton *>("IDCANCEL");
                if (close && close->text().contains(QStringLiteral("Close")))
                    close->click();
            }
        });
        responder.start(1);
        panel.OpenParentFolder();
        responder.stop();
        QCOMPARE(prompts, 1);
        QVERIFY(backend.UpdateStarted);
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
