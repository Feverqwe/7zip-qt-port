#include "CPP/7zip/UI/FileManager/ViewSettings.h"
#include "source_extract_fixture.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QScopeGuard>
#include <QStandardPaths>
#include <QtTest>

namespace {
bool CreateArchive(const QString &engine, const QString &directory, const QString &archive,
                   const QStringList &names) {
    QProcess process;
    process.setWorkingDirectory(directory);
    process.start(engine, QStringList{"a", "-t7z", "-m0=Copy", "--", archive} + names);
    return process.waitForFinished(10000) && process.exitCode() == 0;
}

QString PrepareSettings(const QTemporaryDir &fixture, unsigned pane) {
    const QString settings = fixture.filePath("RAM startup.ini");
    CWindowInfo info{};
    info.rect.right = 980;
    info.rect.bottom = 620;
    info.numPanels = 2;
    info.currentPanel = pane;
    info.splitterPos = 490;
    info.Save(settings);
    SavePanelPath(1 - pane, ToUString(fixture.path()), settings);
    return settings;
}

QProcessEnvironment ProbeEnvironment(const QString &settings, const QString &inner,
                                     const QString &output, unsigned pane, bool defined,
                                     quint64 initial, quint64 later) {
    auto environment = QProcessEnvironment::systemEnvironment();
    environment.insert("QT_QPA_PLATFORM", "offscreen");
    environment.insert("SEVENZIP_SETTINGS_PATH", settings);
    environment.insert("SEVENZIP_RAM_START", QString::number(initial));
    environment.insert("SEVENZIP_RAM_LATER", QString::number(later));
    environment.insert("SEVENZIP_RAM_DEFINED_START", defined ? "1" : "0");
    environment.insert("SEVENZIP_RAM_DEFINED_LATER", "1");
    environment.insert("SEVENZIP_RAM_PANE", QString::number(pane));
    environment.insert("SEVENZIP_RAM_MEMBER", QFileInfo(inner).fileName());
    environment.insert("SEVENZIP_RAM_OUTPUT", output);
    return environment;
}
} // namespace

class FileManagerRamTests final : public QObject {
    Q_OBJECT
  private slots:
    void nestedOpenWithUnavailableOrChangingRam_data() {
        QTest::addColumn<unsigned>("pane");
        QTest::addColumn<bool>("defined");
        QTest::addColumn<quint64>("initial");
        QTest::addColumn<quint64>("later");
        QTest::addColumn<bool>("broken");
        for (unsigned pane : {0u, 1u}) {
            const auto row = [pane](const char *name, bool defined, quint64 initial, quint64 later,
                                    bool broken) {
                QTest::newRow(qPrintable(QString("%1-%2").arg(pane).arg(name)))
                    << pane << defined << initial << later << broken;
            };
            row("cached-zero", true, 0, 1u << 26, false);
            row("cached-ram", true, 1u << 26, 0, false);
            row("failed-query-fallback", false, 0, 0, false);
            row("broken-inner-retains-parent", true, 0, 1u << 26, true);
        }
    }
    void nestedOpenWithUnavailableOrChangingRam() {
        QFETCH(unsigned, pane);
        QFETCH(bool, defined);
        QFETCH(quint64, initial);
        QFETCH(quint64, later);
        QFETCH(bool, broken);
        QTemporaryDir fixture;
        QVERIFY(fixture.isValid());
        const QString engine = QStandardPaths::findExecutable("7zz");
        QVERIFY(!engine.isEmpty());
        const QString first = QStringLiteral("первый 空 with spaces.txt");
        const QString second = QStringLiteral("второй 空 with spaces.txt");
        const QByteArray firstBytes("first protected native bytes");
        const QByteArray secondBytes("second protected native bytes");
        QVERIFY(SourceExtractFixture::writeFile(fixture.filePath(first), firstBytes));
        QVERIFY(SourceExtractFixture::writeFile(fixture.filePath(second), secondBytes));
        const QString inner = fixture.filePath(QStringLiteral("вложенный 空 with spaces.7z"));
        const QString outer = fixture.filePath(QStringLiteral("внешний 空 with spaces.7z"));
        if (broken)
            QVERIFY(SourceExtractFixture::writeFile(inner, "invalid nested archive"));
        else
            QVERIFY(CreateArchive(engine, fixture.path(), inner, {first, second}));
        QVERIFY(CreateArchive(engine, fixture.path(), outer, {QFileInfo(inner).fileName()}));
        const QByteArray innerBytes = SourceExtractFixture::readFile(inner);
        const QByteArray outerBytes = SourceExtractFixture::readFile(outer);
        const QString settings = PrepareSettings(fixture, pane);
        const QString output = fixture.filePath("source copy output 空");
        QVERIFY(QDir().mkpath(output));
        QProcess child;
        child.setProcessEnvironment(
            ProbeEnvironment(settings, inner, output, pane, defined, initial, later));
        const auto stop = qScopeGuard([&] {
            if (child.state() != QProcess::NotRunning) {
                child.kill();
                child.waitForFinished(5000);
            }
        });
        child.start(QString::fromUtf8(SEVENZIP_RAM_PROBE), {outer, "-t7z"});
        QVERIFY(child.waitForStarted(5000));
        QVERIFY(child.waitForFinished(35000));
        QCOMPARE(child.exitStatus(), QProcess::NormalExit);
        QCOMPARE(child.exitCode(), 0);
        const auto report = QJsonDocument::fromJson(child.readAllStandardOutput()).object();
        QVERIFY2(report.value("bound").toBool(), child.readAllStandardError().constData());
        QVERIFY(!report.value("timeout").toBool());
        QCOMPARE(report.value("opened").toBool(), !broken);
        QVERIFY(report.value("restored").toBool());
        if (!broken) {
            QVERIFY(report.value("copied").toBool());
            QCOMPARE(SourceExtractFixture::readFile(QDir(output).filePath(first)), firstBytes);
            QCOMPARE(SourceExtractFixture::readFile(QDir(output).filePath(second)), secondBytes);
        } else
            QVERIFY(QDir(output).entryList(QDir::Files).isEmpty());
        QCOMPARE(SourceExtractFixture::readFile(inner), innerBytes);
        QCOMPARE(SourceExtractFixture::readFile(outer), outerBytes);
        QCOMPARE(SourceExtractFixture::readFile(fixture.filePath(first)), firstBytes);
        QCOMPARE(SourceExtractFixture::readFile(fixture.filePath(second)), secondBytes);
    }
};

QTEST_MAIN(FileManagerRamTests)
#include "file_manager_ram_tests.moc"
