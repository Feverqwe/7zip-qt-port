#include "Common/MyTypes.h"

#include "7zip/UI/Explorer/ContextMenuFlags.h"
#include "7zip/UI/FileManager/ProgressDialog2Res.h"
#include "CPP/7zip/UI/FileManager/ProgressDialog2.h"
#include "archivecodeccatalog.h"
#include "createarchivedialog.h"
#include "fixtures/widget_dialogs.h"
#include "layout_inspection.h"
#include "source_extract_fixture.h"
#include "window_state_support.h"
#include <QScrollArea>
#include <QScrollBar>

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
struct CCurrentDirectory {
    const QString Path = QDir::currentPath();
    ~CCurrentDirectory() { QDir::setCurrent(Path); }
};
int rowNamed(MainWindow &window, const QString &name) {
    auto *view = window.findChild<QTableWidget *>("archiveTable");
    for (int row = 0; row < view->rowCount(); ++row)
        if (view->item(row, 0)->text() == name)
            return row;
    return -1;
}
void prepareSettings(const QString &language = "-") {
    const QString settings = qEnvironmentVariable("SEVENZIP_SETTINGS_PATH");
    QFile::remove(settings);
    SaveRegLang(ToUString(language), settings);
    CContextMenuInfo ci;
    ci.Flags = NContextMenuFlags::kCompress | NContextMenuFlags::kCompressTo7z |
               NContextMenuFlags::kCompressToZip | NContextMenuFlags::kCRC_Cascaded;
    ci.Flags_Def = true;
    ci.Cascaded.SetTrueTrue();
    ci.MenuIcons.SetTrueTrue();
    ci.Save(settings);
}
} // namespace

