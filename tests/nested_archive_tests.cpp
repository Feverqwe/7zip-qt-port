#include "mainwindow.h"

#include <QAction>
#include <QFile>
#include <QLineEdit>
#include <QMessageBox>
#include <QProcess>
#include <QSignalSpy>
#include <QStandardPaths>
#include <QTableWidget>
#include <QTemporaryDir>
#include <QTimer>
#include <QtTest>

class NestedBackend : public ArchiveBackend {
  public:
    NestedBackend() : ArchiveBackend(nullptr, "/missing/7zz") {}

    void list(const QString &archive, const QString & = {}) override {
        listed << archive;
        QTimer::singleShot(0, this, [this, archive] {
            if (archive == listed.first())
                emit entriesReady(
                    {ArchiveEntry{"folder/вложенный архив.zip", 4, 4, {}, false, false}});
            else if (!failNestedListing)
                emit entriesReady({ArchiveEntry{"inside.txt", 3, 3, {}, false, false}});
            const bool success = archive == listed.first() || !failNestedListing;
            emit finished(success, success ? "Архив открыт." : "Неверный архив.");
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
        auto *add = window.findChild<QAction *>("addArchiveAction");
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
        QVERIFY(!edit->isEnabled());

        const QString destination = dir.filePath("output");
        QVERIFY(QDir().mkdir(destination));
        QSignalSpy finished(&backend, &ArchiveBackend::finished);
        window.extractTo(destination);
        QVERIFY(finished.wait(30000));
        QVERIFY(finished.takeFirst().first().toBool());
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
        back->trigger();
        QCOMPARE(table->item(0, 0)->text().normalized(QString::NormalizationForm_C),
                 QStringLiteral("вложенный архив.zip"));
    }

    void opensNestedArchiveAndReturnsToParent() {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        NestedBackend backend;
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
        auto *add = window.findChild<QAction *>("addArchiveAction");
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
        QCOMPARE(backend.extracted, QStringList({"folder/вложенный архив.zip"}));
        QVERIFY(QFileInfo::exists(backend.listed.last()));
        QVERIFY(address->text().contains("outer.7z / folder/вложенный архив.zip"));
        table->selectRow(0);
        QVERIFY(!add->isEnabled());
        QVERIFY(!remove->isEnabled());
        back->trigger();
        QCOMPARE(table->item(0, 0)->text(), QStringLiteral("вложенный архив.zip"));
        QVERIFY(!QFileInfo::exists(backend.listed.last()));
        table->selectRow(0);
        outside->trigger();
        QTRY_COMPARE(externalOpens, 1);
    }

    void cancelledNestedExtractionKeepsParent() {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        NestedBackend backend;
        backend.cancelExtraction = true;
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
        QTimer::singleShot(100, &window, [&window] {
            if (auto *box = window.findChild<QMessageBox *>())
                box->accept();
        });
        inside->trigger();
        QTRY_COMPARE(backend.listed.size(), 2);
        QTRY_VERIFY(!QFileInfo::exists(backend.listed.last()));
        QCOMPARE(table->item(0, 0)->text(), QStringLiteral("вложенный архив.zip"));
        QVERIFY(address->text().endsWith("outer.7z / folder"));
    }
};

QTEST_MAIN(NestedArchiveTests)
#include "nested_archive_tests.moc"
