#include "CPP/7zip/UI/FileManager/LangUtils.h"
#include "CPP/7zip/UI/FileManager/RegistryUtils.h"
#include "archivebackend.h"
#include "createarchivedialog.h"
#include "layout_inspection.h"
#include <QFile>
#include <QMessageBox>
#include <QProcess>
#include <QScrollArea>
#include <QSettings>
#include <QSignalSpy>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QTimer>
#include <QtTest>

namespace {
QString engine() { return QStandardPaths::findExecutable("7zz"); }
bool writeFile(const QString &path, const QByteArray &bytes) {
    QDir().mkpath(QFileInfo(path).absolutePath());
    QFile file(path);
    return file.open(QIODevice::WriteOnly) && file.write(bytes) == bytes.size();
}
QByteArray readFile(const QString &path) {
    QFile file(path);
    return file.open(QIODevice::ReadOnly) ? file.readAll() : QByteArray();
}
bool runEngine(const QStringList &args) {
    QProcess process;
    process.start(engine(), args);
    return process.waitForFinished(30000) && process.exitCode() == 0 &&
           process.exitStatus() == QProcess::NormalExit;
}
} // namespace

class ArchiveVolumeCreationTests final : public QObject {
    Q_OBJECT
    QTemporaryDir preferences_;
    QByteArray previousSettings_;
    QString settings_;
  private slots:
    void initTestCase() {
        QVERIFY(!engine().isEmpty());
        QVERIFY(preferences_.isValid());
        previousSettings_ = qgetenv("SEVENZIP_SETTINGS_PATH");
        settings_ = preferences_.filePath("options.ini");
        qputenv("SEVENZIP_SETTINGS_PATH", settings_.toUtf8());
    }
    void init() {
        QFile::remove(settings_);
        SaveRegLang(UString("-"), settings_);
        ReloadLang(settings_);
    }
    void cleanup() {
        SaveRegLang(UString("-"), settings_);
        ReloadLang(settings_);
    }
    void cleanupTestCase() {
        if (previousSettings_.isNull())
            qunsetenv("SEVENZIP_SETTINGS_PATH");
        else
            qputenv("SEVENZIP_SETTINGS_PATH", previousSettings_);
    }
    void createVariableVolumes_data() {
        QTest::addColumn<QString>("format");
        QTest::addColumn<bool>("protectedArchive");
        QTest::addColumn<bool>("russian");
        for (bool russian : {false, true})
            for (const QString &format : {QString("zip"), QString("7z")}) {
                const QString suffix = russian ? "-ru" : "-en";
                QTest::newRow(qPrintable(format + suffix)) << format << false << russian;
                QTest::newRow(qPrintable(format + "-protected" + suffix))
                    << format << true << russian;
            }
    }
    void createVariableVolumes() {
        QFETCH(QString, format);
        QFETCH(bool, protectedArchive);
        QFETCH(bool, russian);
        SaveRegLang(UString(russian ? "ru" : "-"), settings_);
        ReloadLang(settings_);
        QTemporaryDir fixture;
        QVERIFY(fixture.isValid());
        const QString first = fixture.filePath("source-a/first.bin");
        const QString second = fixture.filePath(QStringLiteral("source-b/имя с пробелом.bin"));
        const QByteArray a(240000, 'a'), b(200000, 'b');
        QVERIFY(writeFile(first, a));
        QVERIFY(writeFile(second, b));
        const QString target = fixture.filePath(QStringLiteral("архив с пробелом.") + format);
        CompressionOptions options;
        options.level = 0;
        options.VolumeSizes.Add(64 << 10);
        options.VolumeSizes.Add(128 << 10);
        if (format == "zip" && protectedArchive)
            options.EncryptionMethod = "AES256";
        const QString password = protectedArchive ? QStringLiteral("fixture password") : QString();
        ArchiveBackend backend(nullptr, engine());
        QSignalSpy finished(&backend, &ArchiveBackend::finished);
        backend.create(target, {first, second}, password, format == "7z", options);
        QTRY_COMPARE_WITH_TIMEOUT(finished.count(), 1, 30000);
        const auto result = finished.takeFirst();
        QVERIFY2(result.first().toBool(), qPrintable(result.at(1).toString()));
        QCOMPARE(result.at(1).toString(), russian ? QStringLiteral("Архивные тома созданы.")
                                                  : QString("Archive volumes created."));
        QVERIFY(!backend.busy());
        QVERIFY(!QFileInfo::exists(target));
        const QStringList volumes =
            fixture.path().isEmpty()
                ? QStringList()
                : QDir(fixture.path())
                      .entryList({QFileInfo(target).fileName() + ".*"}, QDir::Files, QDir::Name);
        QVERIFY(volumes.size() >= 4);
        for (int i = 0; i < volumes.size(); ++i) {
            QCOMPARE(volumes[i], QFileInfo(target).fileName() + '.' +
                                     QString::number(i + 1).rightJustified(3, '0'));
            const qint64 size = QFileInfo(fixture.filePath(volumes[i])).size();
            QVERIFY(size > 0);
            if (i + 1 != volumes.size())
                QCOMPARE(size, qint64(i == 0 ? 64 << 10 : 128 << 10));
            else
                QVERIFY(size <= 128 << 10);
        }
        QStringList passwordArgs;
        if (protectedArchive)
            passwordArgs << "-p" + password;
        if (protectedArchive) {
            QVERIFY(!runEngine({"t", "-y", "-pwrong", "--", target + ".001"}));
            QProcess listing;
            listing.start(engine(), QStringList{"l", "-slt"} + passwordArgs +
                                        QStringList{"--", target + ".001"});
            QVERIFY(listing.waitForFinished(30000));
            QCOMPARE(listing.exitCode(), 0);
            QCOMPARE(listing.readAllStandardOutput().count("Encrypted = +"), 2);
        }
        QVERIFY(
            runEngine(QStringList{"t", "-y"} + passwordArgs + QStringList{"--", target + ".001"}));
        const QString extracted = fixture.filePath("extracted");
        QVERIFY(runEngine(QStringList{"x", "-y", "-o" + extracted} + passwordArgs +
                          QStringList{"--", target + ".001"}));
        QCOMPARE(readFile(QDir(extracted).filePath(QFileInfo(first).fileName())), a);
        QCOMPARE(readFile(QDir(extracted).filePath(QFileInfo(second).fileName())), b);
        QCOMPARE(readFile(first), a);
        QCOMPARE(readFile(second), b);
        QVERIFY(
            QDir(fixture.path())
                .entryList({".sevenzip-*"}, QDir::AllEntries | QDir::Hidden | QDir::NoDotAndDotDot)
                .isEmpty());
    }
    void occupiedAndCancelledStagesLeaveNoOutputs_data() {
        QTest::addColumn<int>("mode");
        QTest::addColumn<bool>("russian");
        const char *names[] = {"occupied-preflight", "occupied-after-compression",
                               "cancel-compression", "cancel-volume-worker",
                               "pause-cancel-and-join"};
        for (bool russian : {false, true})
            for (int mode = 0; mode < 5; ++mode)
                QTest::newRow(
                    qPrintable(QString::fromLatin1(names[mode]) + (russian ? "-ru" : "-en")))
                    << mode << russian;
    }
    void invalidSizesAndMissingEngineProduceNoVolumes() {
        QTemporaryDir fixture;
        QVERIFY(fixture.isValid());
        const QString source = fixture.filePath("source.bin");
        QVERIFY(writeFile(source, "source"));
        const QString target = fixture.filePath("output.7z");
        ArchiveBackend backend(nullptr, fixture.filePath("missing-7zz"));
        QSignalSpy finished(&backend, &ArchiveBackend::finished);
        CompressionOptions options;
        options.VolumeSizes.Add(0);
        backend.create(target, {source}, {}, false, options);
        QCOMPARE(finished.count(), 1);
        QVERIFY(!finished.takeFirst().first().toBool());
        options.VolumeSizes[0] = 65536;
        backend.create(target, {source}, {}, false, options);
        QTRY_COMPARE_WITH_TIMEOUT(finished.count(), 1, 10000);
        QVERIFY(!finished.takeFirst().first().toBool());
        QVERIFY(!backend.busy());
        QCOMPARE(readFile(source), QByteArray("source"));
        QCOMPARE(QDir(fixture.path()).entryList(QDir::Files), QStringList{"source.bin"});
        QVERIFY(
            QDir(fixture.path())
                .entryList({".sevenzip-*"}, QDir::AllEntries | QDir::Hidden | QDir::NoDotAndDotDot)
                .isEmpty());
    }
    void occupiedAndCancelledStagesLeaveNoOutputs() {
        QFETCH(int, mode);
        QFETCH(bool, russian);
        SaveRegLang(UString(russian ? "ru" : "-"), settings_);
        ReloadLang(settings_);
        QTemporaryDir fixture;
        QVERIFY(fixture.isValid());
        const QString source = fixture.filePath(QStringLiteral("источник с пробелом.bin"));
        const QByteArray bytes(500000, 'x');
        QVERIFY(writeFile(source, bytes));
        const QString target = fixture.filePath("output.7z");
        const QString occupied = target + ".002";
        if (mode == 0)
            QVERIFY(writeFile(occupied, "occupied"));
        CompressionOptions options;
        options.level = 0;
        options.VolumeSizes.Add(64 << 10);
        ArchiveBackend backend(nullptr, engine());
        QSignalSpy finished(&backend, &ArchiveBackend::finished);
        QSignalSpy started(&backend, &ArchiveBackend::engineStarted);
        if (mode == 2)
            connect(&backend, &ArchiveBackend::engineStarted, &backend, &ArchiveBackend::cancel);
        bool handled = false;
        connect(&backend, &ArchiveBackend::progress, &backend, [&](int, const QString &detail) {
            if (handled || detail != (russian ? QStringLiteral("Создание архивных томов")
                                              : QString("Creating archive volumes")))
                return;
            handled = true;
            if (mode == 1)
                QVERIFY(writeFile(occupied, "occupied"));
            if (mode == 3)
                backend.cancel();
            if (mode == 4) {
                QCOMPARE(backend.setPaused(true), S_OK);
                QTimer::singleShot(20, &backend, [&] { backend.cancelAndWait(); });
            }
        });
        backend.create(target, {source}, {}, false, options);
        QTRY_COMPARE_WITH_TIMEOUT(finished.count(), 1, 30000);
        const auto result = finished.takeFirst();
        QVERIFY(!result.first().toBool());
        QCOMPARE(result.at(1).toString(), mode < 2
                                              ? (russian ? QStringLiteral("Том уже существует: %1")
                                                         : QString("Volume already exists: %1"))
                                                    .arg(occupied)
                                              : (russian ? QStringLiteral("Операция отменена.")
                                                         : QString("Operation canceled.")));
        QVERIFY(!backend.busy());
        QCOMPARE(readFile(source), bytes);
        QVERIFY(!QFileInfo::exists(target));
        if (mode < 2)
            QCOMPARE(readFile(occupied), QByteArray("occupied"));
        const QStringList volumes = QDir(fixture.path()).entryList({"output.7z.*"}, QDir::Files);
        QCOMPARE(volumes.size(), mode < 2 ? 1 : 0);
        if (mode == 0)
            QCOMPARE(started.count(), 0);
        else if (mode >= 2)
            QCOMPARE(backend.lastResult(), E_ABORT);
        QVERIFY(
            QDir(fixture.path())
                .entryList({".sevenzip-*"}, QDir::AllEntries | QDir::Hidden | QDir::NoDotAndDotDot)
                .isEmpty());
    }
    void volumeDialogValidationAndGeometry_data() {
        QTest::addColumn<int>("width");
        QTest::addColumn<int>("height");
        QTest::newRow("normal") << 760 << 720;
        QTest::newRow("compact") << 640 << 480;
    }
    void smallVolumeConfirmationDoesNotSaveUntilYes() {
        QTemporaryDir fixture;
        QVERIFY(fixture.isValid());
        const QString source = fixture.filePath("source.bin");
        QVERIFY(writeFile(source, "source"));
        CreateArchiveDialog dialog(fixture.path(), {source});
        auto *volume = dialog.findChild<QComboBox *>("archiveVolumeSize");
        volume->setEditText("64K");
        auto *ok = dialog.findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Ok);
        const QByteArray original = readFile(settings_);
        for (const auto answer : {QMessageBox::No, QMessageBox::Cancel, QMessageBox::Yes}) {
            QTimer::singleShot(0, &dialog, [answer] {
                auto *message = qobject_cast<QMessageBox *>(QApplication::activeModalWidget());
                QVERIFY(message);
                QVERIFY(message->text().contains("65536"));
                message->button(answer)->click();
            });
            ok->click();
            if (answer != QMessageBox::Yes) {
                QCOMPARE(dialog.result(), int(QDialog::Rejected));
                QCOMPARE(readFile(settings_), original);
            }
        }
        QCOMPARE(dialog.result(), int(QDialog::Accepted));
        QCOMPARE(dialog.compressionOptions().VolumeSizes[0], UInt64(65536));
    }
    void volumeDialogValidationAndGeometry() {
        QFETCH(int, width);
        QFETCH(int, height);
        QTemporaryDir fixture;
        QVERIFY(fixture.isValid());
        const QString source = fixture.filePath("source.bin");
        QVERIFY(writeFile(source, "source"));
        CreateArchiveDialog dialog(fixture.path(), {source});
        auto *volume = dialog.findChild<QComboBox *>("archiveVolumeSize");
        QVERIFY(volume && volume->isEditable() && volume->isEnabled());
        QCOMPARE(volume->currentText(), QString());
        QCOMPARE(volume->itemText(0), QString("10M"));
        volume->setEditText("64K 128K");
        dialog.resize(width, height);
        dialog.show();
        QCoreApplication::processEvents();
        auto *scroll = dialog.findChild<QScrollArea *>("archiveOptionsScrollArea");
        scroll->ensureWidgetVisible(volume);
        QCoreApplication::processEvents();
        QVERIFY(UiInspection::fullyVisible(*volume, dialog));
        auto *footer = dialog.findChild<QDialogButtonBox *>();
        QVERIFY(UiInspection::fullyVisible(*footer, dialog));
        QVERIFY(UiInspection::separate(*volume, *footer, dialog));
        QVERIFY(UiInspection::writeReport(dialog, "archive-volumes-" + QString::number(width)));
        const QByteArray original = readFile(settings_);
        volume->setEditText("0");
        QTimer::singleShot(0, &dialog, [&] {
            auto *message = qobject_cast<QMessageBox *>(QApplication::activeModalWidget());
            QVERIFY(message);
            message->accept();
        });
        footer->button(QDialogButtonBox::Ok)->click();
        QCOMPARE(dialog.result(), int(QDialog::Rejected));
        QCOMPARE(readFile(settings_), original);
        volume->setEditText("64K 128K");
        footer->button(QDialogButtonBox::Ok)->click();
        QCOMPARE(dialog.result(), int(QDialog::Accepted));
        const auto sizes = dialog.compressionOptions().VolumeSizes;
        QCOMPARE(sizes.Size(), 2u);
        QCOMPARE(sizes[0], UInt64(64 << 10));
        QCOMPARE(sizes[1], UInt64(128 << 10));
        CreateArchiveDialog update(fixture.path(), {source}, nullptr,
                                   fixture.filePath("existing.zip"));
        QVERIFY(!update.findChild<QComboBox *>("archiveVolumeSize")->isEnabled());
    }
};
QTEST_MAIN(ArchiveVolumeCreationTests)
#include "archive_volume_creation_tests.moc"
