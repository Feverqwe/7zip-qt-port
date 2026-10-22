#include "CPP/7zip/UI/FileManager/RegistryUtils.h"
#include "archivebackend.h"
#include "exfat_fixture.h"
#include "fixtures/progress_dialog.h"
#include "layout_inspection.h"
#include "mainwindow.h"
#include "ntfs_test_support.h"
#include "qtfilesystemreadprompt.h"
#include "qtlanguageservices.h"
#include <QAction>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QMessageBox>
#include <QProcess>
#include <QPushButton>
#include <QSignalSpy>
#include <QTableWidget>
#include <QTemporaryDir>
#include <QTimer>
#include <QTreeWidget>
#include <QtEndian>
#include <QtTest>

namespace {
int Row(QTableWidget &table, const QString &name) {
    for (int i = 0; i < table.rowCount(); ++i)
        if (table.item(i, 0) && table.item(i, 0)->text() == name)
            return i;
    return -1;
}
} // namespace

class FilesystemReadUiTests final : public QObject, protected NtfsTestSupport::Fixture {
    Q_OBJECT
    QTemporaryDir runtime_;
    QString engine_, module_;
    QByteArray previousSettings_;
  private slots:
    void initTestCase() {
        QVERIFY(runtime_.isValid());
        previousSettings_ = qgetenv("SEVENZIP_SETTINGS_PATH");
        qputenv("SEVENZIP_SETTINGS_PATH", runtime_.filePath("settings.ini").toUtf8());
        const QDir prepared(QStringLiteral(SEVENZIP_EXFAT_RUNTIME));
#ifdef Q_OS_WIN
        const QString engineName = "7z.exe", coreName = "7z.dll", suffix = ".dll";
#else
        const QString engineName = "7z", coreName = "7z.so", suffix = ".so";
#endif
        engine_ = runtime_.filePath(engineName);
        module_ = runtime_.filePath(coreName);
        QVERIFY(QFile::copy(prepared.filePath(engineName), engine_));
        QVERIFY(QFile::copy(prepared.filePath(coreName), module_));
        QVERIFY(QDir(runtime_.path()).mkdir("Formats"));
        QVERIFY(QFile::copy(QStringLiteral(SEVENZIP_EXFAT_MODULE),
                            runtime_.filePath("Formats/exfat" + suffix)));
        QVERIFY(QFile::copy(QStringLiteral(SEVENZIP_HFS_MODULE),
                            runtime_.filePath("Formats/hfs" + suffix)));
        QVERIFY(QFile::copy(QStringLiteral(SEVENZIP_NTFS_MODULE),
                            runtime_.filePath("Formats/ntfs" + suffix)));
        QVERIFY(QFile::copy(QStringLiteral(SEVENZIP_EXT_MODULE),
                            runtime_.filePath("Formats/ext" + suffix)));
        QVERIFY(QFile::copy(QStringLiteral(SEVENZIP_FAT_MODULE),
                            runtime_.filePath("Formats/fat" + suffix)));
        QVERIFY(QFile::copy(QStringLiteral(SEVENZIP_SQUASHFS_MODULE),
                            runtime_.filePath("Formats/squashfs" + suffix)));
        QVERIFY(QFile::copy(QStringLiteral(SEVENZIP_CRAMFS_MODULE),
                            runtime_.filePath("Formats/cramfs" + suffix)));
    }
    void cleanupTestCase() { qputenv("SEVENZIP_SETTINGS_PATH", previousSettings_); }
    void deletedRecoveryConsent_data() {
        QTest::addColumn<QString>("language");
        QTest::addColumn<bool>("accept");
        for (const QString &language : {"en", "ru"})
            for (bool accept : {false, true})
                QTest::newRow(qPrintable(language + (accept ? "-accept" : "-cancel")))
                    << language << accept;
    }
    void deletedRecoveryConsent() {
        QFETCH(QString, language);
        QFETCH(bool, accept);
        QtLoadTranslation(language == "en" ? "-" : "ru");
        createArchive();
        loadFixture("deleted-reparse-bad-run");
        CMyComPtr<ISetProperties> properties;
        QCOMPARE(
            archive_->QueryInterface(IID_ISetProperties, reinterpret_cast<void **>(&properties)),
            S_OK);
        const wchar_t *name = L"ld";
        const NWindows::NCOM::CPropVariant enabled(true);
        QCOMPARE(properties->SetProperties(&name, &enabled, 1), S_OK);
        CMyComPtr<IInStream> source = new NtfsTestSupport::Input(bytes_);
        QCOMPARE(archive_->Open(source, nullptr, nullptr), S_OK);
        QString limitations;
        QVERIFY(readLimitations(limitations));
        QVERIFY(limitations.contains("Deleted NTFS records"));
        bool geometry = true, handled = false;
        QTimer automation;
        connect(&automation, &QTimer::timeout, [&] {
            for (auto *widget : QApplication::topLevelWidgets()) {
                auto *message = qobject_cast<QMessageBox *>(widget);
                if (!message || message->objectName() != "filesystemReadLimitationsWarning")
                    continue;
                message->layout()->activate();
                for (auto *label : message->findChildren<QLabel *>())
                    if (label->isVisible())
                        geometry &= UiInspection::fullyVisible(*label, *message) &&
                                    UiInspection::labelFits(*label);
                message->setProperty("uiInspection_language", language);
                message->setProperty("uiInspection_state", "deleted-recovery");
                geometry &= UiInspection::writeReport(*message, "filesystem-deleted-" + language);
                handled = true;
                if (accept)
                    message->findChild<QPushButton *>("deviceOpenWithoutRecoveryButton")->click();
                else
                    message->button(QMessageBox::Cancel)->click();
            }
        });
        automation.start(10);
        QCOMPARE(ConfirmLimitedFilesystemRead(nullptr, limitations), accept);
        automation.stop();
        QVERIFY(handled);
        QVERIFY(geometry);
        QCOMPARE(archive_->Close(), S_OK);
        archive_.Release();
    }
    void confirmsAndCopiesFlaggedVolume_data() { flaggedVolumeData("en"); }
    void confirmsAndCopiesFlaggedVolumeRu_data() { flaggedVolumeData("ru"); }

