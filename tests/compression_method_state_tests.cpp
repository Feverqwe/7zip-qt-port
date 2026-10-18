#include "7zip/UI/GUI/CompressDialogRes.h"
#include "CPP/7zip/UI/FileManager/LangUtils.h"
#include "CPP/7zip/UI/FileManager/RegistryUtils.h"
#include "CPP/7zip/UI/GUI/UpdateGUI.h"
#include "archivecodeccatalog.h"
#include "createarchivedialog.h"
#include "layout_inspection.h"
#include <QScrollArea>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QtTest>

class CompressionMethodStateTests final : public QObject {
    Q_OBJECT
    QTemporaryDir _fixture;
    QByteArray _previousSettings;
    QString _settings, _input;
    std::shared_ptr<const ArchiveCodecCatalog> _catalog;
    const QByteArray _bytes = QByteArray("original\0bytes", 14);

    void language(const QString &value) {
        SaveRegLang(UString(value.toStdWString().c_str()), _settings);
        ReloadLang(_settings);
    }
    CCompressDialog source(const QString &format, bool external = false) {
        CCompressDialog model;
        model.Info.FormatIndex = _catalog->formatIndex(format);
        model.Info.ArcPath = _fixture.filePath("archive." + format).toStdWString().c_str();
        NWildcard::CCensor censor;
        censor.AddPreItem_NoWildcard(UString(_input.toStdWString().c_str()));
        NCompressDialog::PrepareFormatSelection(model, _catalog->nativeCodecs(),
                                                censor.CensorPaths);
        if (external)
            model.ExternalMethods.Add(AString("UnavailableFixtureEncoder"));
        return model;
    }
    static QComboBox *combo(CreateArchiveDialog &dialog, const char *name) {
        return dialog.findChild<QComboBox *>(name);
    }

