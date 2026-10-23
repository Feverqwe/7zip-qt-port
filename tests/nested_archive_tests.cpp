#include "CPP/7zip/UI/FileManager/OverwriteDialog.h"
#include "CPP/7zip/UI/FileManager/ProgressDialog2.h"
#include "archivecodeccatalog.h"
#include "dot_path_cpio_fixture.h"
#include "fixtures/progress_dialog.h"
#include "mainwindow.h"

#include <QAction>
#include <QFile>
#include <QLineEdit>
#include <QMessageBox>
#include <QProcess>
#include <QPushButton>
#include <QSignalSpy>
#include <QStandardPaths>
#include <QTableWidget>
#include <QTemporaryDir>
#include <QTimer>
#include <QtTest>

class NestedBackend : public ArchiveBackend {
  public:
    NestedBackend() : ArchiveBackend(nullptr, "/missing/7zz") {}

    HRESULT lastResult() const override { return cancelExtraction ? E_ABORT : E_FAIL; }
    void openFolder(const QString &archive, const QString &,
                    const QString &password = {}) override {
        list(archive, password);
    }

    void list(const QString &archive, const QString & = {}) override {
        listed << archive;
        QTimer::singleShot(0, this, [this, archive] {
            ArchiveInfo info;
            info.format = "zip";
            emit infoReady(info);
            if (archive == listed.first())
                emit entriesReady(
                    {ArchiveEntry{"folder/вложенный архив.zip", 4, 4, {}, false, false}});
            else if (!failNestedListing)
                emit entriesReady({ArchiveEntry{"inside.txt", 3, 3, {}, false, false}});
            const bool success = archive == listed.first() || !failNestedListing;
            emit finished(success, success ? "Архив открыт." : "Неверный архив.");
        });
    }
    void extractSourceEntry(const QString &, const QString &entry, const QString &output,
                            const QString &, const QString &, bool = false) override {
        extracted = {entry};
        temporary = output;
        QTimer::singleShot(0, this, [this, output] {
            if (cancelExtraction) {
                emit finished(false, QStringLiteral("Операция отменена."));
                return;
            }
            QFile file(output);
            const bool ok = file.open(QIODevice::WriteOnly) && file.write("test") == 4;
            file.close();
            emit finished(ok, ok ? QStringLiteral("Извлечено.") : QStringLiteral("Ошибка записи."));
        });
    }
    void extract(const QString &, const QString &destination, const QStringList &selected,
                 const QString & = {}, const ExtractionOptions & = {}) override {
        extracted = selected;
        temporary = destination;
        QTimer::singleShot(0, this, [this, destination, selected] {
            if (cancelExtraction) {
                emit finished(false, "Операция отменена.");
                return;
            }
            const QString file = QDir(destination).filePath(selected.first());
            QDir().mkpath(QFileInfo(file).absolutePath());
            QFile output(file);
            if (output.open(QIODevice::WriteOnly)) {
                output.write("test");
                output.close();
                emit finished(true, "Извлечено.");
            } else
                emit finished(false, "Ошибка записи.");
        });
    }

    QStringList listed;
    QStringList extracted;
    QString temporary;
    bool cancelExtraction = false;
    bool failNestedListing = false;
};

class NestedArchiveTests : public QObject {
    Q_OBJECT