    void confirmsPartialSquashfsIndexes_data() {
        QTest::addColumn<QString>("language");
        QTest::addColumn<bool>("accept");
        QTest::addColumn<QString>("kind");
        for (const QString &language : {"en", "ru"})
            for (bool accept : {false, true})
                for (const QString &kind :
                     {"squashfs-xattr-index-outside", "squashfs-table-indexes-combined"})
                    QTest::newRow(
                        qPrintable(language + '-' + kind + (accept ? "-accept" : "-cancel")))
                        << language << accept << kind;
    }
    void confirmsPartialSquashfsIndexes() {
        QFETCH(QString, language);
        QFETCH(bool, accept);
        QFETCH(QString, kind);
        confirmAndCopy(language, accept, kind);
    }

    void confirmsPartialSquashfsInodes_data() {
        QTest::addColumn<QString>("language");
        QTest::addColumn<bool>("accept");
        QTest::addColumn<QString>("kind");
        for (const QString &language : {"en", "ru"})
            for (bool accept : {false, true})
                for (const QString &kind :
                     {"squashfs-inode-record", "squashfs-inode-unused",
                      "squashfs-inode-block-header", "squashfs-inode-block-zlib",
                      "squashfs-blocklist-header", "squashfs-blocklist-zlib"}) {
                    if (!accept && kind != "squashfs-inode-record")
                        continue;
                    QTest::newRow(
                        qPrintable(language + '-' + kind + (accept ? "-accept" : "-cancel")))
                        << language << accept << kind;
                }
    }
    void confirmsPartialSquashfsInodes() {
        QFETCH(QString, language);
        QFETCH(bool, accept);
        QFETCH(QString, kind);
        confirmAndCopy(language, accept, kind);
    }

