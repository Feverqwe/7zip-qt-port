#include "7zip/UI/GUI/CompressDialogRes.h"
#include "CPP/7zip/UI/FileManager/LangUtils.h"
#include "CPP/7zip/UI/FileManager/RegistryUtils.h"
#include "archivecodeccatalog.h"
#include "createarchivedialog.h"
#include "layout_inspection.h"
#include <QScrollArea>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTimer>
#include <QtTest>

class CompressionEventTests final : public QObject {
    Q_OBJECT
    QTemporaryDir fixture_;
    QString settings_, input_, module_;
    QByteArray previous_;
    std::shared_ptr<const ArchiveCodecCatalog> catalog_;
    const QByteArray bytes_ = QByteArray("source\0данные", 19);

    static QComboBox *combo(CreateArchiveDialog &dialog, const char *name) {
        return dialog.findChild<QComboBox *>(name);
    }
    void profile(UInt32 threads, UInt32 block = UInt32(-1)) {
        NCompression::CInfo registry;
        registry.ArcType = "xz";
        auto &format = registry.Formats.AddNew();
        format.FormatID = "xz";
        format.Method = "LZMA2";
        format.Level = 5;
        format.Dictionary = 1 << 20;
        format.NumThreads = threads;
        format.BlockLogSize = block;
        format.MemUse = "50M";
        registry.Save(settings_);
    }
    static QByteArray contents(const QString &path) {
        QFile file(path);
        return file.open(QIODevice::ReadOnly) ? file.readAll() : QByteArray();
    }
    void language(const QString &name) {
        SaveRegLang(UString(name.toStdWString().c_str()), settings_);
        ReloadLang(settings_);
    }
    static void languageData() {
        QTest::addColumn<QString>("language");
        QTest::newRow("en") << QString("-");
        QTest::newRow("ru") << QString("ru");
    }