  private slots:
    void extractsDotFolderInNestedPayload_data() {
        QTest::addColumn<bool>("selectedOnly");
        QTest::addColumn<QString>("prefix");
        QTest::addColumn<bool>("native");
        QTest::newRow("whole-folder") << false << "." << false;
        QTest::newRow("selected-app") << true << "." << false;
        QTest::newRow("parent-whole-folder") << false << ".." << false;
        QTest::newRow("parent-selected-app") << true << ".." << false;
#ifdef SEVENZIP_CODEC_MODULE
        QTest::newRow("native-parent-whole-folder") << false << ".." << true;
        QTest::newRow("native-parent-selected-app") << true << ".." << true;
#endif
    }
    void extractsDotFolderInNestedPayload() {
        QFETCH(bool, selectedOnly);
        QFETCH(QString, prefix);
        QFETCH(bool, native);
        QTemporaryDir fixture;
        QVERIFY(fixture.isValid());
        const QString cpio = fixture.filePath("Payload~");
        QFile file(cpio);
        QVERIFY(file.open(QIODevice::WriteOnly));
        const QByteArray bytes = DotPathCpioFixture(prefix.toUtf8());
        QCOMPARE(file.write(bytes), qint64(bytes.size()));
        file.close();
        QVERIFY(QDir().mkpath(fixture.filePath("temp.pkg")));
        const auto make = [&](const QStringList &arguments) {
            QProcess process;
            process.setWorkingDirectory(fixture.path());
            process.start(QStandardPaths::findExecutable("7zz"), arguments);
            return process.waitForFinished(30000) && process.exitCode() == 0;
        };
        QVERIFY(make({"a", "-tgzip", "temp.pkg/Payload.gz", "Payload~"}));
        QVERIFY(QFile::rename(fixture.filePath("temp.pkg/Payload.gz"),
                              fixture.filePath("temp.pkg/Payload")));
        QVERIFY(make({"a", "-tzip", "installer.zip", "temp.pkg"}));
        ArchiveBackend backend;
        if (native) {
#ifdef SEVENZIP_CODEC_MODULE
            const QString module = QString::fromUtf8(SEVENZIP_CODEC_MODULE);
            QSignalSpy ready(&backend, &ArchiveBackend::codecCatalogReady);
            QVERIFY(backend.discoverCodecCatalog(module));
            QTRY_COMPARE(ready.size(), 1);
            QVERIFY2(ready[0][0].toBool(), qPrintable(ready[0][1].toString()));
#endif
        }
        MainWindow window(&backend);
        window.show();
        auto *table = window.findChild<QTableWidget *>("archiveTable");
        auto *inside = window.findChild<QAction *>("openInsideAction");
        QVERIFY(table && inside);
        window.openArchive(fixture.filePath("installer.zip"));
        const auto row = [&](const QString &name) {
            for (int i = 0; i < table->rowCount(); ++i)
                if (table->item(i, 0) && table->item(i, 0)->text() == name)
                    return i;
            return -1;
        };
        for (const QString &name : QStringList{"temp.pkg", "Payload", "Payload~", prefix}) {
            QTRY_VERIFY2(row(name) >= 0, qPrintable(name));
            table->selectRow(row(name));
            table->setCurrentCell(row(name), 0);
            inside->trigger();
        }
        QTRY_VERIFY(row("ProxyBridge.app") >= 0 && row("._ProxyBridge.app") >= 0);
        table->clearSelection();
        if (selectedOnly)
            table->selectRow(row("ProxyBridge.app"));
        QString errors;
        QTimer dismiss;
        connect(&dismiss, &QTimer::timeout, &window, [&] {
            for (QWidget *widget : QApplication::topLevelWidgets())
                if (auto *box = qobject_cast<QMessageBox *>(widget); box && box->isVisible()) {
                    errors += box->text();
                    box->accept();
                }
        });
        dismiss.start(1);
        const QString output = fixture.filePath("output");
        ExtractionOptions options;
        if (prefix == "..") {
            options.pathMode = ExtractionOptions::PathMode::CurrentPaths;
            options.currentFolder = prefix;
        }
        window.extractTo(output, selectedOnly, {}, options);
        QVERIFY2(errors.isEmpty(), qPrintable(errors));
        QFile extracted(QDir(output).filePath(QStringLiteral("ProxyBridge.app/имя 空.txt")));
        QVERIFY(extracted.open(QIODevice::ReadOnly));
        QCOMPARE(extracted.readAll(), QByteArray("payload bytes"));
        QCOMPARE(QFileInfo::exists(QDir(output).filePath("._ProxyBridge.app")), !selectedOnly);
    }
    void opensArchiveWithoutRecognizedExtension() {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const QString source = dir.filePath(QStringLiteral("данные с пробелом.txt"));
        QFile payload(source);
        QVERIFY(payload.open(QIODevice::WriteOnly));
        QCOMPARE(payload.write("payload"), 7);
        payload.close();
        ArchiveBackend backend;
        QSignalSpy finished(&backend, &ArchiveBackend::finished);
        const QString zip = dir.filePath("inner.zip");
        const QString unknown = dir.filePath("inner.bin");
        backend.create(zip, {source});
        QVERIFY(finished.wait(30000));
        QVERIFY(finished.takeFirst().first().toBool());
        QVERIFY(QFile::rename(zip, unknown));
        const QString outer = dir.filePath("outer.7z");
        backend.create(outer, {unknown});
        QVERIFY(finished.wait(30000));
        QVERIFY(finished.takeFirst().first().toBool());

        MainWindow window(&backend);
        window.show();
        const QString initialDirectory = window.findChild<QLineEdit *>("archivePath")->text();
        auto *table = window.findChild<QTableWidget *>("archiveTable");
        auto *inside = window.findChild<QAction *>("openInsideAction");
        auto *back = window.findChild<QAction *>("backAction");
        QVERIFY(table && inside && back);
        auto *address = window.findChild<QLineEdit *>("archivePath");
        address->setText(unknown);
        QTest::keyClick(address, Qt::Key_Return);
        QTRY_COMPARE(table->rowCount(), 1);
        QCOMPARE(table->item(0, 0)->text().normalized(QString::NormalizationForm_C),
                 QStringLiteral("данные с пробелом.txt"));

        window.openArchive(outer);
        QTRY_VERIFY(table->rowCount() == 1 && table->item(0, 0) &&
                    table->item(0, 0)->text() == QStringLiteral("inner.bin"));
        table->selectRow(0);
        QVERIFY(inside->isEnabled());
        inside->trigger();
        QTRY_VERIFY(table->rowCount() == 1 && table->item(0, 0) &&
                    table->item(0, 0)->text().normalized(QString::NormalizationForm_C) ==
                        QStringLiteral("данные с пробелом.txt"));
        back->trigger();
        QTRY_VERIFY(table->rowCount() == 1 && table->item(0, 0) &&
                    table->item(0, 0)->text() == QStringLiteral("inner.bin"));

        QFile plain(dir.filePath("plain.bin"));
        QVERIFY(plain.open(QIODevice::WriteOnly));
        plain.write("ordinary data");
        plain.close();
        finished.clear();
        QTimer dismissWarning;
        connect(&dismissWarning, &QTimer::timeout, &window, [] {
            for (QWidget *widget : QApplication::topLevelWidgets()) {
                if (auto *box = qobject_cast<QMessageBox *>(widget); box && box->isVisible())
                    box->accept();
            }
        });
        dismissWarning.start(1);
        window.openArchive(plain.fileName());
        QTRY_COMPARE(finished.size(), 1);
        dismissWarning.stop();
        QVERIFY(!finished.first().first().toBool());
        QCOMPARE(window.findChild<QLineEdit *>("archivePath")->text(), initialDirectory);
    }