  private:
    void flaggedVolumeData(const QString &language) {
        QTest::addColumn<QString>("language");
        QTest::addColumn<bool>("accept");
        QTest::addColumn<QString>("kind");
        for (bool accept : {false, true})
            for (const QString &kind : {"exfat",
                                        "wrapper-no-slock",
                                        "unclean",
                                        "free-overflow",
                                        "classic-unclean-free-overflow",
                                        "volume-dirty",
                                        "volume-short",
                                        "volume-list-nonresident",
                                        "reparse-list-bad-id",
                                        "ea-record-small",
                                        "data-list-unreadable-list",
                                        "standard-short",
                                        "ext-recover",
                                        "ext-orphan",
                                        "ext-unclean",
                                        "ext-unverified",
                                        "ext-combined",
                                        "dir-root-missing",
                                        "record-total",
                                        "security-root-offset",
                                        "security-sd-short",
                                        "security-list-bad-id",
                                        "source-prefix",
                                        "source-partial-data",
                                        "source-backup-used",
                                        "source-backup-mismatch"}) {
                // Rejection uses the shared consent service; acceptance checks every reason.
                if (!accept && kind != "exfat" && kind != "unclean" && kind != "volume-dirty" &&
                    kind != "ext-recover")
                    continue;
                QTest::newRow(qPrintable(language + (accept ? "-accept-" : "-cancel-") + kind))
                    << language << accept << kind;
            }
    }
  private slots:
    void confirmsAndCopiesFlaggedVolume() {
        QFETCH(QString, language);
        QFETCH(bool, accept);
        QFETCH(QString, kind);
        confirmAndCopy(language, accept, kind);
    }
    void confirmsAndCopiesFlaggedVolumeRu() {
        QFETCH(QString, language);
        QFETCH(bool, accept);
        QFETCH(QString, kind);
        confirmAndCopy(language, accept, kind);
    }
    void confirmsPartialSecurityStream_data() {
        QTest::addColumn<QString>("language");
        for (const QString &language : {"en", "ru"})
            QTest::newRow(qPrintable(language)) << language;
    }
    void confirmsPartialSecurityStream() {
        QFETCH(QString, language);
        confirmAndCopy(language, true, "security-sds-map-missing-tail");
    }
    void confirmsLimitedFat_data() {
        QTest::addColumn<QString>("language");
        QTest::addColumn<QString>("kind");
        QTest::addColumn<bool>("accept");
        for (const QString &language : {"en", "ru"}) {
            for (const QString &kind :
                 {"fat-dirty12", "fat-dirty32", "fat-short16", "fat-combined32"})
                QTest::newRow(qPrintable(language + '-' + kind)) << language << kind << true;
            QTest::newRow(qPrintable(language + "-cancel-fat"))
                << language << QString("fat-dirty12") << false;
        }
    }
    void confirmsLimitedFat() {
        QFETCH(QString, language);
        QFETCH(QString, kind);
        QFETCH(bool, accept);
        confirmAndCopy(language, accept, kind);
    }
    void confirmsPartialFatDirectory_data() {
        QTest::addColumn<QString>("language");
        QTest::addColumn<QString>("kind");
        QTest::addColumn<bool>("accept");
        for (const QString &language : {"en", "ru"}) {
            for (const QString &kind : {"fat-directory-free32", "fat-directory-start12"})
                QTest::newRow(qPrintable(language + '-' + kind)) << language << kind << true;
            QTest::newRow(qPrintable(language + "-cancel-directory"))
                << language << QString("fat-directory-free32") << false;
        }
    }
    void confirmsPartialFatDirectory() {
        QFETCH(QString, language);
        QFETCH(QString, kind);
        QFETCH(bool, accept);
        confirmAndCopy(language, accept, kind);
    }
    void confirmsPartialFatFile_data() {
        QTest::addColumn<QString>("language");
        QTest::addColumn<QString>("kind");
        QTest::addColumn<bool>("accept");
        for (const QString &language : {"en", "ru"}) {
            for (const QString &kind : {"fat-file-free-32", "fat-file-eof-12", "fat-file-cycle-16"})
                QTest::newRow(qPrintable(language + '-' + kind)) << language << kind << true;
            QTest::newRow(qPrintable(language + "-cancel-file"))
                << language << QString("fat-file-free-32") << false;
        }
    }
    void confirmsPartialFatFile() {
        QFETCH(QString, language);
        QFETCH(QString, kind);
        QFETCH(bool, accept);
        confirmAndCopy(language, accept, kind);
    }
    void confirmsLimitedCramfs_data() {
        QTest::addColumn<QString>("language");
        QTest::addColumn<bool>("accept");
        QTest::addColumn<QString>("kind");
        for (const QString &language : {"en", "ru"})
            for (const QString &kind :
                 {"cramfs-crc", "cramfs-short", "cramfs-clean", "cramfs-directory",
                  "cramfs-directory-cycle", "cramfs-counts", "cramfs-size", "cramfs-table"})
                for (bool accept : {false, true}) {
                    if (!accept && kind != "cramfs-crc" && kind != "cramfs-directory" &&
                        kind != "cramfs-size" && kind != "cramfs-table")
                        continue;
                    QTest::newRow(
                        qPrintable(language + '-' + kind + (accept ? "-accept" : "-cancel")))
                        << language << accept << kind;
                }
    }
    void confirmsLimitedCramfs() {
        QFETCH(QString, language);
        QFETCH(bool, accept);
        QFETCH(QString, kind);
        confirmAndCopy(language, accept, kind);
    }
    void confirmsPartialSquashfsAttributes_data() {
        QTest::addColumn<QString>("language");
        QTest::addColumn<bool>("accept");
        QTest::addColumn<QString>("kind");
        for (const QString &language : {"en", "ru"})
            for (bool accept : {false, true})
                for (const QString &kind :
                     {"squashfs", "squashfs-id-metadata-zero", "squashfs-fragment-metadata-zero",
                      "squashfs-metadata-combined"})
                    QTest::newRow(
                        qPrintable(language + '-' + kind + (accept ? "-accept" : "-cancel")))
                        << language << accept << kind;
    }
    void confirmsPartialSquashfsAttributes() {
        QFETCH(QString, language);
        QFETCH(bool, accept);
        QFETCH(QString, kind);
        confirmAndCopy(language, accept, kind);
    }

