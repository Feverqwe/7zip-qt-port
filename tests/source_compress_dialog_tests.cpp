#include "7zip/UI/FileManager/ProgressDialog2Res.h"
#include "CPP/7zip/UI/FileManager/LangUtils.h"
#include "CPP/7zip/UI/FileManager/ProgressDialog2.h"
#include "CPP/7zip/UI/FileManager/RegistryUtils.h"
#include "CPP/7zip/UI/GUI/UpdateGUI.h"
#include "archivecodeccatalog.h"
#include "createarchivedialog.h"
#include "layout_inspection.h"
#include "qtshellarchiveservices.h"
#include <QScrollArea>
#include <QScrollBar>
#include <QTimer>
#include <QtTest>

class SourceCompressDialogTests final : public QObject {
    Q_OBJECT
    std::shared_ptr<const ArchiveCodecCatalog> _catalog;
    QString _module;
  private slots:
    void initTestCase() {
#ifndef SEVENZIP_CODEC_MODULE
        QSKIP("Official native module unavailable");
#else
        _module = QString::fromUtf8(SEVENZIP_CODEC_MODULE);
        _catalog = std::make_shared<ArchiveCodecCatalog>(_module, 0);
        QCOMPARE(_catalog->result(), S_OK);
#endif
    }
    void fixedSelectionRoundTrip_data() {
        QTest::addColumn<QString>("language");
        QTest::addColumn<QSize>("size");
        for (const QString &lang : {QString("-"), QString("ru")})
            for (const QSize size : {QSize(760, 720), QSize(640, 480)})
                QTest::newRow(qPrintable(lang + QString::number(size.width()))) << lang << size;
    }
    void originalMethodSelection() {
        QFile::remove(qEnvironmentVariable("SEVENZIP_SETTINGS_PATH"));
        QTemporaryDir fixture;
        CreateArchiveDialog dialog(fixture.path(), {}, nullptr, _catalog);
        auto *method = dialog.findChild<QComboBox *>("compressionMethod");
        auto *level = dialog.findChild<QComboBox *>("compressionLevel");
        auto *format = dialog.findChild<QComboBox *>("archiveFormat");
        QVERIFY(method && level && format);
        QCOMPARE(method->currentText(), QString("*  LZMA2"));
        QCOMPARE(method->currentData().toInt(), -1);
        QVERIFY(dialog.sourceInfo().Method.IsEmpty());
        QVERIFY(dialog.compressionOptions().Method.IsEmpty());
        method->setCurrentText("PPMd");
        QCOMPARE(QString::fromWCharArray(dialog.sourceInfo().Method.Ptr()), QString("PPMd"));
        format->setCurrentText("zip");
        QCOMPARE(method->currentText(), QString("*  Deflate"));
        QVERIFY(dialog.sourceInfo().Method.IsEmpty());
        format->setCurrentText("7z");
        QCOMPARE(method->currentText(), QString("PPMd"));
        level->setCurrentIndex(level->findData(0u));
        QCOMPARE(method->count(), 0);
        QVERIFY(!method->isEnabled());
        QVERIFY(dialog.sourceInfo().Method.IsEmpty());
        level->setCurrentIndex(level->findData(5u));
        QCOMPARE(method->currentText(), QString("*  LZMA2"));
        format->setCurrentText("tar");
        QCOMPARE(level->currentData().toUInt(), 0u);
        QVERIFY(method->count() > 1);
        QCOMPARE(method->currentText(), QString("*  GNU"));
        QVERIFY(dialog.sourceInfo().Method.IsEmpty());
        method->setCurrentText("POSIX");
        QCOMPARE(QString::fromWCharArray(dialog.sourceInfo().Method.Ptr()), QString("POSIX"));
    }
    void fixedSelectionRoundTrip() {
        QFETCH(QString, language);
        QFETCH(QSize, size);
        const QString settings = qEnvironmentVariable("SEVENZIP_SETTINGS_PATH");
        QFile::remove(settings);
        SaveRegLang(UString(language.toStdWString().c_str()), settings);
        ReloadLang(settings);
        QTemporaryDir fixture;
        const QString path = fixture.filePath(QStringLiteral("данные 空 with spaces.txt"));
        QFile file(path);
        QVERIFY(file.open(QIODevice::WriteOnly));
        QCOMPARE(file.write("abc"), qint64(3));
        file.close();
        CCompressDialog source;
        source.Info.FormatIndex = _catalog->formatIndex("7z");
        source.Info.ArcPath = fixture.filePath("chosen").toStdWString().c_str();
        source.Info.Password = "initial";
        source.Info.UpdateMode = NCompressDialog::NUpdateMode::kFresh;
        source.Info.PathMode = NWildcard::k_FullPath;
        NWildcard::CCensor censor;
        censor.AddPreItem_NoWildcard(UString(path.toStdWString().c_str()));
        QCOMPARE(NCompressDialog::PrepareFormatSelection(source, _catalog->nativeCodecs(),
                                                         censor.CensorPaths),
                 S_OK);
        CreateArchiveDialog dialog(fixture.path(), {path}, nullptr, _catalog, &source);
        dialog.resize(size);
        dialog.show();
        QCoreApplication::processEvents();
        auto *name = dialog.findChild<QLineEdit *>("archiveName");
        QVERIFY(name);
        QCOMPARE(name->selectedText(), name->text());
        QVERIFY(UiInspection::writeReport(
            dialog,
            QString("source-add-%1-%2").arg(language == "ru" ? "ru" : "en").arg(size.width())));
        QVERIFY(!dialog.findChild<QListWidget *>("availableSources"));
        QVERIFY(!dialog.findChild<QLineEdit *>("sourceDirectory"));
        QVERIFY(!dialog.findChild<QLineEdit *>("archiveDestination"));
        QVERIFY(!dialog.findChild<QLabel *>("archiveTargetPreview"));
        auto *folder = dialog.findChild<QLabel *>("archiveFolder");
        auto *archiveRow = dialog.findChild<QWidget *>("archivePathRow");
        QVERIFY(folder && archiveRow);
        // IDT_COMPRESS_ARCHIVE_FOLDER and IDC_COMPRESS_ARCHIVE share the
        // xArcFolderOffs column; the Archive caption occupies the left column.
        QCOMPARE(UiInspection::bounds(*folder, dialog).left(),
                 UiInspection::bounds(*archiveRow, dialog).left());
        auto *caption = dialog.findChild<QLabel *>("archiveLabel");
        QVERIFY(caption);
        const int captionCenter = UiInspection::bounds(*caption, dialog).center().y();
        QVERIFY(captionCenter > UiInspection::bounds(*folder, dialog).center().y());
        QVERIFY(captionCenter < UiInspection::bounds(*archiveRow, dialog).center().y());
        const auto formatBounds =
            UiInspection::bounds(*dialog.findChild<QComboBox *>("archiveFormat"), dialog);
        // Desktop g1x/g1xs align the six compression combo rows. Memory's
        // full-column caption must not force ordinary controls onto new rows.
        for (const char *control :
             {"compressionLevel", "compressionMethod", "compressionDictionary",
              "compressionWordSize", "compressionSolidBlock"}) {
            const auto bounds = UiInspection::bounds(*dialog.findChild<QWidget *>(control), dialog);
            QCOMPARE(bounds.left(), formatBounds.left());
            QCOMPARE(bounds.width(), formatBounds.width());
        }
        QCOMPARE(dialog.sources(), QStringList{path});
        auto *mode = dialog.findChild<QComboBox *>("archiveUpdateMode");
        auto *pathMode = dialog.findChild<QComboBox *>("archivePathMode");
        QVERIFY(mode && pathMode);
        auto *left = dialog.findChild<QWidget *>("archiveCompressionColumn");
        auto *right = dialog.findChild<QWidget *>("archiveEncryptionColumn");
        QVERIFY(left && right);
        // Desktop IDD_COMPRESS: format/compression/volumes/parameters left;
        // update/path modes and Options/Encryption right, without a left group.
        for (const char *name :
             {"archiveFormat", "compressionLevel", "compressionMethod", "compressionDictionary",
              "compressionWordSize", "compressionSolidBlock", "compressionThreads",
              "archiveVolumeSize", "archiveParameters", "archiveAdvancedOptions"})
            QVERIFY2(left->isAncestorOf(dialog.findChild<QWidget *>(name)), name);
        for (const char *name : {"archiveUpdateMode", "archivePathMode", "archiveLifecycleOptions",
                                 "archiveEncryptionGroup"})
            QVERIFY2(right->isAncestorOf(dialog.findChild<QWidget *>(name)), name);
        QVERIFY(!dialog.findChild<QGroupBox *>("archiveCompressionGroup"));
        QVERIFY(left->geometry().right() < right->geometry().left());
        QCOMPARE(mode->currentData().toInt(), int(NCompressDialog::NUpdateMode::kFresh));
        QCOMPARE(pathMode->currentData().toInt(), int(NWildcard::k_FullPath));
        QVERIFY(UiInspection::fullyVisible(*mode, dialog));
        QVERIFY(UiInspection::fullyVisible(*pathMode, dialog));
        auto *buttons = dialog.findChild<QDialogButtonBox *>();
        QVERIFY(buttons && UiInspection::fullyVisible(*buttons, dialog));
        auto *scroll = dialog.findChild<QScrollArea *>("archiveOptionsScrollArea");
        QVERIFY(scroll);
        QCOMPARE(scroll->horizontalScrollBar()->maximum(), 0);
        scroll->verticalScrollBar()->setValue(scroll->verticalScrollBar()->maximum());
        QCoreApplication::processEvents();
        QVERIFY(UiInspection::fullyVisible(*dialog.findChild<QLineEdit *>("archiveParameters"),
                                           dialog));
        QVERIFY(UiInspection::writeReport(dialog, QString("source-add-%1-%2-bottom")
                                                      .arg(language == "ru" ? "ru" : "en")
                                                      .arg(size.width())));
        mode->setCurrentIndex(mode->findData(int(NCompressDialog::NUpdateMode::kUpdate)));
        dialog.findChild<QLineEdit *>("archivePassword")->setText("edited");
        dialog.findChild<QLineEdit *>("archivePasswordConfirmation")->setText("edited");
        // Existing output is intentionally allowed: source UpdateArchive owns
        // update/refusal, while this screen owns presentation and confirmation.
        QFile target(fixture.filePath("chosen.7z"));
        QVERIFY(target.open(QIODevice::WriteOnly));
        target.write("sentinel");
        target.close();
        buttons->button(QDialogButtonBox::Ok)->click();
        QCOMPARE(dialog.result(), int(QDialog::Accepted));
        const auto result = dialog.sourceInfo();
        QCOMPARE(result.UpdateMode, NCompressDialog::NUpdateMode::kUpdate);
        QCOMPARE(result.Password, QString("edited"));
        QCOMPARE(result.FormatIndex, _catalog->formatIndex("7z"));
        QCOMPARE(QString::fromWCharArray(result.ArcPath.Ptr()), fixture.filePath("chosen.7z"));
        QCOMPARE(source.Info.Password, QString("initial"));
        QVERIFY(target.open(QIODevice::ReadOnly));
        QCOMPARE(target.readAll(), QByteArray("sentinel"));
    }
    void actualSourceModalOperation_data() {
        QTest::addColumn<bool>("cancel");
        QTest::addColumn<bool>("broken");
        QTest::addColumn<bool>("deleteSources");
        QTest::addColumn<bool>("unsupportedDelete");
        QTest::newRow("success") << false << false << false << false;
        QTest::newRow("broken-existing") << false << true << false << false;
        QTest::newRow("modal-cancel") << true << false << false << false;
        QTest::newRow("delete-success") << false << false << true << false;
        QTest::newRow("delete-cancel") << true << false << true << false;
        QTest::newRow("delete-incompatible-mode") << false << false << true << true;
    }
    void actualSourceModalOperation() {
        QFETCH(bool, cancel);
        QFETCH(bool, broken);
        QFETCH(bool, deleteSources);
        QFETCH(bool, unsupportedDelete);
        QTemporaryDir fixture;
        const QString path = fixture.filePath(QStringLiteral("данные 空 with spaces.txt"));
        QFile source(path);
        QVERIFY(source.open(QIODevice::WriteOnly));
        source.write("abc");
        source.close();
        const QString output = fixture.filePath("actual.7z");
        if (broken) {
            QFile target(output);
            QVERIFY(target.open(QIODevice::WriteOnly));
            target.write("sentinel");
        }
        ArchiveBackend backend;
        QCOMPARE(backend.loadCodecCatalog(_module, _catalog->version()), S_OK);
        bool shown = false;
        unsigned errors = 0;
        QtShellArchiveServices service(
            backend, nullptr, {}, {}, [&](HRESULT, const UString &) { errors++; },
            [&](CCompressDialog &dialog, const CObjectVector<NWildcard::CCensorPath> &paths,
                QWidget *parent) {
                return CreateArchiveDialog::PresentSource(dialog, paths, backend.codecCatalog(),
                                                          parent);
            });
        QTimer dismissal;
        connect(&dismissal, &QTimer::timeout, [&] {
            if (auto *dialog =
                    qobject_cast<CreateArchiveDialog *>(QApplication::activeModalWidget())) {
                if (!shown) {
                    shown = true;
                    if (unsupportedDelete) {
                        auto *mode = dialog->findChild<QComboBox *>("archiveUpdateMode");
                        mode->setCurrentIndex(
                            mode->findData(int(NCompressDialog::NUpdateMode::kFresh)));
                    }
                    auto *remove = dialog->findChild<QCheckBox *>("archiveDeleteAfterCompression");
                    QVERIFY(remove && remove->isEnabled());
                    remove->setChecked(deleteSources);
                    if (cancel)
                        dialog->reject();
                    else
                        dialog->findChild<QDialogButtonBox *>()
                            ->button(QDialogButtonBox::Ok)
                            ->click();
                }
            } else if (auto *box = qobject_cast<QMessageBox *>(QApplication::activeModalWidget())) {
                errors++;
                box->accept();
            } else if (auto *progress =
                           dynamic_cast<CProgressDialog *>(QApplication::activeModalWidget())) {
                auto *button = progress->findChild<QPushButton *>("IDCANCEL");
                if (button && button->property("languageID").toUInt() == IDS_CLOSE)
                    button->click();
            }
        });
        dismissal.start(5);
        UStringVector paths;
        paths.Add(UString(path.toStdWString().c_str()));
        service.CompressFiles(UString((fixture.path() + '/').toStdWString().c_str()),
                              UString(L"actual.7z"), UString(L"7z"), false, paths, false, true,
                              false);
        QVERIFY(shown);
        if (cancel) {
            QCOMPARE(service.Result(), E_ABORT);
            QVERIFY(!QFileInfo::exists(output));
            QCOMPARE(errors, 0u);
        } else if (broken) {
            QVERIFY(service.Result() != S_OK);
            QCOMPARE(errors, 1u);
            QFile target(output);
            QVERIFY(target.open(QIODevice::ReadOnly));
            QCOMPARE(target.readAll(), QByteArray("sentinel"));
        } else if (unsupportedDelete) {
            QVERIFY(service.Result() != S_OK);
            QCOMPARE(errors, 1u);
            QVERIFY(!QFileInfo::exists(output));
        } else {
            QCOMPARE(service.Result(), S_OK);
            QCOMPARE(errors, 0u);
            QProcess verify;
            verify.start(QStandardPaths::findExecutable("7zz"), {"t", "--", output});
            QVERIFY(verify.waitForFinished(10000));
            QCOMPARE(verify.exitCode(), 0);
        }
        if (deleteSources && !cancel && !broken && !unsupportedDelete)
            QVERIFY(!QFileInfo::exists(path));
        else {
            QVERIFY(source.open(QIODevice::ReadOnly));
            QCOMPARE(source.readAll(), QByteArray("abc"));
        }
        QVERIFY(!backend.busy());
    }
};
QTEST_MAIN(SourceCompressDialogTests)
#include "source_compress_dialog_tests.moc"
