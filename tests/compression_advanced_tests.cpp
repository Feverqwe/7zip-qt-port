#include "7zip/PropID.h"
#include "createarchivedialog.h"
#include "layout_inspection.h"
#include "qtcompressoptionsdialog.h"
#include <QDateTime>
#include <QScrollArea>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTimeZone>
#include <QTimer>
#include <QtTest>

namespace {
bool writeFile(const QString &path, const QByteArray &bytes) {
    QFile file(path);
    return file.open(QIODevice::WriteOnly) && file.write(bytes) == bytes.size();
}
QByteArray readFile(const QString &path) {
    QFile file(path);
    return file.open(QIODevice::ReadOnly) ? file.readAll() : QByteArray();
}
QCheckBox *check(QWidget &dialog, const char *name) { return dialog.findChild<QCheckBox *>(name); }
CCompressDialog owner(NCompression::CFormatOptions &profile, const QString &format) {
    CCompressDialog result;
    result.FormatOptions = &profile;
    result.ArcInfo.Name = format.toStdWString().c_str();
    result.ArcInfo.Flags = NArcInfoFlags::kCTime | NArcInfoFlags::kATime | NArcInfoFlags::kMTime |
                           NArcInfoFlags::kMTime_Default;
    result.ArcInfo.TimeFlags = TIME_PREC_TO_ARC_FLAGS_MASK(NFileTimeType::kWindows);
    if (format == "zip")
        result.ArcInfo.TimeFlags |= TIME_PREC_TO_ARC_FLAGS_MASK(NFileTimeType::kUnix) |
                                    TIME_PREC_TO_ARC_FLAGS_MASK(NFileTimeType::kDOS);
    return result;
}
} // namespace

class CompressionAdvancedTests final : public QObject {
    Q_OBJECT
    QTemporaryDir settingsDirectory_;
    QByteArray oldSettings_;
    QString settings_;