  private slots:
    void initTestCase() {
#ifndef SEVENZIP_CODEC_MODULE
        QSKIP("Official codec module required");
#else
        QVERIFY(fixture_.isValid());
        module_ = QString::fromUtf8(SEVENZIP_CODEC_MODULE);
        catalog_ = std::make_shared<ArchiveCodecCatalog>(module_, 0);
        QCOMPARE(catalog_->result(), S_OK);
        settings_ = fixture_.filePath("settings.ini");
        input_ = fixture_.filePath(QStringLiteral("вход 空.bin"));
        QFile file(input_);
        QVERIFY(file.open(QIODevice::WriteOnly));
        QCOMPARE(file.write(bytes_), qint64(bytes_.size()));
        previous_ = qgetenv("SEVENZIP_SETTINGS_PATH");
        qputenv("SEVENZIP_SETTINGS_PATH", settings_.toUtf8());
#endif
    }
    void init() { QFile::remove(settings_); }
    void cleanupTestCase() {
        if (previous_.isNull())
            qunsetenv("SEVENZIP_SETTINGS_PATH");
        else
            qputenv("SEVENZIP_SETTINGS_PATH", previous_);
    }
    void levelKeepsEditedParameters_data() {
        QTest::addColumn<QString>("language");
        QTest::addColumn<QString>("format");
        for (const QString &language : {QString("-"), QString("ru")})
            for (const QString &format : {QString("7z"), QString("zip"), QString("xz")})
                QTest::newRow(qPrintable(language + '-' + format)) << language << format;
    }
    void levelKeepsEditedParameters() {
        QFETCH(QString, language);
        QFETCH(QString, format);
        this->language(language);
        const QByteArray before = contents(settings_);
        CreateArchiveDialog dialog(fixture_.path(), {input_}, nullptr, catalog_);
        combo(dialog, "archiveFormat")->setCurrentText(format);
        auto *parameters = dialog.findChild<QLineEdit *>("archiveParameters");
        parameters->setText("  mt=1  ");
        auto *level = combo(dialog, "compressionLevel");
        for (UInt32 value : {9u, 1u, 5u}) {
            const int index = level->findData(value);
            QVERIFY(index >= 0);
            level->setCurrentIndex(index);
            QCOMPARE(parameters->text(), QString("  mt=1  "));
            QCOMPARE(dialog.sourceInfo().Level, value);
            QCOMPARE(dialog.sourceInfo().Options, UString(L"mt=1"));
            QCOMPARE(dialog.sourceInfo().NumThreads, UInt32(-1));
        }
        dialog.reject();
        QCOMPARE(contents(settings_), before);
        QCOMPARE(contents(input_), bytes_);
    }
    void solidKeepsThreadSelection_data() {
        QTest::addColumn<UInt32>("threads");
        QTest::addColumn<QString>("language");
        for (const QString &lang : {QString("-"), QString("ru")})
            for (UInt32 threads : {UInt32(-1), UInt32(1), UInt32(999999)})
                QTest::newRow(qPrintable(lang + '-' + QString::number(threads))) << threads << lang;
    }
    void solidKeepsThreadSelection() {
        QFETCH(UInt32, threads);
        QFETCH(QString, language);
        profile(threads);
        this->language(language);
        const auto before = contents(settings_);
        CreateArchiveDialog dialog(fixture_.path(), {input_}, nullptr, catalog_);
        QCOMPARE(combo(dialog, "archiveFormat")->currentText(), QString("xz"));
        auto *thread = combo(dialog, "compressionThreads");
        auto *solid = combo(dialog, "compressionSolidBlock");
        const int selection = thread->currentIndex();
        QCOMPARE(selection < 0, threads == 999999);
        const QString autoText = thread->itemText(0);
        for (UInt32 block : {20u, 64u, UInt32(-1)}) {
            const int index = solid->findData(block);
            QVERIFY(index >= 0);
            solid->setCurrentIndex(index);
            QCOMPARE(thread->currentIndex(), selection);
            QCOMPARE(thread->itemText(0), autoText);
            QCOMPARE(dialog.compressionOptions().NumThreads,
                     threads == 999999 ? UInt32(-1) : threads);
        }
        dialog.reject();
        QCOMPARE(contents(settings_), before);
        QCOMPARE(contents(input_), bytes_);
    }
    void solidMemoryUsesCachedAuto_data() { languageData(); }
    void solidMemoryUsesCachedAuto() {
        QFETCH(QString, language);
        const auto system = NCompressDialog::GetSystemInfo();
        if (system.NumCPUs < 4 || !system.RamSizeDefined)
            QSKIP("Cached-auto difference requires four available CPUs and known RAM");
        profile(UInt32(-1));
        this->language(language);
        CreateArchiveDialog dialog(fixture_.path(), {input_}, nullptr, catalog_);
        QCOMPARE(combo(dialog, "archiveFormat")->currentText(), QString("xz"));
        auto *thread = combo(dialog, "compressionThreads");
        QCOMPARE(thread->itemText(0), NCompressDialog::Modify_Auto(QString("2")));
        auto *solid = combo(dialog, "compressionSolidBlock");
        solid->setCurrentIndex(solid->findData(20u));
        UInt64 decompress;
        const UInt64 usage = NCompressDialog::GetMemoryUsage_Threads_Dict_DecompMem(
            QStringLiteral("xz"), NCompressDialog::kLZMA2, 5, 2, 1 << 20, 20, decompress);
        const auto options = dialog.compressionOptions();
        QCOMPARE(dialog.findChild<QLabel *>("compressionMemoryUsage")->text(),
                 CCompressDialog::PrintMemUsage(IDT_COMPRESS_MEMORY_VALUE, usage, options.MemUsage,
                                                system));
        QCOMPARE(thread->itemText(0), NCompressDialog::Modify_Auto(QString("2")));
        dialog.show();
        for (const QSize size : {QSize(760, 720), QSize(640, 480)}) {
            dialog.resize(size);
            QApplication::processEvents();
            // Native fonts can put the usage/limit row more than 80 pixels
            // below Threads. Expose the actual source group, retaining both
            // controls' original visibility assertions.
            auto *memoryGroup = dialog.findChild<QWidget *>("compressionMemoryGroup");
            QVERIFY(memoryGroup);
            dialog.findChild<QScrollArea *>()->ensureWidgetVisible(memoryGroup, 0, 0);
            QApplication::processEvents();
            QVERIFY(UiInspection::fullyVisible(*thread, dialog));
            auto *memory = dialog.findChild<QLabel *>("compressionMemoryUsage");
            QVERIFY(UiInspection::labelFits(*memory));
            if (!UiInspection::fullyVisible(*memory, dialog))
                QVERIFY(UiInspection::writeReport(
                    dialog,
                    QString("compression-event-%1-%2-clipped").arg(language).arg(size.width())));
            QVERIFY(UiInspection::fullyVisible(*memory, dialog));
            QVERIFY(UiInspection::writeReport(
                dialog, QString("compression-event-%1-%2").arg(language).arg(size.width())));
        }
        // The memory-limit event, unlike Solid, deliberately regenerates Auto.
        auto *limit = combo(dialog, "compressionMemoryLimit");
        limit->setCurrentIndex(limit->findData("256M"));
        const auto recalculated = NCompressDialog::SetNumThreads2(
            QStringLiteral("xz"), NCompressDialog::kLZMA2, 5, 1 << 20, 20, system,
            dialog.compressionOptions().MemUsage);
        QCOMPARE(thread->itemText(0),
                 NCompressDialog::Modify_Auto(QString::number(recalculated.AutoNumThreads)));
        QCOMPARE(thread->currentIndex(), 0);
        auto *dictionary = combo(dialog, "compressionDictionary");
        dictionary->setCurrentIndex(dictionary->findData(qulonglong(2 << 20)));
        const auto afterDictionary = NCompressDialog::SetNumThreads2(
            QStringLiteral("xz"), NCompressDialog::kLZMA2, 5, 2 << 20, UInt32(-1), system,
            dialog.compressionOptions().MemUsage);
        QCOMPARE(thread->itemText(0),
                 NCompressDialog::Modify_Auto(QString::number(afterDictionary.AutoNumThreads)));
        QVERIFY(!dialog.compressionOptions().SolidIsSpecified);
        dialog.reject();
    }
    void sourceMemoryRefusalDoesNotSave_data() { languageData(); }
    void sourceMemoryRefusalDoesNotSave() {
        QFETCH(QString, language);
        const auto system = NCompressDialog::GetSystemInfo();
        if (system.NumCPUs < 4 || !system.RamSizeDefined)
            QSKIP("Cached-auto refusal requires four available CPUs and known RAM");
        profile(UInt32(-1), 64);
        this->language(language);
        const auto before = contents(settings_);
        CreateArchiveDialog dialog(fixture_.path(), {input_}, nullptr, catalog_);
        QCOMPARE(combo(dialog, "archiveFormat")->currentText(), QString("xz"));
        auto *solid = combo(dialog, "compressionSolidBlock");
        solid->setCurrentIndex(solid->findData(UInt32(-1)));
        QSignalSpy accepted(&dialog, &QDialog::accepted);
        bool refused = false;
        QTimer::singleShot(0, &dialog, [&] {
            if (auto *message = qobject_cast<QMessageBox *>(QApplication::activeModalWidget())) {
                refused =
                    message->icon() == QMessageBox::Critical && message->text().contains("50 MB");
                message->accept();
            }
        });
        dialog.findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Ok)->click();
        QVERIFY(refused);
        QCOMPARE(accepted.size(), 0);
        QCOMPARE(dialog.result(), 0);
        QCOMPARE(contents(settings_), before);
        dialog.reject();
        QCOMPARE(contents(input_), bytes_);
    }
    void savedEventProfileRunsOfficialEngine_data() { languageData(); }
    void savedEventProfileRunsOfficialEngine() {
        QFETCH(QString, language);
        profile(UInt32(-1));
        this->language(language);
        {
            CreateArchiveDialog dialog(fixture_.path(), {input_}, nullptr, catalog_);
            QCOMPARE(combo(dialog, "archiveFormat")->currentText(), QString("xz"));
            auto *solid = combo(dialog, "compressionSolidBlock");
            solid->setCurrentIndex(solid->findData(20u));
            dialog.findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Ok)->click();
            QCOMPARE(dialog.result(), int(QDialog::Accepted));
        }
        CreateArchiveDialog restarted(fixture_.path(), {input_}, nullptr, catalog_);
        const auto options = restarted.compressionOptions();
        QCOMPARE(options.format, QString("xz"));
        QCOMPARE(options.NumThreads, UInt32(-1));
        QCOMPARE(options.SolidBlockSize, UInt64(1 << 20));
        QCOMPARE(options.Dict64, UInt64(1 << 20));
        QCOMPARE(options.MemUsage.GetBytes(0), UInt64(50 << 20));
        restarted.reject();
        ArchiveBackend backend;
        QCOMPARE(backend.loadCodecCatalog(module_, catalog_->version()), S_OK);
        QSignalSpy done(&backend, &ArchiveBackend::finished);
        const QString target = fixture_.filePath("event " + language + " 空.xz");
        backend.create(target, {input_}, {}, false, options);
        QTRY_COMPARE_WITH_TIMEOUT(done.size(), 1, 15000);
        QVERIFY2(done.first()[0].toBool(), qPrintable(done.first()[1].toString()));
        QList<ArchiveEntry> entries;
        connect(&backend, &ArchiveBackend::entriesReady, this,
                [&](const QList<ArchiveEntry> &value) { entries = value; });
        done.clear();
        backend.list(target);
        QTRY_COMPARE_WITH_TIMEOUT(done.size(), 1, 15000);
        QVERIFY2(done.first()[0].toBool(), qPrintable(done.first()[1].toString()));
        QCOMPARE(entries.size(), 1);
        QCOMPARE(entries.first().size, qint64(bytes_.size()));
        done.clear();
        backend.test(target);
        QTRY_COMPARE_WITH_TIMEOUT(done.size(), 1, 15000);
        QVERIFY2(done.first()[0].toBool(), qPrintable(done.first()[1].toString()));
        done.clear();
        const QString output = fixture_.filePath("output " + language);
        backend.extract(target, output);
        QTRY_COMPARE_WITH_TIMEOUT(done.size(), 1, 15000);
        QVERIFY2(done.first()[0].toBool(), qPrintable(done.first()[1].toString()));
        QCOMPARE(contents(QDir(output).filePath(entries.first().path)), bytes_);
        const QString failed = fixture_.filePath("invalid " + language + ".xz");
        auto invalid = options;
        invalid.NumThreads = 999999;
        done.clear();
        backend.create(failed, {input_}, {}, false, invalid);
        QTRY_COMPARE_WITH_TIMEOUT(done.size(), 1, 15000);
        QVERIFY(!done.first()[0].toBool());
        QVERIFY(!QFileInfo::exists(failed));
        const QString cancelled = fixture_.filePath("cancel " + language + ".xz");
        done.clear();
        backend.create(cancelled, {input_}, {}, false, options);
        backend.cancel();
        QTRY_COMPARE_WITH_TIMEOUT(done.size(), 1, 15000);
        QVERIFY(!done.first()[0].toBool());
        QCOMPARE(backend.lastResult(), E_ABORT);
        QVERIFY(!QFileInfo::exists(cancelled));
        QCOMPARE(contents(input_), bytes_);
    }
};
QTEST_MAIN(CompressionEventTests)
#include "compression_event_tests.moc"