    void opensAndExtractsTarArchiveReadOnly() {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const QString sourceName = QStringLiteral("данные с пробелом.txt");
        QFile source(dir.filePath(sourceName));
        QVERIFY(source.open(QIODevice::WriteOnly));
        source.write("tar payload");
        source.close();
        const QString archive = dir.filePath("sample.tar");
        QString executable = QStandardPaths::findExecutable("7zz");
        if (executable.isEmpty())
            executable = "/opt/homebrew/bin/7zz";
        QProcess creator;
        creator.setWorkingDirectory(dir.path());
        creator.start(executable, {"a", "-ttar", "--", archive, sourceName});
        QVERIFY(creator.waitForFinished(30000));
        QCOMPARE(creator.exitCode(), 0);

        ArchiveBackend backend;
        MainWindow window(&backend);
        window.show();
        window.openArchive(archive);
        auto *table = window.findChild<QTableWidget *>("archiveTable");
        auto *add = window.findChild<QAction *>("toolbarAddAction");
        auto *remove = window.findChild<QAction *>("deleteAction");
        auto *rename = window.findChild<QAction *>("renameAction");
        auto *move = window.findChild<QAction *>("moveAction");
        auto *edit = window.findChild<QAction *>("editFileAction");
        QVERIFY(table && add && remove && rename && move && edit);
        QTRY_COMPARE(table->rowCount(), 1);
        QCOMPARE(table->item(0, 0)->text().normalized(QString::NormalizationForm_C), sourceName);
        table->selectRow(0);
        QVERIFY(!add->isEnabled());
        QVERIFY(!remove->isEnabled());
        QVERIFY(!rename->isEnabled());
        QVERIFY(!move->isEnabled());
        QVERIFY(edit->isEnabled());

        const QString destination = dir.filePath("output");
        QVERIFY(QDir().mkdir(destination));
        QSignalSpy finished(&backend, &ArchiveBackend::finished);
        window.extractTo(destination);
        // Source modal extraction joins its worker before returning. Backend
        // signals describe its listing/stream subrequests, not the UI operation.
        QVERIFY(!backend.busy());
        for (const auto &result : finished)
            QVERIFY2(result.first().toBool(), qPrintable(result.at(1).toString()));
        QFile extracted(QDir(destination).filePath(sourceName));
        QVERIFY(extracted.open(QIODevice::ReadOnly));
        QCOMPARE(extracted.readAll(), QByteArray("tar payload"));

        const QString compressed = dir.filePath("sample.tar.gz");
        QProcess compressor;
        compressor.setWorkingDirectory(dir.path());
        compressor.start(executable, {"a", "-tgzip", "--", compressed, "sample.tar"});
        QVERIFY(compressor.waitForFinished(30000));
        QCOMPARE(compressor.exitCode(), 0);
        window.openArchive(compressed);
        QTRY_COMPARE(table->rowCount(), 1);
        QCOMPARE(table->item(0, 0)->text(), QStringLiteral("sample.tar"));
        table->selectRow(0);
        auto *inside = window.findChild<QAction *>("openInsideAction");
        auto *back = window.findChild<QAction *>("backAction");
        QVERIFY(inside && back);
        QVERIFY(inside->isEnabled());
        inside->trigger();
        QTRY_VERIFY(table->rowCount() == 1 && table->item(0, 0) &&
                    table->item(0, 0)->text().normalized(QString::NormalizationForm_C) ==
                        sourceName);
        back->trigger();
        QTRY_COMPARE(table->item(0, 0)->text(), QStringLiteral("sample.tar"));
    }

