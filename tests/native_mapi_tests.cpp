#include "source_extract_fixture.h"
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QProcessEnvironment>
#include <QTemporaryDir>
#include <QtTest>
#include <windows.h>

#include <MAPI.h>

class NativeMAPITests final : public QObject {
    Q_OBJECT
    QString engine;
    static QJsonObject json(const QString &path) {
        return QJsonDocument::fromJson(SourceExtractFixture::readFile(path)).object();
    }

  private slots:
    void initTestCase() {
        engine = QStandardPaths::findExecutable("7z");
        QVERIFY(!engine.isEmpty());
    }
    void sourceEmail_data() {
        QTest::addColumn<QString>("provider");
        QTest::addColumn<bool>("remove");
        QTest::addColumn<bool>("cancel");
        QTest::addColumn<bool>("recipient");
        QTest::newRow("wide-keep") << QString(NATIVE_MAPI_WIDE) << false << false << true;
        QTest::newRow("wide-remove") << QString(NATIVE_MAPI_WIDE) << true << false << true;
        QTest::newRow("wide-cancel-remove") << QString(NATIVE_MAPI_WIDE) << true << true << false;
        QTest::newRow("ansi-remove") << QString(NATIVE_MAPI_ANSI) << true << false << true;
        QTest::newRow("missing-exports") << QString(NATIVE_MAPI_MISSING) << true << false << true;
    }
    void sourceEmail() {
        QFETCH(QString, provider);
        QFETCH(bool, remove);
        QFETCH(bool, cancel);
        QFETCH(bool, recipient);
        const bool ansi = provider == QString(NATIVE_MAPI_ANSI);
        const bool missing = provider == QString(NATIVE_MAPI_MISSING);
        QTemporaryDir fixture;
        QVERIFY(fixture.isValid());
        const QString name =
            ansi ? QString("input with spaces.bin") : QStringLiteral("вход 空 with spaces.bin");
        const QByteArray bytes("MAPI attachment payload");
        const QString source = fixture.filePath(name);
        QVERIFY(SourceExtractFixture::writeFile(source, bytes));
        const QString archive = fixture.filePath(
            ansi ? "mail with spaces.zip" : QStringLiteral("письмо 空 with spaces.zip"));
        const QString report = fixture.filePath("report.json");
        const QString mailCwd = fixture.filePath("mail-client-cwd");
        const QString temp = fixture.filePath("email-temp");
        QVERIFY(QDir().mkpath(mailCwd));
        QVERIFY(QDir().mkpath(temp));
        const QString address = recipient ? (ansi ? QString("fixture@example.invalid")
                                                  : QStringLiteral("тест@example.invalid"))
                                          : QString();
        QProcessEnvironment environment = QProcessEnvironment::systemEnvironment();
        environment.insert("SEVENZIP_MAPI_TEST_PROVIDER", provider);
        environment.insert("SEVENZIP_MAPI_TEST_REPORT", report);
        environment.insert("SEVENZIP_MAPI_TEST_CWD", mailCwd);
        environment.insert("SEVENZIP_MAPI_TEST_CANCEL", cancel ? "1" : "0");
        environment.insert("TEMP", QDir::toNativeSeparators(temp));
        environment.insert("TMP", QDir::toNativeSeparators(temp));
        QProcess host;
        host.setWorkingDirectory(fixture.path());
        host.setProcessEnvironment(environment);
        const QString mailSwitch = QString("-seml") + (remove ? "." : "") + address;
        host.start(QString(NATIVE_MAPI_HOST),
                   {"a", "-y", "-tzip", mailSwitch, "--", QDir::toNativeSeparators(archive),
                    QDir::toNativeSeparators(source)});
        QVERIFY(host.waitForStarted(5000));
        QVERIFY2(host.waitForFinished(30000), "source email host did not complete");
        QCOMPARE(host.exitStatus(), QProcess::NormalExit);
        QCOMPARE(host.exitCode(), missing ? 2 : 0);
        const auto owner = json(report + ".host.json");
        QVERIFY(!owner.isEmpty());
        QCOMPARE(owner["cwdAfter"].toString(), owner["cwdBefore"].toString());
        QCOMPARE(owner["provider"].toString(), QFileInfo(provider).canonicalFilePath());
        QCOMPARE(SourceExtractFixture::readFile(source), bytes);
        if (missing) {
            QVERIFY(!QFileInfo::exists(report));
        } else {
            const auto result = json(report);
            QCOMPARE(result["encoding"].toString(), ansi ? QString("ansi") : QString("wide"));
            for (const char *zero :
                 {"session", "owner", "reserved", "messageReserved", "messageFlags"})
                QCOMPARE(result[zero].toInt(), 0);
            QCOMPARE(result["flags"].toInt(), int(MAPI_DIALOG));
            QVERIFY(result["originatorNull"].toBool());
            QVERIFY(result["subjectNull"].toBool());
            QVERIFY(result["cwdChanged"].toBool());
            const auto recipients = result["recipients"].toArray();
            QCOMPARE(recipients.size(), recipient ? 1 : 0);
            if (recipient) {
                const auto to = recipients[0].toObject();
                QCOMPARE(to["class"].toInt(), int(MAPI_TO));
                QCOMPARE(to["address"].toString(), address);
                QVERIFY(to["nameNull"].toBool());
                QCOMPARE(to["entrySize"].toInt(), 0);
                QVERIFY(to["entryNull"].toBool());
            }
            const auto files = result["files"].toArray();
            QCOMPARE(files.size(), 1);
            const auto file = files[0].toObject();
            QCOMPARE(file["position"].toDouble(), double(0xffffffffu));
            QCOMPARE(file["reserved"].toInt(), 0);
            QCOMPARE(file["flags"].toInt(), 0);
            QVERIFY(file["fileTypeNull"].toBool());
            QVERIFY(file["readable"].toBool());
            QVERIFY(file["size"].toInt() > 0);
            QVERIFY(file["copied"].toBool());
            const QString attachment = QDir::fromNativeSeparators(file["path"].toString());
            QVERIFY(QDir::isAbsolutePath(attachment));
            QCOMPARE(file["name"].toString(), QFileInfo(archive).fileName());
            if (remove) {
                QVERIFY(attachment.startsWith(temp + "/"));
                QVERIFY(!QFileInfo::exists(attachment));
                QVERIFY(!QFileInfo::exists(QFileInfo(attachment).absolutePath()));
            } else {
                QCOMPARE(attachment, archive);
                QVERIFY(QFileInfo::exists(archive));
            }
            const QString copy = report + ".attachment-0.zip";
            QProcess verify;
            verify.start(engine, {"t", copy});
            QVERIFY(verify.waitForFinished(10000));
            QVERIFY2(verify.exitCode() == 0, verify.readAll().constData());
            const QString output = fixture.filePath("verify-output");
            verify.start(engine, {"x", "-y", "-o" + output, copy});
            QVERIFY(verify.waitForFinished(10000));
            QCOMPARE(verify.exitCode(), 0);
            QCOMPARE(SourceExtractFixture::readFile(QDir(output).filePath(name)), bytes);
        }
        if (remove)
            QVERIFY(!QFileInfo::exists(archive));
        QCOMPARE(QDir(temp).entryList(QDir::AllEntries | QDir::NoDotAndDotDot).size(), 0);
    }
};

QTEST_GUILESS_MAIN(NativeMAPITests)
#include "native_mapi_tests.moc"