  private slots:
    void initTestCase() {
#ifndef SEVENZIP_CODEC_MODULE
        QSKIP("Official codec module required");
#else
        QVERIFY(_fixture.isValid());
        _catalog =
            std::make_shared<ArchiveCodecCatalog>(QString::fromUtf8(SEVENZIP_CODEC_MODULE), 0);
        QCOMPARE(_catalog->result(), S_OK);
        _settings = _fixture.filePath("settings.ini");
        _input = _fixture.filePath(QStringLiteral("данные 空.bin"));
        QFile file(_input);
        QVERIFY(file.open(QIODevice::WriteOnly));
        QCOMPARE(file.write(_bytes), qint64(_bytes.size()));
        _previousSettings = qgetenv("SEVENZIP_SETTINGS_PATH");
        qputenv("SEVENZIP_SETTINGS_PATH", _settings.toUtf8());
#endif
    }
    void init() { QFile::remove(_settings); }
    void cleanupTestCase() {
        if (_previousSettings.isNull())
            qunsetenv("SEVENZIP_SETTINGS_PATH");
        else
            qputenv("SEVENZIP_SETTINGS_PATH", _previousSettings);
    }
    void states_data() {
        QTest::addColumn<QString>("scenario");
        QTest::addColumn<QString>("language");
        for (const QString &scenario :
             {"7z-store", "zip-store", "tar-gnu", "tar-posix", "gzip", "external", "lzma2"})
            for (const QString &language : {QString("-"), QString("ru")})
                QTest::newRow(qPrintable(scenario + '-' + language)) << scenario << language;
    }
    void states() {
        QFETCH(QString, scenario);
        QFETCH(QString, language);
        this->language(language);
        QFile settings(_settings);
        QVERIFY(settings.open(QIODevice::ReadOnly));
        const QByteArray before = settings.readAll();
        settings.close();
        const QString format = scenario.startsWith("tar")   ? "tar"
                               : scenario.startsWith("zip") ? "zip"
                               : scenario == "gzip"         ? "gzip"
                                                            : "7z";
        auto model = source(format, scenario == "external");
        CreateArchiveDialog dialog(_fixture.path(), {_input}, nullptr, _catalog, &model);
        auto *level = combo(dialog, "compressionLevel");
        auto *method = combo(dialog, "compressionMethod");
        auto *dictionary = combo(dialog, "compressionDictionary");
        auto *order = combo(dialog, "compressionWordSize");
        auto *solid = combo(dialog, "compressionSolidBlock");
        auto *threads = combo(dialog, "compressionThreads");
        if (scenario.endsWith("store"))
            level->setCurrentIndex(level->findData(0u));
        if (scenario == "tar-posix")
            method->setCurrentText("POSIX");
        if (scenario == "external")
            method->setCurrentText("UnavailableFixtureEncoder");
        const bool store = scenario.endsWith("store");
        const bool tar = format == "tar";
        QCOMPARE(dictionary->count() == 0, store || tar || scenario == "external");
        QCOMPARE(order->count() == 0, store || tar || scenario == "external");
        QCOMPARE(solid->count() == 0, store || tar || scenario == "gzip");
        QCOMPARE(threads->count() == 0, tar || scenario == "gzip");
        QCOMPARE(threads->isEnabled(), !tar && scenario != "gzip");
        QCOMPARE(dictionary->isEnabled(), scenario == "lzma2");
        QCOMPARE(order->isEnabled(), scenario == "lzma2" || scenario == "gzip");
        QCOMPARE(solid->isEnabled(), scenario == "lzma2" || scenario == "external");
        auto *memory = dialog.findChild<QLabel *>("compressionMemoryUsage");
        auto *decompress = dialog.findChild<QLabel *>("compressionDecompressionMemory");
        if (store) {
            const QString oneMegabyte = QString::fromUtf8(language == "ru" ? "1 МБ" : "1 MB");
            QVERIFY(memory->text().startsWith(oneMegabyte));
            QCOMPARE(decompress->text(), oneMegabyte);
        }
        if (scenario == "external") {
            QCOMPARE(memory->text(), QString("?"));
            QCOMPARE(decompress->text(), QString("?"));
        }
        dialog.show();
        QCOMPARE(combo(dialog, "compressionMemoryLimit")->isVisible(), !tar);
        QCOMPARE(memory->isVisible(), !tar);
        QCOMPARE(decompress->isVisible(), !tar);
        for (auto *label : dialog.findChildren<QLabel *>()) {
            const UInt32 id = label->property("languageID").toUInt();
            if (id == IDT_COMPRESS_MEMORY || id == IDT_COMPRESS_MEMORY_DE)
                QCOMPARE(label->isVisible(), !tar);
        }
        for (const QSize size : {QSize(760, 720), QSize(640, 480)}) {
            dialog.resize(size);
            QApplication::processEvents();
            auto *scroll = dialog.findChild<QScrollArea *>();
            for (auto *field : {dictionary, order, solid, threads}) {
                scroll->ensureWidgetVisible(field, 0, 80);
                QApplication::processEvents();
                QVERIFY(UiInspection::fullyVisible(*field, dialog));
            }
            QVERIFY(UiInspection::fullyVisible(*dialog.findChild<QDialogButtonBox *>(), dialog));
            QVERIFY(UiInspection::writeReport(
                dialog,
                QString("method-state-%1-%2-%3").arg(scenario, language).arg(size.width())));
        }
        // The same controls recover after the format's hidden/empty states.
        combo(dialog, "archiveFormat")->setCurrentText("7z");
        level->setCurrentIndex(level->findData(5u));
        method->setCurrentIndex(method->findData(-1));
        QVERIFY(dictionary->count() > 1 && dictionary->isEnabled());
        QVERIFY(order->count() > 1 && order->isEnabled());
        QVERIFY(solid->count() > 1 && solid->isEnabled());
        QVERIFY(memory->isVisible() && decompress->isVisible());
        dialog.reject();
        QVERIFY(dialog.target().isEmpty());
        QVERIFY(settings.open(QIODevice::ReadOnly));
        QCOMPARE(settings.readAll(), before);
    }
    void storeWorkflow_data() {
        QTest::addColumn<QString>("format");
        QTest::newRow("7z") << QString("7z");
        QTest::newRow("zip") << QString("zip");
    }
    void storeWorkflow() {
        QFETCH(QString, format);
        auto model = source(format);
        CreateArchiveDialog dialog(_fixture.path(), {_input}, nullptr, _catalog, &model);
        combo(dialog, "compressionLevel")->setCurrentIndex(0);
        auto *threads = combo(dialog, "compressionThreads");
        const int index = threads->findData(1u);
        QVERIFY(index >= 0);
        threads->setCurrentIndex(index);
        dialog.findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Ok)->click();
        QCOMPARE(dialog.result(), int(QDialog::Accepted));
        auto options = dialog.compressionOptions();
        QCOMPARE(options.Level, 0);
        QCOMPARE(options.NumThreads, 1u);
        QCOMPARE(options.Dict64, UInt64(-1));
        QCOMPARE(options.Order, UInt32(-1));
        QVERIFY(!options.SolidIsSpecified);
        ArchiveBackend backend;
        QSignalSpy done(&backend, &ArchiveBackend::finished);
        const auto completed = [&] {
            if (done.isEmpty() && !done.wait(15000))
                return false;
            const bool result = done[0][0].toBool();
            done.clear();
            return result;
        };
        QFile::remove(dialog.target());
        backend.create(dialog.target(), {_input}, {}, false, options);
        QVERIFY(completed());
        backend.list(dialog.target());
        QVERIFY(completed());
        backend.test(dialog.target());
        QVERIFY(completed());
        const QString outputDir = _fixture.filePath("out-" + format);
        backend.extract(dialog.target(), outputDir);
        QVERIFY(completed());
        QFile output(QDir(outputDir).filePath(QFileInfo(_input).fileName()));
        QVERIFY(output.open(QIODevice::ReadOnly));
        QCOMPARE(output.readAll(), _bytes);
        options.NumThreads = 100000;
        const QString invalid = _fixture.filePath("invalid." + format);
        backend.create(invalid, {_input}, {}, false, options);
        QCOMPARE(done.size(), 1);
        QVERIFY(!done[0][0].toBool());
        QVERIFY(!QFileInfo::exists(invalid));
        done.clear();
        options.NumThreads = 1;
        const QString cancelled = _fixture.filePath("cancelled." + format);
        backend.create(cancelled, {_input}, {}, false, options);
        QVERIFY(backend.busy());
        backend.cancel();
        QTRY_COMPARE_WITH_TIMEOUT(done.size(), 1, 15000);
        QVERIFY(!done[0][0].toBool());
        QCOMPARE(backend.lastResult(), E_ABORT);
        QVERIFY(!QFileInfo::exists(cancelled));
        QFile input(_input);
        QVERIFY(input.open(QIODevice::ReadOnly));
        QCOMPARE(input.readAll(), _bytes);
    }
};
QTEST_MAIN(CompressionMethodStateTests)
#include "compression_method_state_tests.moc"