    void opensRealNestedArchive() {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const QString source = dir.filePath(QStringLiteral("δ с пробелом.txt"));
        QFile file(source);
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.write("nested data");
        file.close();
        ArchiveBackend backend;
        const QString inner = dir.filePath(QStringLiteral("вложенный архив.zip"));
        const QString outer = dir.filePath("outer.7z");
        QSignalSpy finished(&backend, &ArchiveBackend::finished);
        backend.create(inner, {source});
        QVERIFY(finished.wait(30000));
        QVERIFY(finished.takeFirst().first().toBool());
        backend.create(outer, {inner});
        QVERIFY(finished.wait(30000));
        QVERIFY(finished.takeFirst().first().toBool());

        MainWindow window(&backend);
        window.show();
        window.openArchive(outer);
        auto *table = window.findChild<QTableWidget *>("archiveTable");
        auto *inside = window.findChild<QAction *>("openInsideAction");
        auto *back = window.findChild<QAction *>("backAction");
        QVERIFY(table && inside && back);
        QTRY_VERIFY(table->rowCount() == 1 && table->item(0, 0) &&
                    table->item(0, 0)->text().normalized(QString::NormalizationForm_C) ==
                        QStringLiteral("вложенный архив.zip"));
        table->selectRow(0);
        inside->trigger();
        QTRY_VERIFY(table->rowCount() == 1 && table->item(0, 0) &&
                    table->item(0, 0)->text().normalized(QString::NormalizationForm_C) ==
                        QStringLiteral("δ с пробелом.txt"));
        const QString output = dir.filePath("extracted");
        QVERIFY(QDir().mkpath(output));
        QFile existing(QDir(output).filePath(QFileInfo(source).fileName()));
        QVERIFY(existing.open(QIODevice::WriteOnly));
        QCOMPARE(existing.write("old data"), qint64(8));
        existing.close();
        QSignalSpy batchOpen(&backend, &ArchiveBackend::entriesReady);
        bool answered = false;
        QTimer answer;
        connect(&answer, &QTimer::timeout, &window, [&] {
            for (QWidget *widget : QApplication::topLevelWidgets()) {
                auto *dialog = dynamic_cast<COverwriteDialog *>(widget);
                if (!dialog || !dialog->isVisible())
                    continue;
                answered = true;
                dialog->findChild<QPushButton *>("overwriteYes")->click();
            }
        });
        answer.start(1);
        ExtractionOptions options;
        options.overwriteMode = ExtractionOptions::OverwriteMode::Ask;
        window.extractTo(output, false, {}, options);
        answer.stop();
        QVERIFY(answered);
        QCOMPARE(batchOpen.size(), 1);
        QVERIFY(existing.open(QIODevice::ReadOnly));
        QCOMPARE(existing.readAll(), QByteArray("nested data"));
        existing.close();
        QVERIFY(existing.open(QIODevice::WriteOnly | QIODevice::Truncate));
        QCOMPARE(existing.write("old data"), qint64(8));
        existing.close();
        answered = false;
        table->selectRow(0);
        answer.start(1);
        window.extractTo(output, true, {}, options);
        answer.stop();
        QVERIFY(answered);
        QCOMPARE(batchOpen.size(), 1); // Selected items use the bound source FM folder.
        QVERIFY(existing.open(QIODevice::ReadOnly));
        QCOMPARE(existing.readAll(), QByteArray("nested data"));
        existing.close();
        back->trigger();
        QCOMPARE(table->item(0, 0)->text().normalized(QString::NormalizationForm_C),
                 QStringLiteral("вложенный архив.zip"));
    }