  private slots:
    void initTestCase() {
        QVERIFY(settingsDirectory_.isValid());
        oldSettings_ = qgetenv("SEVENZIP_SETTINGS_PATH");
        settings_ = settingsDirectory_.filePath("settings.ini");
        qputenv("SEVENZIP_SETTINGS_PATH", settings_.toUtf8());
    }
    void cleanupTestCase() {
        if (oldSettings_.isNull())
            qunsetenv("SEVENZIP_SETTINGS_PATH");
        else
            qputenv("SEVENZIP_SETTINGS_PATH", oldSettings_);
    }
    void init() { QFile::remove(settings_); }
    void optionsStatesAndGeometry_data() {
        QTest::addColumn<QString>("format");
        QTest::addColumn<QSize>("size");
        QTest::newRow("zip-normal") << QString("zip") << QSize(520, 400);
        QTest::newRow("zip-compact") << QString("zip") << QSize(440, 340);
        QTest::newRow("7z-normal") << QString("7z") << QSize(520, 400);
        QTest::newRow("7z-compact") << QString("7z") << QSize(440, 340);
    }
    void optionsStatesAndGeometry() {
        QFETCH(QString, format);
        QFETCH(QSize, size);
        NCompression::CFormatOptions profile;
        auto source = owner(profile, format);
        COptionsDialog dialog(&source);
        dialog.resize(size);
        dialog.show();
        QCoreApplication::processEvents();
        QCOMPARE(dialog.size(), size);
        auto *precision = dialog.findChild<QComboBox *>("IDC_COMPRESS_TIME_PREC");
        QCOMPARE(precision->count(), format == "zip" ? 3 : 1);
        QVERIFY(!dialog.findChild<QGroupBox *>("IDG_COMPRESS_NTFS")->isVisible());
        QVERIFY(check(dialog, "IDX_COMPRESS_MTIME")->isChecked());
        QVERIFY(!check(dialog, "IDX_COMPRESS_MTIME")->isEnabled());
        QVERIFY(!check(dialog, "IDX_COMPRESS_MTIME_SET")->isVisible());
        QVERIFY(!check(dialog, "IDX_COMPRESS_CTIME")->isEnabled());
        QVERIFY(!check(dialog, "IDX_COMPRESS_PRESERVE_ATIME")->isEnabled());
        QVERIFY(!check(dialog, "IDX_COMPRESS_ZTIME_SET")->isEnabled());
        if (format == "zip") {
            check(dialog, "IDX_COMPRESS_PREC_SET")->click();
            precision->setCurrentIndex(precision->findData(UInt32(NFileTimeType::kUnix)));
            QMetaObject::invokeMethod(precision, "activated", Qt::DirectConnection,
                                      Q_ARG(int, precision->currentIndex()));
            QVERIFY(!check(dialog, "IDX_COMPRESS_CTIME")->isVisible());
            QVERIFY(!check(dialog, "IDX_COMPRESS_ATIME")->isVisible());
            check(dialog, "IDX_COMPRESS_PREC_SET")->click();
            QCOMPARE(precision->currentData().toUInt(), UInt32(NFileTimeType::kWindows));
            QVERIFY(check(dialog, "IDX_COMPRESS_CTIME")->isVisible());
        }
        check(dialog, "IDX_COMPRESS_CTIME_SET")->click();
        QVERIFY(check(dialog, "IDX_COMPRESS_CTIME")->isEnabled());
        check(dialog, "IDX_COMPRESS_CTIME")->click();
        auto *buttons = dialog.findChild<QDialogButtonBox *>("compressOptionsButtons");
        for (QWidget *widget :
             {static_cast<QWidget *>(precision),
              static_cast<QWidget *>(check(dialog, "IDX_COMPRESS_CTIME")),
              static_cast<QWidget *>(check(dialog, "IDX_COMPRESS_PRESERVE_ATIME")),
              static_cast<QWidget *>(buttons)})
            QVERIFY(UiInspection::fullyVisible(*widget, dialog));
        QVERIFY(UiInspection::separate(*dialog.findChild<QGroupBox *>("IDG_COMPRESS_TIME"),
                                       *buttons, dialog));
        QVERIFY(UiInspection::writeReport(
            dialog, QString("compression-options-%1-%2").arg(format).arg(size.width())));
        dialog.accept();
        QVERIFY(profile.CTime.Def);
        QVERIFY(profile.CTime.Val);
        QVERIFY(!profile.SetArcMTime.Def);
    }
    void addFormGeometry_data() {
        QTest::addColumn<QSize>("size");
        QTest::newRow("normal") << QSize(760, 720);
        QTest::newRow("compact") << QSize(640, 480);
    }
    void addFormGeometry() {
        QFETCH(QSize, size);
        QTemporaryDir fixture;
        QVERIFY(fixture.isValid());
        const QString source = fixture.filePath("source.txt");
        QVERIFY(writeFile(source, "data"));
        CreateArchiveDialog dialog(fixture.path(), {source});
        dialog.resize(size);
        dialog.show();
        auto *parameters = dialog.findChild<QLineEdit *>("archiveParameters");
        auto *advanced = dialog.findChild<QPushButton *>("archiveAdvancedOptions");
        auto *volume = dialog.findChild<QComboBox *>("archiveVolumeSize");
        auto *scroll = dialog.findChild<QScrollArea *>();
        auto *buttons = dialog.findChild<QDialogButtonBox *>();
        QVERIFY(scroll);
        scroll->ensureWidgetVisible(advanced);
        QCoreApplication::processEvents();
        QVERIFY(UiInspection::fullyVisible(*parameters, dialog));
        QVERIFY(UiInspection::fullyVisible(*advanced, dialog));
        QVERIFY(UiInspection::fullyVisible(*buttons, dialog));
        QVERIFY(UiInspection::separate(*parameters, *advanced, dialog));
        QVERIFY(UiInspection::bounds(*volume, dialog).bottom() <
                UiInspection::bounds(*parameters, dialog).top());
        QVERIFY(
            UiInspection::writeReport(dialog, QString("archive-advanced-%1").arg(size.width())));
    }
    void profilesAndCancel() {
        QTemporaryDir fixture;
        QVERIFY(fixture.isValid());
        const QString source = fixture.filePath("source.txt");
        QVERIFY(writeFile(source, "data"));
        CreateArchiveDialog dialog(fixture.path(), {source});
        auto *format = dialog.findChild<QComboBox *>("archiveFormat");
        auto *parameters = dialog.findChild<QLineEdit *>("archiveParameters");
        auto *advanced = dialog.findChild<QPushButton *>("archiveAdvancedOptions");
        parameters->setText("  -mtm=off mt=2  ");
        format->setCurrentText("zip");
        QVERIFY(parameters->text().isEmpty());
        parameters->setText("m=Store");
        format->setCurrentText("7z");
        QCOMPARE(parameters->text(), QString("-mtm=off mt=2"));
        const auto setTime = [&](bool accept) {
            QTimer::singleShot(0, [&] {
                auto *options = dynamic_cast<COptionsDialog *>(QApplication::activeModalWidget());
                QVERIFY(options);
                check(*options, "IDX_COMPRESS_CTIME_SET")->click();
                check(*options, "IDX_COMPRESS_CTIME")->click();
                if (accept)
                    options->accept();
                else
                    options->reject();
            });
            advanced->click();
        };
        setTime(false);
        QVERIFY(!dialog.compressionOptions().CTime.Def);
        setTime(true);
        QVERIFY(dialog.compressionOptions().CTime.Val);
        QVERIFY(!QFileInfo::exists(settings_));
        dialog.reject();
        QVERIFY(!QFileInfo::exists(settings_));
        // A separate confirmed Add saves both per-format property sets.
        CreateArchiveDialog confirmed(fixture.path(), {source});
        QTimer::singleShot(0, [&] {
            auto *options = dynamic_cast<COptionsDialog *>(QApplication::activeModalWidget());
            QVERIFY(options);
            check(*options, "IDX_COMPRESS_CTIME_SET")->click();
            check(*options, "IDX_COMPRESS_CTIME")->click();
            options->accept();
        });
        confirmed.findChild<QPushButton *>("archiveAdvancedOptions")->click();
        confirmed.findChild<QLineEdit *>("archiveParameters")->setText("mt=2");
        confirmed.findChild<QComboBox *>("archiveFormat")->setCurrentText("zip");
        confirmed.findChild<QLineEdit *>("archiveParameters")->setText("m=Store");
        QTimer::singleShot(0, [&] {
            auto *options = dynamic_cast<COptionsDialog *>(QApplication::activeModalWidget());
            QVERIFY(options);
            check(*options, "IDX_COMPRESS_PREC_SET")->click();
            auto *precision = options->findChild<QComboBox *>("IDC_COMPRESS_TIME_PREC");
            precision->setCurrentIndex(precision->findData(UInt32(NFileTimeType::kUnix)));
            options->accept();
        });
        confirmed.findChild<QPushButton *>("archiveAdvancedOptions")->click();
        confirmed.findChild<QComboBox *>("archiveFormat")->setCurrentText("7z");
        confirmed.findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Ok)->click();
        QCOMPARE(confirmed.result(), int(QDialog::Accepted));
        CreateArchiveDialog reopened(fixture.path(), {source});
        QCOMPARE(reopened.findChild<QLineEdit *>("archiveParameters")->text(), QString("mt=2"));
        QVERIFY(reopened.compressionOptions().CTime.Def);
        QVERIFY(reopened.compressionOptions().CTime.Val);
        reopened.findChild<QComboBox *>("archiveFormat")->setCurrentText("zip");
        QCOMPARE(reopened.findChild<QLineEdit *>("archiveParameters")->text(), QString("m=Store"));
        QCOMPARE(reopened.compressionOptions().TimePrec, UInt32(NFileTimeType::kUnix));
    }
    void timestampAndMethodRoundTrip_data() {
        QTest::addColumn<QString>("format");
        QTest::addColumn<UInt32>("precision");
        QTest::newRow("7z-windows") << QString("7z") << UInt32(NFileTimeType::kWindows);
        QTest::newRow("zip-windows") << QString("zip") << UInt32(NFileTimeType::kWindows);
        QTest::newRow("zip-unix") << QString("zip") << UInt32(NFileTimeType::kUnix);
        QTest::newRow("zip-dos") << QString("zip") << UInt32(NFileTimeType::kDOS);
    }
    void timestampAndMethodRoundTrip() {
        QFETCH(QString, format);
        QFETCH(UInt32, precision);
        QTemporaryDir fixture, output;
        QVERIFY(fixture.isValid());
        QVERIFY(output.isValid());
        const QString name = QStringLiteral("время с пробелом.txt");
        const QString source = fixture.filePath(name);
        QVERIFY(writeFile(source, QByteArray(8192, 'a')));
        QFile file(source);
        QVERIFY(file.open(QIODevice::ReadWrite));
        const qint64 time = 1700000001123;
        QVERIFY(file.setFileTime(QDateTime::fromMSecsSinceEpoch(time, QTimeZone::UTC),
                                 QFileDevice::FileModificationTime));
        file.close();
        CompressionOptions options;
        options.TimePrec = precision;
        options.Options = format == "7z" ? "-m0=BZip2" : "-mm=Store";
        const QString archive = fixture.filePath("time." + format);
        ArchiveBackend backend;
        QSignalSpy finished(&backend, &ArchiveBackend::finished);
        backend.create(archive, {source}, {}, false, options);
        QTRY_COMPARE_WITH_TIMEOUT(finished.size(), 1, 15000);
        QVERIFY2(finished.last().first().toBool(), qPrintable(finished.last().at(1).toString()));
        QProcess listing;
        listing.start("7zz", {"l", "-slt", "--", archive});
        QVERIFY(listing.waitForFinished(15000));
        QCOMPARE(listing.exitCode(), 0);
        const QByteArray metadata = listing.readAllStandardOutput();
        QVERIFY(metadata.contains(format == "7z" ? "Method = BZip2" : "Method = Store"));
        QSignalSpy opened(&backend, &ArchiveBackend::entriesReady);
        backend.list(archive);
        QTRY_COMPARE_WITH_TIMEOUT(opened.size(), 1, 15000);
        finished.clear();
        backend.extract(archive, output.path());
        QTRY_COMPARE_WITH_TIMEOUT(finished.size(), 1, 15000);
        QVERIFY2(finished.last().first().toBool(), qPrintable(finished.last().at(1).toString()));
        const qint64 actual = QFileInfo(output.filePath(name)).lastModified().toMSecsSinceEpoch();
        if (precision == NFileTimeType::kDOS) {
            // Source TimeUtils rounds up on POSIX; Windows uses its native API.
            // Both must store two-second precision within one quantum.
            QCOMPARE(actual % 2000, qint64(0));
            QVERIFY(qAbs(actual - time) < 2000);
#ifndef Q_OS_WIN
            QCOMPARE(actual, (time + 1999) / 2000 * 2000);
#endif
        } else
            QCOMPARE(actual, precision == NFileTimeType::kUnix ? time / 1000 * 1000 : time);
        QCOMPARE(readFile(output.filePath(name)), QByteArray(8192, 'a'));
    }
    void omittedTimeInvalidPropertyAndCancel() {
        QTemporaryDir fixture;
        QVERIFY(fixture.isValid());
        const QString source = fixture.filePath(QStringLiteral("исходник с пробелом.txt"));
        const QByteArray bytes(500000, 'a');
        QVERIFY(writeFile(source, bytes));
        CompressionOptions options;
        options.MTime.Def = true;
        options.MTime.Val = false;
        ArchiveBackend backend;
        QSignalSpy finished(&backend, &ArchiveBackend::finished);
        const QString target = fixture.filePath("no-time.7z");
        backend.create(target, {source}, {}, false, options);
        QTRY_COMPARE_WITH_TIMEOUT(finished.size(), 1, 15000);
        QVERIFY(finished.last().first().toBool());
        QSignalSpy listed(&backend, &ArchiveBackend::entriesReady);
        backend.list(target);
        QTRY_COMPARE_WITH_TIMEOUT(listed.size(), 1, 15000);
        const auto entries = qvariant_cast<QList<ArchiveEntry>>(listed.first().first());
        QCOMPARE(entries.size(), 1);
        QVERIFY(entries.first().modified.isEmpty());
        options.Options = "unknown-property=on";
        const QString bad = fixture.filePath("invalid.7z");
        finished.clear();
        backend.create(bad, {source}, {}, false, options);
        QTRY_COMPARE_WITH_TIMEOUT(finished.size(), 1, 15000);
        QVERIFY(!finished.last().first().toBool());
        QVERIFY(!QFileInfo::exists(bad));
        options.Options.Empty();
        options.TimePrec = UInt32(NFileTimeType::kUnix); // Unsupported by the source 7z handler.
        finished.clear();
        backend.create(bad, {source}, {}, false, options);
        QTRY_COMPARE_WITH_TIMEOUT(finished.size(), 1, 15000);
        QVERIFY(!finished.last().first().toBool());
        QVERIFY(!QFileInfo::exists(bad));
        options.TimePrec = UInt32(-1);
        options.Options = "0=BZip2";
        finished.clear();
        connect(&backend, &ArchiveBackend::engineStarted, &backend, &ArchiveBackend::cancel);
        backend.create(bad, {source}, {}, false, options);
        QTRY_COMPARE_WITH_TIMEOUT(finished.size(), 1, 15000);
        QCOMPARE(backend.lastResult(), E_ABORT);
        QVERIFY(!QFileInfo::exists(bad));
        QCOMPARE(readFile(source), bytes);
        QVERIFY(
            QDir(fixture.path())
                .entryList({".sevenzip-*"}, QDir::AllEntries | QDir::Hidden | QDir::NoDotAndDotDot)
                .isEmpty());
    }
};
QTEST_MAIN(CompressionAdvancedTests)
#include "compression_advanced_tests.moc"
