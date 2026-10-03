#include "CPP/7zip/UI/Agent/Agent.h"
#include "CPP/7zip/UI/FileManager/FileFolderPluginOpen.h"
#include "CPP/7zip/UI/FileManager/LangUtils.h"
#include "CPP/7zip/UI/FileManager/RegistryUtils.h"
#include "createarchivedialog.h"
#include "layout_inspection.h"
#include "native_security_fixture.h"
#include "qtarchivefileresource.h"
#include "qtarchivefoldermanager.h"
#include "source_extract_fixture.h"
#include "source_metadata_update_fixture.h"
#include <QScopeGuard>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTimer>
#include <QtTest>

using namespace SourceExtractFixture;

class SourceMetadataStorageTests final : public QObject {
    Q_OBJECT
  private slots:
    void optionsTransactionsAndGeometry_data() {
        QTest::addColumn<QString>("language");
        QTest::addColumn<QSize>("size");
        for (const QString &language : {QString("-"), QString("ru")})
            for (const QSize size : {QSize(540, 480), QSize(480, 430)})
                QTest::newRow(qPrintable(language + QString::number(size.width())))
                    << language << size;
    }
    void optionsTransactionsAndGeometry() {
#if defined(Q_OS_WIN) && defined(SEVENZIP_CODEC_MODULE)
        QFETCH(QString, language);
        QFETCH(QSize, size);
        QTemporaryDir fixture;
        const QByteArray oldSettings = qgetenv("SEVENZIP_SETTINGS_PATH");
        const QString settings = fixture.filePath("options.ini");
        qputenv("SEVENZIP_SETTINGS_PATH", settings.toUtf8());
        const auto restore = qScopeGuard([&] {
            if (oldSettings.isNull())
                qunsetenv("SEVENZIP_SETTINGS_PATH");
            else
                qputenv("SEVENZIP_SETTINGS_PATH", oldSettings);
            ReloadLang();
        });
        SaveRegLang(UString(language.toStdWString().c_str()), settings);
        ReloadLang(settings);
        const QString source = fixture.filePath("source.txt");
        QVERIFY(writeFile(source, "payload"));
        ArchiveBackend backend;
        QSignalSpy ready(&backend, &ArchiveBackend::codecCatalogReady);
        QVERIFY(backend.discoverCodecCatalog(QString::fromUtf8(SEVENZIP_CODEC_MODULE)));
        QTRY_COMPARE(ready.size(), 1);
        QVERIFY(ready[0][0].toBool());
        const auto configure = [&](CreateArchiveDialog &form, bool accept) {
            form.findChild<QComboBox *>("archiveFormat")->setCurrentText("wim");
            QTimer::singleShot(0, &form, [&, accept] {
                auto *dialog = qobject_cast<QDialog *>(QApplication::activeModalWidget());
                QVERIFY(dialog);
                const auto close = qScopeGuard([&] {
                    if (dialog->isVisible())
                        dialog->reject();
                });
                dialog->resize(size);
                QApplication::processEvents();
                QVERIFY(UiInspection::writeReport(
                    *dialog, QString("source-metadata-wim-%1-%2").arg(language).arg(size.width())));
                QCOMPARE(dialog->size(), size);
                for (const char *name : {"IDX_COMPRESS_NT_ALT_STREAMS", "IDX_COMPRESS_NT_SECUR"}) {
                    auto *control = dialog->findChild<QCheckBox *>(name);
                    QVERIFY(control && control->isVisible() && control->isEnabled());
                    QVERIFY(UiInspection::fullyVisible(*control, *dialog));
                    QVERIFY(control->width() >= control->sizeHint().width());
                    control->setChecked(true);
                }
                for (const char *name : {"IDX_COMPRESS_MTIME", "IDX_COMPRESS_CTIME",
                                         "IDX_COMPRESS_ATIME", "IDX_COMPRESS_PRESERVE_ATIME"}) {
                    auto *control = dialog->findChild<QCheckBox *>(name);
                    QVERIFY(control && UiInspection::fullyVisible(*control, *dialog));
                    QVERIFY(control->width() >= control->sizeHint().width());
                }
                auto *buttons = dialog->findChild<QDialogButtonBox *>("compressOptionsButtons");
                QVERIFY(buttons && UiInspection::fullyVisible(*buttons, *dialog));
                if (accept)
                    buttons->button(QDialogButtonBox::Ok)->click();
            });
            form.findChild<QPushButton *>("archiveAdvancedOptions")->click();
        };
        CreateArchiveDialog cancelled(fixture.path(), {source}, nullptr, {},
                                      backend.codecCatalog());
        configure(cancelled, false);
        QVERIFY(!cancelled.compressionOptions().AltStreams.Val);
        QVERIFY(!cancelled.compressionOptions().NtSecurity.Val);
        configure(cancelled, true);
        QVERIFY(cancelled.compressionOptions().AltStreams.Val);
        QVERIFY(cancelled.compressionOptions().NtSecurity.Val);
        cancelled.reject();
        NCompression::CInfo saved;
        saved.Load(settings);
        QVERIFY(!saved.AltStreams.Val && !saved.NtSecurity.Val);
        CreateArchiveDialog accepted(fixture.path(), {source}, nullptr, {}, backend.codecCatalog());
        configure(accepted, true);
        accepted.findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Ok)->click();
        QCOMPARE(accepted.result(), int(QDialog::Accepted));
        saved.Load(settings);
        QVERIFY(saved.AltStreams.Val && saved.NtSecurity.Val);
        CreateArchiveDialog restarted(fixture.path(), {source}, nullptr, {},
                                      backend.codecCatalog());
        QCOMPARE(restarted.findChild<QComboBox *>("archiveFormat")->currentText(), QString("wim"));
        QVERIFY(restarted.compressionOptions().AltStreams.Val);
        QVERIFY(restarted.compressionOptions().NtSecurity.Val);
        restarted.reject();
#else
        QSKIP("Windows metadata controls require Windows and the official module.");
#endif
    }
    void nativeAddStoresStreamsAndSecurity_data() {
        QTest::addColumn<bool>("security");
        QTest::addColumn<bool>("streams");
        QTest::addColumn<HRESULT>("failure");
        QTest::newRow("store") << true << true << HRESULT(S_OK);
        QTest::newRow("security-only") << true << false << HRESULT(S_OK);
        QTest::newRow("streams-only") << false << true << HRESULT(S_OK);
        QTest::newRow("omit") << false << false << HRESULT(S_OK);
        QTest::newRow("failure") << true << true << HRESULT(E_FAIL);
        QTest::newRow("cancel") << true << true << HRESULT(E_ABORT);
    }
    void nativeAddStoresStreamsAndSecurity() {
#if defined(Q_OS_WIN) && defined(SEVENZIP_CODEC_MODULE)
        QFETCH(bool, security);
        QFETCH(bool, streams);
        QFETCH(HRESULT, failure);
        QTemporaryDir fixture;
        QVERIFY(fixture.isValid());
        const QString engine = QStandardPaths::findExecutable("7zz");
        QVERIFY(!engine.isEmpty());
        const QString archive = fixture.filePath("native metadata.wim");
        QVERIFY(writeFile(fixture.filePath("keep.txt"), "original"));
        QProcess creator;
        creator.setWorkingDirectory(fixture.path());
        creator.start(engine, {"a", "-twim", "--", archive, "keep.txt"});
        QVERIFY(creator.waitForFinished(10000));
        QCOMPARE(creator.exitCode(), 0);
        const QByteArray original = readFile(archive);
        const QString rootName = QStringLiteral("источник с пробелом");
        const QString root = fixture.filePath(rootName);
        const QString name = QStringLiteral("данные δ.bin");
        const QString stream = QStringLiteral("поток δ");
        const QString source = QDir(root).filePath(name);
        const QByteArray payload(1024 * 1024, 'x');
        QVERIFY(writeFile(source, payload));
        QVERIFY(writeFile(source + ':' + stream, "stream bytes"));
        QVERIFY(SetFixturePublicDacl(source));
        const QString ace = QStringLiteral("(A;;FA;;;WD)");
        QVERIFY(FixtureDacl(source).contains(ace));
        ArchiveBackend backend(nullptr, engine);
        QSignalSpy ready(&backend, &ArchiveBackend::codecCatalogReady);
        QVERIFY(backend.discoverCodecCatalog(QString::fromUtf8(SEVENZIP_CODEC_MODULE)));
        QTRY_COMPARE(ready.size(), 1);
        QVERIFY(ready[0][0].toBool());
        CMyComPtr<IFolderManager> manager = new QtArchiveFolderManager(backend);
        CFfpOpen opening(manager);
        QCOMPARE(opening.OpenFileFolderPlugin(nullptr, us2fs(archive.toStdWString().c_str()),
                                              UString(), nullptr),
                 S_OK);
        auto *folder = dynamic_cast<CAgentFolder *>(opening.Folder.Interface());
        QVERIFY(folder);
        auto *resource = dynamic_cast<QtArchiveFileResource *>(folder->GetEngineResource());
        QVERIFY(resource && resource->HasUpdateService());
        QCOMPARE(folder->LoadItems(), S_OK);
        CompressionOptions options;
        options.level = 0;
        options.NtSecurity.Def = options.AltStreams.Def = true;
        options.NtSecurity.Val = security;
        options.AltStreams.Val = streams;
        options.DeleteAfterCompressing = security || streams;
        resource->SetCompressionOptions(options);
        CMyComPtr<IFolderOperations> operations;
        QCOMPARE(opening.Folder.QueryInterface(IID_IFolderOperations, &operations), S_OK);
        CLinkUpdateWorker worker(operations, root);
        worker.Failure = failure;
        worker.DisableUserQuestions = true;
        auto transport = resource->CreateProgressTransport(worker);
        QVERIFY(transport);
        QCOMPARE(worker.Create(L"Source metadata storage", nullptr), S_OK);
        QVERIFY2(
            worker.Result == failure,
            qPrintable(QString("HRESULT=%1; %2; %3; archive unchanged=%4; source exists=%5")
                           .arg(quint32(worker.Result), 8, 16, QChar('0'))
                           .arg(QString::fromWCharArray(worker.GetMessagePair(true).Message.Ptr()))
                           .arg(QString::fromWCharArray(worker.GetMessagePair(true).Title.Ptr()))
                           .arg(readFile(archive) == original)
                           .arg(QFileInfo::exists(source))));
        if (failure != S_OK) {
            QVERIFY(worker.Observed);
            QCOMPARE(readFile(archive), original);
            QCOMPARE(readFile(source), payload);
            QCOMPARE(readFile(source + ':' + stream), QByteArray("stream bytes"));
            QVERIFY(FixtureDacl(source).contains(ace));
            return;
        }
        QCOMPARE(QFileInfo::exists(root), !(security || streams));
        const QString output = fixture.filePath("output");
        QProcess extractor;
        extractor.start(engine, {"x", "-y", "-sni", "-sns", "-o" + output, "--", archive});
        QVERIFY(extractor.waitForFinished(10000));
        QCOMPARE(extractor.exitCode(), 0);
        QCOMPARE(readFile(QDir(output).filePath("keep.txt")), QByteArray("original"));
        const QString extracted = QDir(output).filePath(rootName + '/' + name);
        QCOMPARE(readFile(extracted), payload);
        QCOMPARE(readFile(extracted + ':' + stream),
                 streams ? QByteArray("stream bytes") : QByteArray());
        const QString dacl = FixtureDacl(extracted);
        QVERIFY(!dacl.isEmpty());
        QCOMPARE(dacl.contains(ace), security);
#else
        QSKIP("Native filesystem metadata storage requires Windows and the official module.");
#endif
    }
};
QTEST_MAIN(SourceMetadataStorageTests)
#include "source_metadata_storage_tests.moc"