    void opensNestedArchiveAndReturnsToParent() {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        NestedBackend backend;
        QFile outer(dir.filePath("outer.7z"));
        QVERIFY(outer.open(QIODevice::WriteOnly));
        outer.close();
        int externalOpens = 0;
        MainWindow window(&backend, [&](const QUrl &) {
            ++externalOpens;
            return true;
        });
        window.show();
        window.openArchive(dir.filePath("outer.7z"));
        auto *table = window.findChild<QTableWidget *>("archiveTable");
        auto *address = window.findChild<QLineEdit *>("archivePath");
        auto *inside = window.findChild<QAction *>("openInsideAction");
        auto *outside = window.findChild<QAction *>("openOutsideAction");
        auto *add = window.findChild<QAction *>("toolbarAddAction");
        auto *remove = window.findChild<QAction *>("deleteAction");
        auto *back = window.findChild<QAction *>("backAction");
        QVERIFY(table && address && inside && outside && add && remove && back);
        QTRY_COMPARE(table->rowCount(), 1);
        table->selectRow(0);
        QVERIFY(table->item(0, 0)->data(Qt::UserRole + 1).toBool());
        inside->trigger();
        QTRY_COMPARE(table->item(0, 0)->text(), QStringLiteral("вложенный архив.zip"));
        table->selectRow(0);
        QTRY_VERIFY(inside->isEnabled());
        QVERIFY(outside->isEnabled());
        inside->trigger();
        QTRY_COMPARE(backend.listed.size(), 2);
        QTRY_VERIFY(table->rowCount() == 1 && table->item(0, 0) &&
                    table->item(0, 0)->text() == "inside.txt");
        QCOMPARE(backend.extracted.size(), 1);
        QCOMPARE(QDir::fromNativeSeparators(backend.extracted.first()),
                 QStringLiteral("folder/вложенный архив.zip"));
        QVERIFY(QFileInfo::exists(backend.listed.last()));
        QVERIFY(address->text().endsWith("outer.7z/folder/вложенный архив.zip"));
        table->selectRow(0);
        QVERIFY(add->isEnabled());
        QVERIFY(remove->isEnabled());
        back->trigger();
        QCOMPARE(table->item(0, 0)->text(), QStringLiteral("вложенный архив.zip"));
        QVERIFY(!QFileInfo::exists(backend.listed.last()));
        table->selectRow(0);
        QVERIFY(outside->isEnabled());
        outside->trigger();
        QTRY_COMPARE(externalOpens, 1);
    }