    void confirmsPartialSquashfsDirectory_data() {
        QTest::addColumn<QString>("language");
        QTest::addColumn<bool>("accept");
        QTest::addColumn<QString>("kind");
        for (const QString &language : {"en", "ru"})
            for (bool accept : {false, true})
                for (const QString &kind :
                     {"squashfs-directory-tail", "squashfs-directory-block-span",
                      "squashfs-directory-reference", "squashfs-directory-unavailable",
                      "squashfs-directory-index-header", "squashfs-directory-index-inline-name"}) {
                    if (!accept && kind != "squashfs-directory-tail")
                        continue;
                    QTest::newRow(
                        qPrintable(language + '-' + kind + (accept ? "-accept" : "-cancel")))
                        << language << accept << kind;
                }
    }
    void confirmsPartialSquashfsDirectory() {
        QFETCH(QString, language);
        QFETCH(bool, accept);
        QFETCH(QString, kind);
        confirmAndCopy(language, accept, kind);
    }

  private:
    void confirmAndCopy(const QString &language, bool accept, const QString &kind) {
        const bool ntfs = kind.startsWith("volume-") || kind.startsWith("reparse-") ||
                          kind.startsWith("ea-") || kind.startsWith("data-list-") ||
                          kind.startsWith("standard-") || kind.startsWith("dir-") ||
                          kind.startsWith("record-") || kind.startsWith("security-") ||
                          kind.startsWith("source-");
        const bool ext = kind.startsWith("ext-");
        const bool fat = kind.startsWith("fat-");
        const bool squashfs = kind.startsWith("squashfs");
        const bool cramfs = kind.startsWith("cramfs-");
        const bool lateBlocklist = kind.startsWith("squashfs-blocklist-");
        const bool noOpeningLimitations = lateBlocklist || kind == "cramfs-clean" ||
                                          kind == "cramfs-counts" ||
                                          kind.startsWith("squashfs-inode-block-") ||
                                          kind.startsWith("squashfs-directory-index-");
        const bool squashFragment =
            kind == "squashfs-fragment-metadata-zero" || kind == "squashfs-metadata-combined";
        const bool fatDirectory = kind.startsWith("fat-directory-");
        const bool fatFile = kind.startsWith("fat-file-");
        const bool copyDiagnostic = kind == "cramfs-table" || fatFile || squashFragment ||
                                    lateBlocklist || kind == "squashfs-directory-reference";
        const QString fatFolder = kind == "fat-directory-free32" ? "BROKEN" : "GOOD";
        SaveRegLang(UString(language == "en" ? L"-" : L"ru"),
                    qEnvironmentVariable("SEVENZIP_SETTINGS_PATH"));
        TexfatFixture fixture(language == "en" ? 0 : 1);
        fixture.image[106] = char(language == "en" ? 2 : 5);
        QByteArray image = fixture.image;
        QString filename = fixture.fragmentedName;
        QByteArray payload = QByteArray(512, 'A') + QByteArray(188, 'B');
        if (kind != "exfat") {
            QProcess generator;
            generator.start(
                QStringLiteral(SEVENZIP_HFS_PYTHON),
                fat   ? QStringList{QStringLiteral(SEVENZIP_FAT_FIXTURE),
                                    fatDirectory ? "--directory=" + kind.mid(14)
                                    : fatFile    ? "--file-chain=" + kind.mid(9)
                                                 : "--readonly=" + kind.mid(4),
                                    "--bundle"}
                : ext ? QStringList{QStringLiteral(SEVENZIP_EXT_FIXTURE), "readonly-" + kind.mid(4),
                                    "--bundle"}
                : squashfs
                    ? QStringList{QStringLiteral(SEVENZIP_SQUASHFS_FIXTURE), "--xattrs",
                                  "--fault=" + (kind == "squashfs" ? QString("xattr-ool-outside")
                                                                   : kind.mid(9))}
                : cramfs ? QStringList{QStringLiteral(SEVENZIP_CRAMFS_FIXTURE), kind.mid(7)}
                : ntfs   ? QStringList{QStringLiteral(SEVENZIP_NTFS_FIXTURE), "--kind=" + kind}
                         : QStringList{QStringLiteral(SEVENZIP_HFS_FIXTURE), kind});
            QVERIFY(generator.waitForFinished(10000));
            QCOMPARE(generator.exitCode(), 0);
            const QJsonObject data =
                QJsonDocument::fromJson(generator.readAllStandardOutput()).object();
            QByteArray packed(4, '\0');
            qToBigEndian(quint32(data["size"].toInt()), packed.data());
            packed += QByteArray::fromBase64(data["image"].toString().toLatin1());
            image = qUncompress(packed);
            QVERIFY(!image.isEmpty());
            filename = (ext || fat)                   ? data["name"].toString()
                       : (ntfs || squashfs || cramfs) ? QStringLiteral("alpha")
                                                      : QStringLiteral("HfsFixture/alpha");
            if (fatDirectory)
                filename = fatFolder + (fatFolder == "BROKEN" ? "/PREFIX.TXT" : "/GOOD.TXT");
            if (kind == "squashfs-directory-unavailable" || kind == "squashfs-inode-record" ||
                kind.startsWith("squashfs-inode-block-") ||
                kind == "squashfs-table-indexes-combined")
                filename = "данные 空";
            if (ext) {
                payload = QByteArray(data["file_size"].toInt(), '\0');
                payload.replace(0, 4, "head");
                payload.replace(payload.size() - 4, 4, "tail");
            } else
                payload = QByteArray::fromBase64(
                    data["files"].toObject()[filename].toString().toLatin1());
            if (squashFragment)
                payload = payload.left(8192);
            if (lateBlocklist)
                payload = payload.left(8192);
        }
        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        const QString path = directory.filePath(QStringLiteral("том 空.img"));
        QFile source(path);
        QVERIFY(source.open(QIODevice::WriteOnly));
        QCOMPARE(source.write(image), qint64(image.size()));
        source.close();
        ArchiveBackend backend(nullptr, engine_);
        QSignalSpy ready(&backend, &ArchiveBackend::codecCatalogReady);
        QVERIFY(backend.discoverCodecCatalog(module_));
        QTRY_COMPARE(ready.size(), 1);
        QVERIFY(ready[0][0].toBool());
        MainWindow window(&backend);
        window.openPath(directory.path());
        window.show();
        int questions = 0;
        bool geometry = true;
        QStringList unexpectedErrors;
        int incompleteCopyErrors = 0;
        bool copyingIncomplete = false;
        QPointer<QMessageBox> handled;
        QTimer automation;
        connect(&automation, &QTimer::timeout, &window, [&] {
            for (QWidget *widget : QApplication::topLevelWidgets()) {
                if (widget->isVisible() && widget->objectName() == "progressDialog") {
                    auto *close = widget->findChild<QPushButton *>("IDCANCEL");
                    if (TestProgressDialog::IsCloseButton(close)) {
                        const auto *messages =
                            widget->findChild<QTreeWidget *>("IDL_PROGRESS_MESSAGES");
                        if (copyingIncomplete && messages && messages->topLevelItemCount() > 0)
                            ++incompleteCopyErrors;
                        else
                            unexpectedErrors.append("Unexpected extraction diagnostics");
                        close->click();
                        return;
                    }
                }
                if (widget->isVisible() && widget->objectName() == "operationMessagesDialog") {
                    const auto *messages = widget->findChild<QTreeWidget *>("IDL_MESSAGE");
                    if (copyingIncomplete && messages && messages->topLevelItemCount() > 0)
                        ++incompleteCopyErrors;
                    else
                        unexpectedErrors.append("Unexpected extraction diagnostics");
                    if (auto *close = widget->findChild<QPushButton *>("IDOK"))
                        close->click();
                    return;
                }
            }
            QMessageBox *message = nullptr;
            for (QWidget *widget : QApplication::topLevelWidgets())
                if (auto *box = qobject_cast<QMessageBox *>(widget);
                    box && box != handled &&
                    (box->isVisible() || box->objectName() == "filesystemReadLimitationsWarning")) {
                    message = box;
                    break;
                }
            if (!message)
                return;
            handled = message;
            if (message->objectName() != "filesystemReadLimitationsWarning") {
                if (copyingIncomplete)
                    ++incompleteCopyErrors;
                else
                    unexpectedErrors.append(message->text());
                message->reject();
                return;
            }
            ++questions;
            message->layout()->activate();
            for (auto *label : message->findChildren<QLabel *>())
                if (label->isVisible())
                    geometry &= UiInspection::fullyVisible(*label, *message) &&
                                UiInspection::labelFits(*label);
            message->setProperty("uiInspection_language", language);
            message->setProperty("uiInspection_state", kind);
            geometry &= UiInspection::writeReport(*message, "filesystem-limitations-" + language +
                                                                '-' + kind);
            if (accept)
                message->findChild<QPushButton *>("deviceOpenWithoutRecoveryButton")->click();
            else
                QTest::keyClick(message, language == "en" ? Qt::Key_Return : Qt::Key_Escape);
        });
        automation.start(10);
        window.openArchive(path);
        QVERIFY2(unexpectedErrors.isEmpty(), qPrintable(unexpectedErrors.join('\n')));
        QCOMPARE(questions, noOpeningLimitations ? 0 : 1);
        QVERIFY(geometry);
        auto *badge = window.findChild<QLabel *>("deviceReadOnlyLabel");
        QVERIFY(badge);
        if (!accept) {
            QVERIFY(!badge->isVisible());
        } else {
            if (noOpeningLimitations) {
                QVERIFY(!badge->isVisible());
            } else {
                QTRY_VERIFY(badge->isVisible());
                QCOMPARE(badge->text(), QCoreApplication::translate("mainwindow_panel",
                                                                    "Read only · limited access"));
                for (QSize size : {QSize(980, 620), QSize(640, 400)}) {
                    window.resize(size);
                    QCoreApplication::processEvents();
                    QVERIFY(UiInspection::fullyVisible(*badge, window));
                    QVERIFY(UiInspection::labelFits(*badge));
                    window.setProperty("uiInspection_language", language);
                    QVERIFY(UiInspection::writeReport(window, "filesystem-limited-" + language +
                                                                  '-' +
                                                                  QString::number(size.width())));
                }
            }
            auto *table = window.findChild<QTableWidget *>("archiveTable");
            QVERIFY(table);
            // HFS starts at its volume root; descend through the public UI.
            if (fatDirectory) {
                QTRY_VERIFY(Row(*table, fatFolder) >= 0);
                table->selectRow(Row(*table, fatFolder));
                QVERIFY(QMetaObject::invokeMethod(table, "cellDoubleClicked", Qt::DirectConnection,
                                                  Q_ARG(int, Row(*table, fatFolder)),
                                                  Q_ARG(int, 0)));
                filename = fatFolder == "BROKEN" ? "PREFIX.TXT" : "GOOD.TXT";
            }
            if (kind != "exfat" && !ntfs && !ext && !fat && !squashfs && !cramfs) {
                QTRY_VERIFY(Row(*table, "HfsFixture") >= 0);
                const int row = Row(*table, "HfsFixture");
                table->selectRow(row);
                QVERIFY(QMetaObject::invokeMethod(table, "cellDoubleClicked", Qt::DirectConnection,
                                                  Q_ARG(int, row), Q_ARG(int, 0)));
                filename = "alpha";
            }
            QTRY_VERIFY(Row(*table, filename) >= 0);
            table->selectRow(Row(*table, filename));
            for (const char *name : {"deleteAction", "renameAction", "moveAction"})
                QVERIFY(!window.findChild<QAction *>(name)->isEnabled());
            const QString output = directory.filePath("copied");
            ExtractionOptions copyOptions;
            copyOptions.pathMode = ExtractionOptions::PathMode::CurrentPaths;
            if (kind != "exfat" && !ntfs && !ext && !fat && !squashfs && !cramfs)
                copyOptions.currentFolder = "HfsFixture";
            else if (fatDirectory)
                copyOptions.currentFolder = fatFolder;
            copyingIncomplete = copyDiagnostic;
            window.extractTo(output, true, {}, copyOptions);
            copyingIncomplete = false;
            QCOMPARE(incompleteCopyErrors, copyDiagnostic ? 1 : 0);
            QVERIFY2(unexpectedErrors.isEmpty(), qPrintable(unexpectedErrors.join('\n')));
            QFile copied(output + '/' + filename);
            QVERIFY2(
                copied.open(QIODevice::ReadOnly),
                qPrintable(
                    QDir(output).entryList(QDir::AllEntries | QDir::NoDotAndDotDot).join('\n')));
            QCOMPARE(copied.readAll(), payload);
            if (fatFile) {
                QTRY_VERIFY(Row(*table, "SECOND.BIN") >= 0);
                table->selectRow(Row(*table, "SECOND.BIN"));
                window.extractTo(output, true, {}, copyOptions);
                QFile healthy(output + "/SECOND.BIN");
                QVERIFY(healthy.open(QIODevice::ReadOnly));
                QByteArray expected;
                for (int i = 0; i < 1100; ++i)
                    expected.append(char((i * 17 + 23) & 255));
                expected += "second tail\n";
                QCOMPARE(healthy.readAll(), expected);
            }
            if (squashFragment) {
                QTRY_VERIFY(Row(*table, "данные 空") >= 0);
                table->selectRow(Row(*table, "данные 空"));
                window.extractTo(output, true, {}, copyOptions);
                QFile healthy(output + "/данные 空");
                QVERIFY(healthy.open(QIODevice::ReadOnly));
                QCOMPARE(healthy.readAll(), QByteArray(8192, 'B') + QByteArray(8192, '\0'));
            }
            if (kind == "reparse-list-bad-id" || kind == "ea-record-small" ||
                kind == "data-list-unreadable-list" || kind == "standard-short" ||
                kind == "dir-root-missing" || kind.startsWith("security-") ||
                kind == "source-prefix") {
                QTRY_VERIFY(Row(*table, "gamma") >= 0);
                table->selectRow(Row(*table, "gamma"));
                window.extractTo(output, true, {}, copyOptions);
                QFile raw(output + "/gamma");
                QVERIFY(raw.open(QIODevice::ReadOnly));
                QCOMPARE(raw.readAll(), QByteArray(65536, 'B'));
            }
            window.findChild<QAction *>("refreshAction")->trigger();
            QTRY_VERIFY(Row(*table, filename) >= 0);
            QCOMPARE(questions, noOpeningLimitations ? 0 : 1);
            QCOMPARE(badge->isVisible(), !noOpeningLimitations);
            window.openPath(directory.path());
            QVERIFY(!badge->isVisible());
            // Change our fixture after closing the old Windows read lease.
            if (kind == "exfat") {
                image[106] = char(quint8(image[106]) | 6u);
                QFile changed(path);
                QVERIFY(changed.open(QIODevice::ReadWrite));
                QVERIFY(changed.seek(106));
                QCOMPARE(changed.write(image.constData() + 106, 1), qint64(1));
                changed.close();
            }
            window.openArchive(path);
            QCOMPARE(questions, noOpeningLimitations ? 0 : 2);
        }
        automation.stop();
        QVERIFY2(unexpectedErrors.isEmpty(), qPrintable(unexpectedErrors.join('\n')));
        QVERIFY(source.open(QIODevice::ReadOnly));
        QCOMPARE(source.readAll(), image);
    }
};

QTEST_MAIN(FilesystemReadUiTests)
#include "filesystem_read_ui_tests.moc"
