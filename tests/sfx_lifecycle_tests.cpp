#include "archivebackend.h"
#include "archivecodeccatalog.h"
#include "createarchivedialog.h"
#include <QFile>
#include <QRandomGenerator>
#include <QScopeGuard>
#include <QSignalSpy>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QTimer>
#include <QtTest>

namespace {
QByteArray read(const QString &path) {
    QFile file(path);
    return file.open(QIODevice::ReadOnly) ? file.readAll() : QByteArray();
}
bool write(const QString &path, const QByteArray &bytes) {
    QFile file(path);
    return file.open(QIODevice::WriteOnly) && file.write(bytes) == bytes.size();
}
} // namespace

class SfxLifecycleTests final : public QObject {
    Q_OBJECT
  private slots:
    void acceptedFormCreatesAndDeletes_data() {
        QTest::addColumn<bool>("volumes");
        QTest::addColumn<QString>("outcome");
        for (bool volumes : {false, true})
            for (const QString &outcome : {"success", "pause", "cancel-second-stage"})
                QTest::newRow(qPrintable(QString::number(volumes) + outcome)) << volumes << outcome;
        QTest::newRow("cancel-volume-publication") << true << QString("cancel-volume");
        QTest::newRow("pause-volume-worker") << true << QString("pause-volume");
    }
    void acceptedFormCreatesAndDeletes() {
#if !defined(SEVENZIP_CODEC_MODULE)
        QSKIP("Official native codec module unavailable; lifecycle remains unverified");
#else
        QFETCH(bool, volumes);
        QFETCH(QString, outcome);
        const QString sfx = QString::fromUtf8(SEVENZIP_SFX_FIXTURE);
        if (!QFileInfo::exists(sfx))
            QSKIP("Run scripts/build-sfx.sh for the official native module");
        QTemporaryDir fixture;
        QVERIFY(fixture.isValid());
        const QByteArray previousSfx = qgetenv("SEVENZIP_SFX_MODULE");
        const QByteArray previousSettings = qgetenv("SEVENZIP_SETTINGS_PATH");
        const auto restore = qScopeGuard([&] {
            if (previousSfx.isNull())
                qunsetenv("SEVENZIP_SFX_MODULE");
            else
                qputenv("SEVENZIP_SFX_MODULE", previousSfx);
            if (previousSettings.isNull())
                qunsetenv("SEVENZIP_SETTINGS_PATH");
            else
                qputenv("SEVENZIP_SETTINGS_PATH", previousSettings);
        });
        qputenv("SEVENZIP_SFX_MODULE", sfx.toUtf8());
        qputenv("SEVENZIP_SETTINGS_PATH", fixture.filePath("settings.ini").toUtf8());
        QByteArray bytes(2 * 1024 * 1024, Qt::Uninitialized);
        QRandomGenerator random(0x7a5f);
        for (int i = 0; i < bytes.size(); ++i)
            bytes[i] = char(random.generate() & 0xff);
        QVERIFY(QDir(fixture.path()).mkpath("first"));
        QVERIFY(QDir(fixture.path()).mkpath("second"));
        const QString first = fixture.filePath(QStringLiteral("first/данные 空 with spaces.bin"));
        const QString second = fixture.filePath("second/other.txt");
        QVERIFY(write(first, bytes));
        QVERIFY(write(second, "second source"));
        ArchiveBackend backend(nullptr, QStandardPaths::findExecutable("7zz"));
        ArchiveCodecCatalog probe(QString::fromUtf8(SEVENZIP_CODEC_MODULE), 0);
        QCOMPARE(probe.result(), S_OK);
        QCOMPARE(
            backend.loadCodecCatalog(QString::fromUtf8(SEVENZIP_CODEC_MODULE), probe.version()),
            S_OK);
        CreateArchiveDialog dialog(fixture.path(), {first, second}, nullptr,
                                   backend.codecCatalog());
        dialog.findChild<QComboBox *>("archiveFormat")->setCurrentText("7z");
        auto *level = dialog.findChild<QComboBox *>("compressionLevel");
        level->setCurrentIndex(outcome == "pause" ? level->findData(5) : 0);
        auto *pathMode = dialog.findChild<QComboBox *>("archivePathMode");
        pathMode->setCurrentIndex(pathMode->findData(int(NWildcard::k_RelatPath)));
        auto *sfxControl = dialog.findChild<QCheckBox *>("archiveSfx");
        QVERIFY(sfxControl && sfxControl->isEnabled());
        sfxControl->setChecked(true);
        dialog.findChild<QCheckBox *>("archiveDeleteAfterCompression")->setChecked(true);
#ifdef Q_OS_WIN
        dialog.findChild<QLineEdit *>("archiveName")->setText("accepted self extractor.exe");
#else
        dialog.findChild<QLineEdit *>("archiveName")->setText("accepted self extractor");
#endif
        if (volumes)
            dialog.findChild<QComboBox *>("archiveVolumeSize")->setEditText("1M");
        auto *buttons = dialog.findChild<QDialogButtonBox *>();
        QVERIFY(buttons);
        buttons->button(QDialogButtonBox::Ok)->click();
        QCOMPARE(dialog.result(), int(QDialog::Accepted));
        QVERIFY(dialog.compressionOptions().SFXMode);
        QVERIFY(dialog.compressionOptions().DeleteAfterCompressing);
        const QString target = dialog.target();
        QSignalSpy finished(&backend, &ArchiveBackend::finished);
        int stages = 0;
        bool sourcesPresent = true;
        HRESULT pauseResult = E_FAIL, resumeResult = E_FAIL;
        bool volumeHandled = false;
        connect(&backend, &ArchiveBackend::engineStarted, this, [&] {
            ++stages;
            sourcesPresent =
                sourcesPresent && QFileInfo::exists(first) && QFileInfo::exists(second);
            if (outcome == "cancel-second-stage" && stages == 2)
                backend.cancel();
            if (outcome == "pause" && stages == 1) {
                pauseResult = backend.setPaused(true);
                QTimer::singleShot(40, &backend, [&] { resumeResult = backend.setPaused(false); });
            }
        });
        connect(&backend, &ArchiveBackend::progress, this, [&](int, const QString &detail) {
            if (volumeHandled || (detail != "Creating archive volumes" &&
                                  detail != QStringLiteral("Создание архивных томов")))
                return;
            volumeHandled = true;
            if (outcome == "cancel-volume")
                backend.cancel();
            if (outcome == "pause-volume") {
                pauseResult = backend.setPaused(true);
                QTimer::singleShot(40, &backend, [&] { resumeResult = backend.setPaused(false); });
            }
        });
        const QByteArray originalModule = read(sfx);
        backend.create(target, dialog.sources(), dialog.password(), dialog.encryptHeaders(),
                       dialog.compressionOptions());
        QTRY_COMPARE_WITH_TIMEOUT(finished.size(), 1, 15000);
        QVERIFY(sourcesPresent);
        QCOMPARE(stages, 2);
        QCOMPARE(read(sfx), originalModule);
        if (outcome.startsWith("cancel-")) {
            QVERIFY(!finished[0][0].toBool());
            QCOMPARE(read(first), bytes);
            QCOMPARE(read(second), QByteArray("second source"));
            QVERIFY(!QFileInfo::exists(target));
            QVERIFY(QDir(fixture.path()).entryList({"*.7z.*"}, QDir::Files).isEmpty());
        } else {
            QVERIFY2(finished[0][0].toBool(), qPrintable(finished[0][1].toString()));
            if (outcome.startsWith("pause")) {
                QCOMPARE(pauseResult, S_OK);
                QCOMPARE(resumeResult, S_OK);
            }
            QVERIFY(!QFileInfo::exists(first));
            QVERIFY(!QFileInfo::exists(second));
            QVERIFY(QFileInfo(target).isExecutable());
            QProcess extractor;
            const QString output = fixture.filePath("extracted");
            QStringList arguments{"-y", "-o" + output};
#ifndef Q_OS_WIN
            arguments.prepend("x");
#endif
            extractor.start(target, arguments);
            QVERIFY(extractor.waitForFinished(10000));
            QCOMPARE(extractor.exitCode(), 0);
            QCOMPARE(read(QDir(output).filePath(QFileInfo(first).fileName())), bytes);
            QCOMPARE(read(QDir(output).filePath(QFileInfo(second).fileName())),
                     QByteArray("second source"));
        }
        if (outcome.endsWith("volume"))
            QVERIFY(volumeHandled);
        QVERIFY(
            QDir(fixture.path()).entryList({".sevenzip-*"}, QDir::Files | QDir::Hidden).isEmpty());
#endif
    }
};
QTEST_MAIN(SfxLifecycleTests)
#include "sfx_lifecycle_tests.moc"
