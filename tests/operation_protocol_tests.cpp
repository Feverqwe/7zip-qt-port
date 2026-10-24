#include "qtoperationprotocol.h"
#include <QBuffer>
#include <QDir>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTemporaryDir>
#include <QtEndian>
#include <QtTest>

class OperationProtocolTests final : public QObject {
    Q_OBJECT
    using Kind = QtOperationMessage::Kind;
    using State = QtOperationMessageReader::State;
    static QByteArray Frame(const QJsonObject &object) {
        const QByteArray body = QJsonDocument(object).toJson(QJsonDocument::Compact);
        QByteArray frame(4, 0);
        qToBigEndian(quint32(body.size()), frame.data());
        return frame + body;
    }

  private slots:
    void fragmentedLargeSelectionRemainsLiteral() {
        QTemporaryDir root;
        QVERIFY(root.isValid());
        QtOperationMessage job;
        job.Type = Kind::Job;
        for (int i = 0; i < 10000; ++i)
            job.Paths.append(
                root.filePath(QStringLiteral("空 файл %1 \"quotes\" [literal].zip").arg(i)));
        QByteArray encoded;
        QString error;
        QVERIFY2(EncodeOperationMessage(job, encoded, error), qPrintable(error));
        QByteArray available;
        QBuffer input(&available);
        QVERIFY(input.open(QIODevice::ReadOnly));
        QtOperationMessageReader reader;
        QtOperationMessage decoded;
        const int fragments[] = {1, 2, 3, 17, 4096};
        qsizetype offset = 0;
        int fragment = 0;
        while (offset < encoded.size()) {
            const auto count = qMin(qsizetype(fragments[fragment++ % 5]), encoded.size() - offset);
            available.append(encoded.mid(offset, count));
            offset += count;
            QCOMPARE(reader.read(input, decoded, error),
                     offset == encoded.size() ? State::Message : State::Pending);
        }
        QCOMPARE(decoded.Type, Kind::Job);
        QCOMPARE(decoded.Paths, job.Paths);
        QCOMPARE(reader.read(input, decoded, error), State::Pending);
    }
    void coalescedMessagesKeepTheirBoundaries() {
        QByteArray bytes;
        QString error;
        QtOperationMessage message;
        for (const auto kind : {Kind::Ready, Kind::Accepted, Kind::Result}) {
            message.Type = kind;
            message.Result = {QtOperationResult::Outcome::Cancelled, 255, {}};
            QByteArray frame;
            QVERIFY(EncodeOperationMessage(message, frame, error));
            bytes.append(frame);
        }
        QBuffer input(&bytes);
        QVERIFY(input.open(QIODevice::ReadOnly));
        QtOperationMessageReader reader;
        for (const auto kind : {Kind::Ready, Kind::Accepted, Kind::Result}) {
            QCOMPARE(reader.read(input, message, error), State::Message);
            QCOMPARE(message.Type, kind);
        }
        QCOMPARE(message.Result.Status, QtOperationResult::Outcome::Cancelled);
        QCOMPARE(message.Result.ExitCode, 255);
        QCOMPARE(reader.read(input, message, error), State::Pending);
    }
    void malformedMessages_data() {
        QTest::addColumn<QByteArray>("bytes");
        QTest::newRow("unsupported version") << Frame({{"version", 2}, {"type", "ready"}});
        QTest::newRow("fractional version") << Frame({{"version", 1.5}, {"type", "ready"}});
        QTest::newRow("unknown message") << Frame({{"version", 1}, {"type", "execute"}});
        QTest::newRow("unsupported command") << Frame({{"version", 1},
                                                       {"type", "job"},
                                                       {"command", "update"},
                                                       {"paths", QJsonArray({"/file"})}});
        QTest::newRow("contradictory success")
            << Frame({{"version", 1}, {"type", "result"}, {"outcome", "success"}, {"exitCode", 2}});
        QTest::newRow("fractional exit") << Frame(
            {{"version", 1}, {"type", "result"}, {"outcome", "failure"}, {"exitCode", 2.5}});
        QTest::newRow("unsupported job option") << Frame({{"version", 1},
                                                          {"type", "job"},
                                                          {"command", "test"},
                                                          {"paths", QJsonArray({"/file"})},
                                                          {"unexpected", true}});
        QTest::newRow("empty frame") << QByteArray(4, 0);
        QTest::newRow("oversized frame") << QByteArray(4, char(0xff));
    }
    void malformedMessages() {
        QFETCH(QByteArray, bytes);
        QBuffer input(&bytes);
        QVERIFY(input.open(QIODevice::ReadOnly));
        QtOperationMessageReader reader;
        QtOperationMessage message;
        QString error;
        QCOMPARE(reader.read(input, message, error), State::Error);
        QVERIFY(!error.isEmpty());
    }
    void unavailableSelectionCannotBeEncoded() {
        QtOperationMessage job;
        job.Type = Kind::Job;
        QByteArray frame;
        QString error;
        QVERIFY(!EncodeOperationMessage(job, frame, error));
        job.Paths = {"relative/file.zip"};
        QVERIFY(!EncodeOperationMessage(job, frame, error));
        QTemporaryDir root;
        QVERIFY(root.isValid());
        job.Paths = {root.filePath("archive.zip") + QChar::Null + "hidden"};
        QVERIFY(!EncodeOperationMessage(job, frame, error));
    }
    void panelHashSelectionRejectsTraversalAndKeepsLiteralNames() {
        QTemporaryDir root;
        QVERIFY(root.isValid());
        QtOperationMessage job;
        job.Type = Kind::Job;
        job.Operation = QtOperationMessage::Command::Hash;
        job.HashMethod = "SHA256";
        job.HashBasePrefix = root.path() + '/';
        QByteArray frame;
        QString error;
        QStringList invalidPaths{"../outside", root.filePath("absolute"), "sub/../../outside"};
#ifdef Q_OS_WIN
        invalidPaths.append({"..\\outside", "sub\\..\\outside"});
#endif
        for (const QString &path : invalidPaths) {
            job.Paths = {path};
            QVERIFY(!EncodeOperationMessage(job, frame, error));
        }
        job.Paths = {QStringLiteral("nested/literal [*] 空.txt")};
        job.HashEnterToDirs = false;
        QVERIFY(EncodeOperationMessage(job, frame, error));
        QBuffer input(&frame);
        QVERIFY(input.open(QIODevice::ReadOnly));
        QtOperationMessage decoded;
        QtOperationMessageReader reader;
        QCOMPARE(reader.read(input, decoded, error), State::Message);
        QCOMPARE(decoded.Paths, job.Paths);
        QCOMPARE(decoded.HashBasePrefix, job.HashBasePrefix);
        QVERIFY(!decoded.HashEnterToDirs);
    }
    void extractionRequiresAbsoluteDestinationAndSupportedZone() {
        QTemporaryDir root;
        QVERIFY(root.isValid());
        QtOperationMessage job;
        job.Type = Kind::Job;
        job.Operation = QtOperationMessage::Command::Extract;
        job.Paths = {root.filePath("archive.zip")};
        job.Destination = "relative output";
        QByteArray frame;
        QString error;
        QVERIFY(!EncodeOperationMessage(job, frame, error));
        job.Destination = root.filePath("output");
        job.WriteZone = 3;
        QVERIFY(!EncodeOperationMessage(job, frame, error));
        job.WriteZone = quint32(-1);
        QVERIFY(EncodeOperationMessage(job, frame, error));
    }
};

QTEST_GUILESS_MAIN(OperationProtocolTests)
#include "operation_protocol_tests.moc"