    void cancelledNestedExtractionKeepsParent() {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        NestedBackend backend;
        backend.cancelExtraction = true;
        QFile outer(dir.filePath("outer.7z"));
        QVERIFY(outer.open(QIODevice::WriteOnly));
        outer.close();
        MainWindow window(&backend);
        window.openArchive(dir.filePath("outer.7z"));
        auto *table = window.findChild<QTableWidget *>("archiveTable");
        auto *inside = window.findChild<QAction *>("openInsideAction");
        QVERIFY(table && inside);
        QTRY_COMPARE(table->rowCount(), 1);
        emit table->cellDoubleClicked(0, 0);
        QTRY_COMPARE(table->item(0, 0)->text(), QStringLiteral("вложенный архив.zip"));
        table->selectRow(0);
        inside->trigger();
        QTRY_VERIFY(!backend.temporary.isEmpty());
        QTRY_VERIFY(!QFileInfo::exists(backend.temporary));
        QCOMPARE(backend.listed.size(), 1);
        QCOMPARE(table->item(0, 0)->text(), QStringLiteral("вложенный архив.zip"));
    }

    void failedNestedListingRestoresParent() {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        NestedBackend backend;
        backend.failNestedListing = true;
        QFile outer(dir.filePath("outer.7z"));
        QVERIFY(outer.open(QIODevice::WriteOnly));
        outer.close();
        MainWindow window(&backend);
        window.openArchive(dir.filePath("outer.7z"));
        auto *table = window.findChild<QTableWidget *>("archiveTable");
        auto *inside = window.findChild<QAction *>("openInsideAction");
        auto *address = window.findChild<QLineEdit *>("archivePath");
        QVERIFY(table && inside && address);
        QTRY_COMPARE(table->rowCount(), 1);
        emit table->cellDoubleClicked(0, 0);
        QTRY_COMPARE(table->item(0, 0)->text(), QStringLiteral("вложенный архив.zip"));
        table->selectRow(0);
        QTimer dismiss;
        connect(&dismiss, &QTimer::timeout, &window, [] {
            if (auto *box = qobject_cast<QMessageBox *>(QApplication::activeModalWidget()))
                box->accept();
            if (auto *progress =
                    dynamic_cast<CProgressDialog *>(QApplication::activeModalWidget())) {
                auto *close = progress->findChild<QPushButton *>("IDCANCEL");
                if (TestProgressDialog::IsCloseButton(close))
                    close->click();
            }
        });
        dismiss.start(1);
        inside->trigger();
        QTRY_COMPARE(backend.listed.size(), 2);
        QTRY_VERIFY(!QFileInfo::exists(backend.listed.last()));
        QCOMPARE(table->item(0, 0)->text(), QStringLiteral("вложенный архив.zip"));
        QVERIFY(address->text().endsWith("outer.7z / folder"));
    }
};

QTEST_MAIN(NestedArchiveTests)
#include "nested_archive_tests.moc"
