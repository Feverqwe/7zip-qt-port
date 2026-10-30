#include "archive/backend/archivebackend.h"
#include "archive/callbacks/IFileExtractCallback.h"
#include "archive/support/source_creation_fixture.h"
#include "fileops/transfer/filetransferoperation.h"
#include <QDir>
#include <QFile>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QtTest>

class ArchiveWorkflowTests final : public QObject {
    Q_OBJECT
    static QByteArray read(const QString &path) {
        QFile file(path);
        return file.open(QIODevice::ReadOnly) ? file.readAll() : QByteArray();
    }
    static bool write(const QString &path, const QByteArray &bytes) {
        QFile file(path);
        return file.open(QIODevice::WriteOnly) && file.write(bytes) == bytes.size();
    }
    static void finish(ArchiveBackend &backend, QSignalSpy &results, bool expected = true) {
        QTRY_COMPARE_WITH_TIMEOUT(results.size(), 1, 30000);
        const auto result = results.takeFirst();
        QCOMPARE(result.first().toBool(), expected);
        QVERIFY(!backend.busy());
    }
    static void transfer(const QString &source, const QString &destination, int answer,
                         bool expected) {
        FileTransferOperation operation({source}, destination, true);
        QSignalSpy results(&operation, &FileTransferOperation::result);
        connect(
            &operation, &FileTransferOperation::overwriteRequested, &operation,
            [&operation, answer] { operation.submitOverwriteAnswer(answer); },
            Qt::DirectConnection);
        operation.start();
        QVERIFY(operation.wait(30000));
        QTRY_COMPARE(results.size(), 1);
        QCOMPARE(results.first().first().toBool(), expected);
        QCOMPARE(operation.resultKind(), expected ? FileTransferOperation::ResultKind::Success
                                                  : FileTransferOperation::ResultKind::Canceled);
    }

  private slots:
    void createUpdateExtractAndTransfer_data() {
        QTest::addColumn<QString>("format");
        QTest::newRow("7z") << QString("7z");
        QTest::newRow("zip") << QString("zip");
    }
    void createUpdateExtractAndTransfer() {
        QFETCH(QString, format);
        QTemporaryDir fixture;
        QVERIFY(fixture.isValid());
        const QString name = QStringLiteral("данные 空 with spaces.txt");
        const QString unchanged = QStringLiteral("неизменённый δ.txt");
        const QString source = fixture.filePath(name);
        const QString sentinel = fixture.filePath(unchanged);
        const QString archive = fixture.filePath(QStringLiteral("архив 空.") + format);
        QVERIFY(write(source, "before editing"));
        QVERIFY(write(sentinel, "retained member"));
        ArchiveBackend backend;
        QSignalSpy results(&backend, &ArchiveBackend::finished);
        QSignalSpy listed(&backend, &ArchiveBackend::entriesReady);
        QCOMPARE(SourceCreationFixture::Create(backend, archive, {source, sentinel}), S_OK);
        backend.list(archive);
        finish(backend, results);
        QCOMPARE(listed.size(), 1);
        const auto entries = qvariant_cast<QList<ArchiveEntry>>(listed.takeFirst().first());
        QCOMPARE(entries.size(), 2);
        QString member;
        QString retained;
        for (const auto &entry : entries) {
            if (entry.path.normalized(QString::NormalizationForm_C) == name)
                member = entry.path;
            if (entry.path.normalized(QString::NormalizationForm_C) == unchanged)
                retained = entry.path;
        }
        QVERIFY(!member.isEmpty());
        QVERIFY(!retained.isEmpty());

        QTemporaryDir edited;
        QVERIFY(edited.isValid());
        backend.extract(archive, edited.path(), {member});
        finish(backend, results);
        const QString editedPath = edited.filePath(member);
        QCOMPARE(read(editedPath), QByteArray("before editing"));
        QVERIFY(!QFileInfo::exists(edited.filePath(unchanged)));
        QVERIFY(write(editedPath, "after editing: a different size"));
        QCOMPARE(SourceCreationFixture::Create(backend, archive, {editedPath}), S_OK);
        backend.list(archive);
        finish(backend, results);
        QCOMPARE(listed.size(), 1);
        const auto updated = qvariant_cast<QList<ArchiveEntry>>(listed.takeFirst().first());
        QCOMPARE(updated.size(), 2);
        QStringList updatedNames;
        for (const auto &entry : updated) {
            updatedNames.append(entry.path);
            if (entry.path == member)
                QCOMPARE(entry.size, qint64(read(editedPath).size()));
        }
        QVERIFY(updatedNames.contains(member));
        QVERIFY(updatedNames.contains(retained));
        backend.test(archive);
        finish(backend, results);
        verifyTransfer(backend, results, archive, member, retained, read(editedPath));
        QCOMPARE(read(source), QByteArray("before editing"));
        QCOMPARE(read(sentinel), QByteArray("retained member"));
        QVERIFY(QDir(fixture.path())
                    .entryList({".sevenzip-*"}, QDir::Files | QDir::Dirs | QDir::Hidden)
                    .isEmpty());
    }

  private:
    static void verifyTransfer(ArchiveBackend &backend, QSignalSpy &results, const QString &archive,
                               const QString &member, const QString &unchanged,
                               const QByteArray &bytes) {
        QTemporaryDir extracted;
        QTemporaryDir destination;
        QVERIFY(extracted.isValid() && destination.isValid());
        backend.extract(archive, extracted.path());
        finish(backend, results);
        const QByteArray archiveBytes = read(archive);
        const QString extractedPath = extracted.filePath(member);
        const QString target = destination.filePath(member);
        QCOMPARE(read(extractedPath), bytes);
        QCOMPARE(read(extracted.filePath(unchanged)), QByteArray("retained member"));
        QVERIFY(write(target, "existing destination"));
        transfer(extractedPath, destination.path(), NOverwriteAnswer::kCancel, false);
        QCOMPARE(read(extractedPath), bytes);
        QCOMPARE(read(target), QByteArray("existing destination"));
        transfer(extractedPath, destination.path(), NOverwriteAnswer::kYes, true);
        QVERIFY(!QFileInfo::exists(extractedPath));
        QCOMPARE(read(target), bytes);
        QCOMPARE(read(extracted.filePath(unchanged)), QByteArray("retained member"));
        QCOMPARE(read(archive), archiveBytes);
        backend.list(archive);
        finish(backend, results);
        backend.test(archive);
        finish(backend, results);
        QVERIFY(QDir(destination.path())
                    .entryList({".sevenzip-*"}, QDir::Files | QDir::Dirs | QDir::Hidden)
                    .isEmpty());
    }
};

QTEST_MAIN(ArchiveWorkflowTests)
#include "archive_workflow_tests.moc"
