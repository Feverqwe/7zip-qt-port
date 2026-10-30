#include "Common/MyTypes.h"
#include "filemanager/views/filetable.h"

#include "7zip/UI/Explorer/ContextMenuFlags.h"
#include "7zip/UI/FileManager/ProgressDialog2Res.h"
#include "archive/codecs/archivecodeccatalog.h"
#include "archive/support/source_extract_fixture.h"
#include "dialogs/compress/createarchivedialog.h"
#include "filemanager/support/window_state_support.h"
#include "support/native_mapi_loader.h"
#include "ui/progress/ProgressDialog2.h"
#include "ui/support/layout_inspection.h"
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMenuBar>
#include <QScopeGuard>

namespace {
QAction *findVerb(QMenu &menu, const QString &verb) {
    for (auto *action : menu.actions()) {
        if (action->property("shellVerb").toString() == verb)
            return action;
        if (action->menu())
            if (auto *found = findVerb(*action->menu(), verb))
                return found;
    }
    return nullptr;
}
int rowNamed(MainWindow &window, const QString &name) {
    auto *view = window.findChild<FileTable *>("archiveTable");
    for (int row = 0; row < view->rowCount(); ++row)
        if (view->model()->index(row, 0).data().toString() == name)
            return row;
    return -1;
}
void settings(const QString &language) {
    const QString path = qEnvironmentVariable("SEVENZIP_SETTINGS_PATH");
    QFile::remove(path);
    SaveRegLang(ToUString(language), path);
    CContextMenuInfo info;
    info.Flags = NContextMenuFlags::kCompressEmail | NContextMenuFlags::kCompressTo7zEmail |
                 NContextMenuFlags::kCompressToZipEmail;
    info.Flags_Def = true;
    info.Cascaded.SetTrueTrue();
    info.Save(path);
    CFmSettings fm;
    fm.Load(path);
    fm.ShowSystemMenu = false;
    fm.Save(path);
}
} // namespace

class NativeShellEmailTests final : public QObject, protected MainWindowTestSupport {
    Q_OBJECT
    QString module, engine;
    quint32 version = 0;
    void checkGeometry(QMenu &menu, const QString &tag) {
        for (auto *action : menu.actions()) {
            const QRect rect = menu.actionGeometry(action);
            QVERIFY(menu.rect().contains(rect));
            if (!action->isSeparator())
                QVERIFY(menu.fontMetrics().horizontalAdvance(action->text()) < rect.width());
        }
        QVERIFY(UiInspection::writeReport(menu, tag));
    }