class ShellUpdateWindowTests final : public QObject, protected MainWindowTestSupport {
    Q_OBJECT
    WidgetDialogFixture dialogs_;
    QString _module, _engine;
    quint32 _version = 0;
  private slots:
    void initTestCase() {
#ifndef SEVENZIP_CODEC_MODULE
        QSKIP("Official native runtime unavailable");
#else
        _module = QString::fromUtf8(SEVENZIP_CODEC_MODULE);
        _engine = QStandardPaths::findExecutable("7zz");
        QVERIFY(!_engine.isEmpty());
        ArchiveCodecCatalog probe(_module, 0);
        QCOMPARE(probe.result(), S_OK);
        _version = probe.version();
#endif
    }
    void actualPopupWritesAndPreservesFailedSelection_data() {
        QTest::addColumn<QString>("type");
        QTest::addColumn<bool>("broken");
        QTest::newRow("7z") << QString("7z") << false;
        QTest::newRow("zip") << QString("zip") << false;
        QTest::newRow("sha256") << QString("hash") << false;
        QTest::newRow("broken-existing-7z") << QString("7z") << true;
        QTest::newRow("modal-success") << QString("modal") << false;
        QTest::newRow("modal-broken-existing") << QString("modal") << true;
        QTest::newRow("modal-cancel") << QString("cancel") << false;
        QTest::newRow("toolbar-success") << QString("toolbar") << false;
        QTest::newRow("program-menu-7z") << QString("program-7z") << false;
        QTest::newRow("program-menu-modal") << QString("program-modal") << false;
        QTest::newRow("program-menu-cancel") << QString("program-cancel") << false;
        QTest::newRow("program-menu-broken-existing") << QString("program-7z") << true;
        QTest::newRow("toolbar-broken-existing") << QString("toolbar") << true;
        QTest::newRow("toolbar-cancel") << QString("toolbar-cancel") << false;
        QTest::newRow("toolbar-update-existing") << QString("toolbar-update") << false;
        QTest::newRow("toolbar-new-parent") << QString("toolbar-nested") << false;
        QTest::newRow("toolbar-new-parent-cancel") << QString("toolbar-nested-cancel") << false;
        QTest::newRow("toolbar-sfx") << QString("toolbar-sfx") << false;
        QTest::newRow("toolbar-sfx-cancel") << QString("toolbar-sfx-cancel") << false;
        QTest::newRow("toolbar-sfx-broken-existing") << QString("toolbar-sfx") << true;
        QTest::newRow("toolbar-sfx-protected") << QString("toolbar-sfx-protected") << false;
        QTest::newRow("toolbar-sfx-volumes") << QString("toolbar-sfx-volumes") << false;
        QTest::newRow("toolbar-sfx-volumes-cancel")
            << QString("toolbar-sfx-volumes-cancel") << false;
        for (const QString &format : {"7z", "zip", "gzip", "bzip2", "xz", "tar", "wim"}) {
            QTest::newRow(qPrintable("toolbar-create-" + format))
                << "toolbar-create-" + format << false;
            QTest::newRow(qPrintable("toolbar-create-" + format + "-cancel"))
                << "toolbar-create-" + format + "-cancel" << false;
        }
    }
    void actualPopupWritesAndPreservesFailedSelection() {
        QFETCH(QString, type);
        QFETCH(bool, broken);
        QTemporaryDir fixture;
        CCurrentDirectory guard;
        QString requestedFormat = type.startsWith("toolbar-create-") ? type.mid(15) : QString();
        if (requestedFormat.endsWith("-cancel"))
            requestedFormat.chop(7);
        QString name = QStringLiteral("данные 空 with spaces.bin");
#ifdef _WIN32
        // Original gzip headers use CP_ACP for the stored leaf name.
        if (requestedFormat == "gzip")
            name = "input with spaces.bin";
#endif
        const bool toolbar = type.startsWith("toolbar");
        const bool program = type.startsWith("program");
        const bool cancelled = type == "cancel" || type.endsWith("-cancel");
        const bool modal = type == "modal" || type == "cancel" || toolbar ||
                           type == "program-modal" || type == "program-cancel";
        QString target = type == "hash" ? name + ".sha256"
                                        : name.chopped(4) + '.' + (modal || program ? "7z" : type);
        if (!requestedFormat.isEmpty()) {
            const auto catalog = ArchiveCodecCatalog(_module, _version);
            const int index = catalog.formatIndex(requestedFormat);
            QVERIFY(index >= 0);
            target = name + '.' +
                     ToQString(catalog.nativeCodecs().Formats[unsigned(index)].GetMainExt());
        }
        const bool nested = type.startsWith("toolbar-nested");
        const QString outputFolder = QStringLiteral("новая 空 папка with spaces");
        if (nested)
            target = outputFolder + '/' + target;
        const bool sfx = type.startsWith("toolbar-sfx");
        const bool sfxVolumes = type.contains("sfx-volumes");
        const bool protectedSfx = type == "toolbar-sfx-protected";
        const QByteArray bytes = sfxVolumes ? QByteArray(240 << 10, 'a') : QByteArray("abc");
        if (sfx) {
            target = name.chopped(4);
#ifdef _WIN32
            target += ".exe";
#endif
        }
        QVERIFY(SourceExtractFixture::writeFile(fixture.filePath(name), bytes));
        if (type == "toolbar-update") {
            QVERIFY(SourceExtractFixture::writeFile(fixture.filePath(name), "old"));
            QProcess create;
            create.setWorkingDirectory(fixture.path());
            create.start(_engine, {"a", "-t7z", "--", target, name});
            QVERIFY(create.waitForFinished(10000));
            QCOMPARE(create.exitCode(), 0);
            QVERIFY(SourceExtractFixture::writeFile(fixture.filePath(name), "abc"));
        }
        if (broken)
            QVERIFY(SourceExtractFixture::writeFile(fixture.filePath(target), "sentinel"));
        QVERIFY(QDir::setCurrent(fixture.path()));
        prepareSettings();
        ArchiveBackend backend(nullptr, _engine);
        QCOMPARE(backend.loadCodecCatalog(_module, _version), S_OK);
        MainWindow window(&backend);
        window.show();
        window.activateWindow();
        QVERIFY(QTest::qWaitForWindowActive(&window));
        auto *view = table(window);
        QTRY_VERIFY(rowNamed(window, name) >= 0);
        view->selectRow(rowNamed(window, name));
        QSignalSpy processes(&backend, &ArchiveBackend::engineStarted);
        QStringList errors;
        QTimer dismissal;
        dialogs_.connectResponder(dismissal, &window, [&] {
            if (auto *dialog = qobject_cast<CreateArchiveDialog *>(dialogs_.activeDialog())) {
                QVERIFY(QTest::qWaitForWindowActive(dialog));
                QCOMPARE(dialog->sources(), QStringList{fixture.filePath(name)});
                if (nested)
                    dialog->findChild<QLineEdit *>("archiveName")
                        ->setText(fixture.filePath(target));
                if (sfx) {
                    auto *control = dialog->findChild<QCheckBox *>("archiveSfx");
                    if (!control || !control->isEnabled())
                        dialog->reject();
                    QVERIFY(control && control->isEnabled());
                    control->setChecked(true);
                    dialog->findChild<QLineEdit *>("archiveName")
                        ->setText(fixture.filePath(target));
                    QVERIFY(dialog->sourceInfo().SFXMode);
                    if (protectedSfx) {
                        dialog->findChild<QLineEdit *>("archivePassword")
                            ->setText("fixture-password");
                        dialog->findChild<QLineEdit *>("archivePasswordConfirmation")
                            ->setText("fixture-password");
                        dialog->findChild<QCheckBox *>("archiveEncryptHeaders")->setChecked(true);
                    }
                    if (sfxVolumes) {
                        auto *level = dialog->findChild<QComboBox *>("compressionLevel");
                        level->setCurrentIndex(level->findData(0u));
                        dialog->findChild<QComboBox *>("archiveVolumeSize")->setEditText("100K");
                    }
                    dialog->resize(cancelled ? QSize(640, 480) : QSize(760, 720));
                    QCoreApplication::processEvents();
                    QVERIFY(UiInspection::writeReport(*dialog, "source-" + type));
                    auto *scroll = dialog->findChild<QScrollArea *>("archiveOptionsScrollArea");
                    QVERIFY(scroll);
                    scroll->verticalScrollBar()->setValue(scroll->verticalScrollBar()->maximum());
                    QCoreApplication::processEvents();
                    QVERIFY(UiInspection::fullyVisible(
                        *dialog->findChild<QCheckBox *>("archiveEncryptHeaders"), *dialog));
                    QVERIFY(UiInspection::writeReport(*dialog, "source-" + type + "-bottom"));
                }
                if (!requestedFormat.isEmpty()) {
                    auto *format = dialog->findChild<QComboBox *>("archiveFormat");
                    format->setCurrentText(requestedFormat);
                    QCOMPARE(format->currentText(), requestedFormat);
                    dialog->findChild<QLineEdit *>("archiveName")
                        ->setText(fixture.filePath(target));
                    dialog->resize(cancelled ? QSize(640, 480) : QSize(760, 720));
                    QCoreApplication::processEvents();
                    QVERIFY(UiInspection::writeReport(*dialog, QString("source-toolbar-add-%1-%2")
                                                                   .arg(requestedFormat)
                                                                   .arg(dialog->width())));
                }
                if (cancelled)
                    dialog->reject();
                else
                    dialog->findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Ok)->click();
                return;
            }
            if (auto *box = qobject_cast<QMessageBox *>(dialogs_.activeDialog())) {
                errors.append(box->text());
                box->accept();
                return;
            }
            if (auto *progress = dynamic_cast<CProgressDialog *>(dialogs_.activeDialog())) {
                auto *button = progress->findChild<QPushButton *>("IDCANCEL");
                if (button && button->property("languageID").toUInt() == IDS_CLOSE)
                    button->click();
            }
        });
        dismissal.start(5);
        bool invoked = false;
        if (toolbar) {
            auto *add = action(window, "toolbarAddAction");
            QVERIFY(add->isEnabled());
            invoked = true;
            add->trigger();
        } else if (program) {
            auto *menu = window.findChild<QMenu *>("fileMenu");
            QVERIFY(menu);
            QVERIFY(QMetaObject::invokeMethod(menu, "aboutToShow", Qt::DirectConnection));
            const QString verb = modal ? "SevenZipCompress" : "SevenZipCompressTo7z";
            auto *command = findVerb(*menu, verb);
            QVERIFY(command && command->isEnabled());
            invoked = true;
            command->trigger();
            // Source ExecuteFileCommand releases the context after invoke.
            // The retained QAction must neither dangle nor re-invoke it.
            command->trigger();
        } else {
            QTimer::singleShot(0, [&] {
                auto *menu = qobject_cast<QMenu *>(QApplication::activePopupWidget());
                QVERIFY(menu);
                const QString verb = modal            ? "SevenZipCompress"
                                     : type == "hash" ? "SevenZip.Checksum.Generate.SHA256"
                                     : type == "7z"   ? "SevenZipCompressTo7z"
                                                      : "SevenZipCompressToZip";
                auto *action = findVerb(*menu, verb);
                QVERIFY2(action, qPrintable(verb));
                QVERIFY(action->isEnabled());
                menu->close();
                invoked = true;
                action->trigger();
            });
            QVERIFY(QMetaObject::invokeMethod(
                view, "customContextMenuRequested", Qt::DirectConnection,
                Q_ARG(QPoint, view->visualItemRect(view->item(view->currentRow(), 0)).center())));
        }
        dismissal.stop();
        QVERIFY(invoked);
        QCOMPARE(processes.size(), 0);
        QCOMPARE(view->selectionModel()->selectedRows().size(),
                 toolbar || broken || cancelled ? 1 : 0);
        QCOMPARE(SourceExtractFixture::readFile(fixture.filePath(name)), bytes);
        QVERIFY(!backend.busy());
        if (cancelled) {
            QVERIFY(errors.isEmpty());
            QVERIFY(!QFileInfo::exists(fixture.filePath(target)));
            if (nested)
                QVERIFY(!QFileInfo::exists(fixture.filePath(outputFolder)));
            if (sfxVolumes)
                QVERIFY(QDir(fixture.path()).entryList({"*.001", "*.002"}, QDir::Files).isEmpty());
        } else if (broken) {
            QCOMPARE(errors.size(), 1);
            QCOMPARE(SourceExtractFixture::readFile(fixture.filePath(target)),
                     QByteArray("sentinel"));
        } else {
            QVERIFY(errors.isEmpty());
            QTRY_VERIFY(rowNamed(window, nested ? outputFolder : target) >= 0);
            QProcess verify;
            verify.setWorkingDirectory(fixture.path());
            QString archive = target;
            if (sfxVolumes) {
#ifdef _WIN32
                archive.chop(4); // Source GetFinalVolPath replaces .exe with .7z.
#endif
                archive += ".7z.001";
                QVERIFY(QFileInfo::exists(fixture.filePath(archive.chopped(3) + "002")));
                QCOMPARE(
                    SourceExtractFixture::readFile(fixture.filePath(target)),
                    SourceExtractFixture::readFile(backend.codecCatalog()->sfxModule().path()));
            }
            QStringList testArgs{"t", "--", archive};
            if (protectedSfx)
                testArgs.prepend("-pfixture-password");
            verify.start(_engine, testArgs);
            QVERIFY(verify.waitForFinished(10000));
            QCOMPARE(verify.exitCode(), 0);
            if (toolbar) {
                const QString output = fixture.filePath("verify output");
                QStringList extractArgs{"x", "-y", "-o" + output, "--", archive};
                if (protectedSfx)
                    extractArgs.prepend("-pfixture-password");
                verify.start(_engine, extractArgs);
                QVERIFY(verify.waitForFinished(10000));
                QCOMPARE(verify.exitCode(), 0);
                QCOMPARE(SourceExtractFixture::readFile(QDir(output).filePath(name)), bytes);
            }
            if (sfx) {
                QVERIFY(QFileInfo(fixture.filePath(target)).isExecutable());
                const QString output = fixture.filePath("native SFX output");
                QStringList args{"x", "-y", "-o" + output};
#ifdef _WIN32
                args.removeFirst(); // Original SfxWin accepts -o/-y/-p, without command.
#endif
                if (protectedSfx)
                    args.append("-pfixture-password");
                verify.start(fixture.filePath(target), args);
                QVERIFY(verify.waitForFinished(15000));
                QCOMPARE(verify.exitStatus(), QProcess::NormalExit);
                QVERIFY2(verify.exitCode() == 0,
                         qPrintable(QString::fromUtf8(verify.readAllStandardOutput() +
                                                      verify.readAllStandardError())));
                QCOMPARE(SourceExtractFixture::readFile(QDir(output).filePath(name)), bytes);
            }
        }
    }
    void updateCommandGeometry_data() {
        QTest::addColumn<QString>("language");
        QTest::addColumn<QSize>("size");
        for (const QString &language : {QString("-"), QString("ru")})
            for (const QSize &size : {QSize(980, 620), QSize(640, 400)})
                QTest::newRow(qPrintable(QString("%1-%2").arg(language).arg(size.width())))
                    << language << size;
    }
    void updateCommandGeometry() {
        QFETCH(QString, language);
        QFETCH(QSize, size);
        QTemporaryDir fixture;
        CCurrentDirectory guard;
        const QString name = QStringLiteral("данные 空 with spaces.bin");
        QVERIFY(SourceExtractFixture::writeFile(fixture.filePath(name), "abc"));
        QVERIFY(QDir::setCurrent(fixture.path()));
        prepareSettings(language);
        ArchiveBackend backend(nullptr, _engine);
        QCOMPARE(backend.loadCodecCatalog(_module, _version), S_OK);
        MainWindow window(&backend);
        window.resize(size);
        window.show();
        window.activateWindow();
        QVERIFY(QTest::qWaitForWindowActive(&window));
        auto *view = table(window);
        QTRY_VERIFY(rowNamed(window, name) >= 0);
        view->selectRow(rowNamed(window, name));
        bool inspected = false;
        QTimer::singleShot(0, [&] {
            auto *menu = qobject_cast<QMenu *>(QApplication::activePopupWidget());
            QVERIFY(menu);
            auto *root = menu->findChild<QAction *>("sevenZipContextGroup");
            QVERIFY(root && root->menu());
            auto *compress = findVerb(*menu, "SevenZipCompress");
            auto *sevenZip = findVerb(*menu, "SevenZipCompressTo7z");
            auto *zip = findVerb(*menu, "SevenZipCompressToZip");
            auto *generate = findVerb(*menu, "SevenZip.Checksum.Generate.SHA256");
            QVERIFY(compress && sevenZip && zip && generate);
            QVERIFY(compress->isEnabled());
            QVERIFY(sevenZip->isEnabled() && zip->isEnabled() && generate->isEnabled());
            QMenu *child = root->menu();
            child->popup(menu->mapToGlobal(menu->actionGeometry(root).topRight()));
            QCoreApplication::processEvents();
            const QString tag =
                QString("shell-update-%1-%2").arg(language == "ru" ? "ru" : "en").arg(size.width());
            for (auto *action : child->actions()) {
                const QRect rect = child->actionGeometry(action);
                QVERIFY(child->rect().contains(rect));
                if (!action->isSeparator())
                    QVERIFY(child->fontMetrics().horizontalAdvance(action->text()) < rect.width());
            }
            QVERIFY(UiInspection::writeReport(*child, tag));
            QMenu *hashMenu = nullptr;
            for (auto *action : child->actions())
                if (action->menu() && action->menu()->actions().contains(generate))
                    hashMenu = action->menu();
            QVERIFY(hashMenu);
            hashMenu->popup(
                child->mapToGlobal(child->actionGeometry(hashMenu->menuAction()).topRight()));
            QCoreApplication::processEvents();
            for (auto *action : hashMenu->actions()) {
                QVERIFY(hashMenu->rect().contains(hashMenu->actionGeometry(action)));
                if (!action->isSeparator())
                    QVERIFY(hashMenu->fontMetrics().horizontalAdvance(action->text()) <
                            hashMenu->actionGeometry(action).width());
            }
            QVERIFY(UiInspection::writeReport(*hashMenu, tag + "-hash"));
            hashMenu->close();
            child->close();
            menu->close();
            inspected = true;
        });
        QVERIFY(QMetaObject::invokeMethod(
            view, "customContextMenuRequested", Qt::DirectConnection,
            Q_ARG(QPoint, view->visualItemRect(view->item(view->currentRow(), 0)).center())));
        QVERIFY(inspected);
    }
};
QTEST_MAIN(ShellUpdateWindowTests)
#include "shell_update_window_tests.moc"