  private slots:
    void initTestCase() {
        module = QString::fromUtf8(SEVENZIP_CODEC_MODULE);
        engine = QStandardPaths::findExecutable("7z");
        QVERIFY(!engine.isEmpty());
        ArchiveCodecCatalog probe(module, 0);
        QCOMPARE(probe.result(), S_OK);
        version = probe.version();
    }
    void fileEmail_data() {
        QTest::addColumn<QString>("verb");
        QTest::addColumn<QString>("language");
        QTest::addColumn<QSize>("size");
        QTest::addColumn<bool>("cancel");
        QTest::addColumn<bool>("missing");
        for (const QString &verb :
             {"SevenZipCompressEmail", "SevenZipCompressTo7zEmail", "SevenZipCompressToZipEmail"})
            for (const QString &language : {"-", "ru"})
                for (const QSize &size : {QSize(980, 620), QSize(640, 400)})
                    QTest::newRow(qPrintable(verb + language + QString::number(size.width())))
                        << verb << language << size << false << false;
        QTest::newRow("cancel") << QString("SevenZipCompressEmail") << QString("-")
                                << QSize(640, 400) << true << false;
        QTest::newRow("missing-provider-export")
            << QString("SevenZipCompressToZipEmail") << QString("-") << QSize(640, 400) << false
            << true;
    }
    void fileEmail() {
        QFETCH(QString, verb);
        QFETCH(QString, language);
        QFETCH(QSize, size);
        QFETCH(bool, cancel);
        QFETCH(bool, missing);
        QTemporaryDir fixture;
        QVERIFY(fixture.isValid());
        const QString cwd = QDir::currentPath();
        const auto restoreCwd = qScopeGuard([&] { QDir::setCurrent(cwd); });
        QVERIFY(QDir::setCurrent(fixture.path()));
        const QString name = QStringLiteral("данные 空 with spaces.bin");
        const QByteArray bytes("source File email bytes");
        QVERIFY(SourceExtractFixture::writeFile(fixture.filePath(name), bytes));
        const QString report = fixture.filePath("report.json");
        const QString clientCwd = fixture.filePath("mail-client-cwd");
        QVERIFY(QDir().mkpath(clientCwd));
        QMap<QByteArray, QByteArray> previous;
        for (const QByteArray &key :
             {"SEVENZIP_MAPI_TEST_REPORT", "SEVENZIP_MAPI_TEST_CWD", "SEVENZIP_MAPI_TEST_CANCEL"})
            previous.insert(key, qgetenv(key));
        const auto restoreEnvironment = qScopeGuard([&] {
            for (auto it = previous.cbegin(); it != previous.cend(); ++it)
                if (it.value().isNull())
                    qunsetenv(it.key());
                else
                    qputenv(it.key(), it.value());
        });
        qputenv("SEVENZIP_MAPI_TEST_REPORT", report.toUtf8());
        qputenv("SEVENZIP_MAPI_TEST_CWD", clientCwd.toUtf8());
        qputenv("SEVENZIP_MAPI_TEST_CANCEL", "0");
        HMODULE provider = LoadPrivateMAPIProvider(missing ? QString(NATIVE_MAPI_MISSING)
                                                           : QString(NATIVE_MAPI_WIDE));
        QVERIFY(provider);
        const auto unload = qScopeGuard([&] { FreeLibrary(provider); });
        settings(language);
        ArchiveBackend backend(nullptr, engine);
        QCOMPARE(backend.loadCodecCatalog(module, version), S_OK);
        MainWindow window(&backend);
        window.resize(size);
        window.show();
        auto *view = table(window);
        QTRY_VERIFY(rowNamed(window, name) >= 0);
        view->selectRow(rowNamed(window, name));
        QStringList errors;
        QTimer dismissal;
        connect(&dismissal, &QTimer::timeout, [&] {
            if (auto *dialog =
                    qobject_cast<CreateArchiveDialog *>(QApplication::activeModalWidget())) {
                if (cancel)
                    dialog->reject();
                else
                    dialog->findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Ok)->click();
            } else if (auto *box = qobject_cast<QMessageBox *>(QApplication::activeModalWidget())) {
                errors.append(box->text());
                box->accept();
            } else if (auto *progress =
                           dynamic_cast<CProgressDialog *>(QApplication::activeModalWidget())) {
                auto *close = progress->findChild<QPushButton *>("IDCANCEL");
                if (close && close->property("languageID").toUInt() == IDS_CLOSE)
                    close->click();
            }
        });
        dismissal.start(5);
        auto *menu = window.findChild<QMenu *>("fileMenu");
        QVERIFY(menu);
        menu->popup(window.mapToGlobal(QPoint(0, window.menuBar()->height())));
        QCoreApplication::processEvents();
        auto *email = findVerb(*menu, verb);
        QVERIFY(email);
        QVERIFY(email->isEnabled());
        auto *root = menu->findChild<QAction *>("sevenZipContextGroup");
        QVERIFY(root && root->menu());
        root->menu()->popup(menu->mapToGlobal(menu->actionGeometry(root).topRight()));
        QCoreApplication::processEvents();
        checkGeometry(*root->menu(),
                      QString("shell-email-%1-%2-%3").arg(verb).arg(language).arg(size.width()));
        root->menu()->hide();
        menu->hide();
        email->trigger();
        dismissal.stop();
        QCOMPARE(QDir::currentPath(), fixture.path());
        QCOMPARE(SourceExtractFixture::readFile(fixture.filePath(name)), bytes);
        if (!cancel && !missing)
            QVERIFY2(errors.isEmpty(), qPrintable(errors.join("\n")));
        QCOMPARE(view->selectionModel()->selectedRows().size(), cancel || missing ? 1 : 0);
        if (cancel || missing) {
            QVERIFY(!QFileInfo::exists(report));
            QCOMPARE(errors.isEmpty(), cancel);
        } else {
            QVERIFY(errors.isEmpty());
            const auto captured =
                QJsonDocument::fromJson(SourceExtractFixture::readFile(report)).object();
            const auto files = captured["files"].toArray();
            QCOMPARE(files.size(), 1);
            QVERIFY(files[0].toObject()["copied"].toBool());
            QProcess extract;
            const QString output = fixture.filePath("verify-output");
            extract.start(engine, {"x", "-y", "-o" + output, report + ".attachment-0.zip"});
            QVERIFY(extract.waitForFinished(10000));
            QCOMPARE(extract.exitCode(), 0);
            QCOMPARE(SourceExtractFixture::readFile(QDir(output).filePath(name)), bytes);
        }
    }
};

QTEST_MAIN(NativeShellEmailTests)
#include "native_shell_email_tests.moc"
