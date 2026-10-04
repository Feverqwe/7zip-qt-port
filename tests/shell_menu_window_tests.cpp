#include "Common/MyTypes.h"

#include "7zip/UI/Explorer/ContextMenuFlags.h"
#include "CPP/7zip/UI/FileManager/PasswordDialog.h"
#include "archivecodeccatalog.h"
#include "layout_inspection.h"
#include "window_state_support.h"
#include <QClipboard>
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
struct CurrentDirectoryGuard {
    const QString path = QDir::currentPath();
    ~CurrentDirectoryGuard() { QDir::setCurrent(path); }
};
bool write(const QString &path, const QByteArray &bytes) {
    QFile file(path);
    return file.open(QIODevice::WriteOnly) && file.write(bytes) == bytes.size();
}
int rowNamed(MainWindow &window, const QString &name) {
    auto *view = window.findChild<QTableWidget *>("archiveTable");
    for (int row = 0; row < view->rowCount(); ++row)
        if (view->item(row, 0)->text() == name)
            return row;
    return -1;
}
} // namespace

class ShellMenuWindowTests final : public QObject, protected MainWindowTestSupport {
    Q_OBJECT
    QString engine_, module_;
    quint32 version_ = 0;

  private slots:
#ifdef Q_OS_WIN
    void nativeSystemGroupUsesSelectedPathsAndRejectsStaleActions_data() {
        QTest::addColumn<QString>("language");
        QTest::addColumn<QSize>("size");
        for (const QString &language : {QString("-"), QString("ru")})
            for (const QSize &size : {QSize(980, 620), QSize(640, 400)})
                QTest::newRow(qPrintable(QString("%1-%2").arg(language).arg(size.width())))
                    << language << size;
    }
    void nativeSystemGroupUsesSelectedPathsAndRejectsStaleActions() {
        QFETCH(QString, language);
        QFETCH(QSize, size);
        QTemporaryDir fixture;
        QVERIFY(fixture.isValid());
        CurrentDirectoryGuard guard;
        const QString name = QStringLiteral("native System файл 密碼.txt");
        QVERIFY(write(fixture.filePath(name), "native context bytes"));
        QVERIFY(QDir::setCurrent(fixture.path()));
        const QString settings = qEnvironmentVariable("SEVENZIP_SETTINGS_PATH");
        QFile::remove(settings);
        SaveRegLang(ToUString(language), settings);
        CFmSettings fm;
        fm.Load(settings);
        fm.ShowSystemMenu = true;
        fm.Save(settings);
        ArchiveBackend backend;
        MainWindow window(&backend);
        window.resize(size);
        window.show();
        QTRY_VERIFY(rowNamed(window, name) >= 0);
        table(window)->selectRow(rowNamed(window, name));
        auto *menu = window.findChild<QMenu *>("fileMenu");
        menu->popup(window.mapToGlobal(QPoint(0, window.menuBar()->height())));
        QApplication::processEvents();
        auto *system = menu->findChild<QMenu *>();
        for (auto *row : menu->actions())
            if (row->objectName() == "systemContextGroup")
                system = row->menu();
        QVERIFY(system);
        QCOMPARE(menu->actions().first()->objectName(), QString("systemContextGroup"));
        QVERIFY(!system->actions().isEmpty());
        system->popup(menu->mapToGlobal(menu->actionGeometry(menu->actions().first()).topRight()));
        QApplication::processEvents();
        QVERIFY(UiInspection::writeReport(
            *menu, QString("native-system-root-%1-%2").arg(language).arg(size.width())));
        QVERIFY(UiInspection::writeReport(
            *system, QString("native-system-child-%1-%2").arg(language).arg(size.width())));
        system->hide();
        menu->hide();
        // Exercise a real native numeric verb on disposable files without
        // launching another app or changing an installed shell registration.
        QAction *copy = findVerb(*system, QStringLiteral("copy"));
        QVERIFY(copy && copy->isEnabled());
        const auto clearClipboard = qScopeGuard([] { QApplication::clipboard()->clear(); });
        QApplication::clipboard()->clear();
        QTRY_VERIFY(!backend.busy());
        copy->trigger();
        QTRY_COMPARE(QApplication::clipboard()->mimeData()->urls().size(), 1);
        QCOMPARE(QDir::fromNativeSeparators(
                     QApplication::clipboard()->mimeData()->urls().front().toLocalFile()),
                 fixture.filePath(name));
        QCOMPARE(table(window)->selectionModel()->selectedRows().size(), 0);
        QPointer<QAction> old;
        for (auto *row : system->actions())
            if (!row->menu() && row->isEnabled() && row->property("shellCommandID").toUInt()) {
                old = row;
                break;
            }
        QVERIFY(old);
        QVERIFY(QMetaObject::invokeMethod(menu, "aboutToShow", Qt::DirectConnection));
        // It is retained until deferred deletion but cannot invoke a newly
        // generated native verb (which might otherwise launch or delete).
        QVERIFY(old);
        old->trigger();
        QVERIFY(QFileInfo(fixture.filePath(name)).isFile());
        QVERIFY(!QApplication::activeModalWidget());
        QFile::remove(fixture.filePath(name));
        QVERIFY(QMetaObject::invokeMethod(menu, "aboutToShow", Qt::DirectConnection));
        for (auto *row : menu->actions())
            QVERIFY(row->objectName() != "systemContextGroup");
    }
#endif
    void initTestCase() {
#ifndef SEVENZIP_CODEC_MODULE
        QSKIP("Official native runtime unavailable; native shell menu remains unverified");
#else
        engine_ = QStandardPaths::findExecutable("7zz");
        QVERIFY(!engine_.isEmpty());
        module_ = QString::fromUtf8(SEVENZIP_CODEC_MODULE);
        ArchiveBackend backend(nullptr, engine_);
        QSignalSpy ready(&backend, &ArchiveBackend::codecCatalogReady);
        QVERIFY(backend.discoverCodecCatalog(module_));
        QTRY_COMPARE(ready.size(), 1);
        QVERIFY(ready[0][0].toBool());
        version_ = backend.codecCatalog()->version();
#endif
    }
    void nativeOpenKeepsExplicitTypeAndCancellation_data() {
        QTest::addColumn<QString>("outcome");
        QTest::addColumn<bool>("program");
        for (const bool program : {false, true})
            for (const QString &outcome :
                 {QString("success"), QString("wrong-type"), QString("cancel")})
                QTest::newRow(
                    qPrintable(QString("%1-%2").arg(program ? "program" : "popup", outcome)))
                    << outcome << program;
    }
    void programMenuRebuildRejectsOldActionsAndTracksPanel() {
        QTemporaryDir fixture;
        QVERIFY(fixture.isValid());
        CurrentDirectoryGuard guard;
        const QString name = QStringLiteral("данные 空 with space.txt");
        QVERIFY(write(fixture.filePath(name), "program context bytes"));
        const QString other = fixture.filePath("other panel");
        QVERIFY(QDir().mkpath(other));
        QVERIFY(QDir::setCurrent(fixture.path()));
        const QString settings = qEnvironmentVariable("SEVENZIP_SETTINGS_PATH");
        QFile::remove(settings);
        SavePanelPath(1, ToUString(other), settings);
        CContextMenuInfo ci;
        ci.Flags = NContextMenuFlags::kCompressTo7z;
        ci.Flags_Def = true;
        ci.Cascaded.SetTrueTrue();
        ci.Save(settings);
        ArchiveBackend backend(nullptr, engine_);
        QCOMPARE(backend.loadCodecCatalog(module_, version_), S_OK);
        MainWindow window(&backend);
        window.show();
        auto *view = table(window);
        QTRY_VERIFY(rowNamed(window, name) >= 0);
        view->selectRow(rowNamed(window, name));
        view->setFocus();
        auto *menu = window.findChild<QMenu *>("fileMenu");
        QVERIFY(menu);
        const auto activate = [&] {
            return QMetaObject::invokeMethod(menu, "aboutToShow", Qt::DirectConnection);
        };
        QVERIFY(activate());
        QPointer<QAction> old = findVerb(*menu, "SevenZipCompressTo7z");
        QVERIFY(old && old->isEnabled());
        const int count = menu->actions().size();
        QVERIFY(activate());
        QCOMPARE(menu->actions().size(), count);
        auto *current = findVerb(*menu, "SevenZipCompressTo7z");
        QVERIFY(current && current != old);
        // An obsolete Qt action remains alive until DeferredDelete, but must
        // never dispatch the freshly retained command at the same numeric ID.
        old->trigger();
        QVERIFY(!QFileInfo::exists(fixture.filePath(QFileInfo(name).completeBaseName() + ".7z")));
        action(window, "twoPanelsAction")->trigger();
        auto *right = window.findChild<QWidget *>("filePanel1");
        QVERIFY(right);
        auto *rightView = right->findChild<QTableWidget *>("archiveTable");
        QVERIFY(rightView);
        rightView->setFocus();
        QTRY_VERIFY(rightView->hasFocus());
        current->trigger(); // The retained selection belongs to the other panel.
        QVERIFY(!QFileInfo::exists(fixture.filePath(QFileInfo(name).completeBaseName() + ".7z")));
        QVERIFY(activate());
        QVERIFY(!findVerb(*menu, "SevenZipCompressTo7z"));
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        QVERIFY(old.isNull());
        QVERIFY(!backend.busy());
    }
    void contextStructureAndGeometry_data() {
        QTest::addColumn<QString>("language");
        QTest::addColumn<QSize>("size");
        QTest::addColumn<bool>("program");
        for (const QString &language : {QString("-"), QString("ru")})
            for (const QSize &size : {QSize(980, 620), QSize(640, 400)})
                for (const bool program : {false, true})
                    QTest::newRow(qPrintable(QString("%1-%2-%3")
                                                 .arg(language)
                                                 .arg(size.width())
                                                 .arg(program ? "program" : "popup")))
                        << language << size << program;
    }
    void contextStructureAndGeometry() {
        QFETCH(QString, language);
        QFETCH(QSize, size);
        QFETCH(bool, program);
        QTemporaryDir fixture;
        QVERIFY(fixture.isValid());
        CurrentDirectoryGuard guard;
        const QString name = QStringLiteral("архив 空 with space.7z");
        QVERIFY(write(fixture.filePath(name), "menu fixture only"));
        QVERIFY(QDir::setCurrent(fixture.path()));
        const QString settings = qEnvironmentVariable("SEVENZIP_SETTINGS_PATH");
        QFile::remove(settings);
        SaveRegLang(ToUString(language), settings);
        CContextMenuInfo ci;
        ci.Flags = NContextMenuFlags::kOpen | NContextMenuFlags::kOpenAs |
                   NContextMenuFlags::kExtract | NContextMenuFlags::kTest |
                   NContextMenuFlags::kCompress;
        ci.Flags_Def = true;
        ci.Cascaded.SetTrueTrue();
        ci.MenuIcons.SetTrueTrue();
        ci.Save(settings);
        ArchiveBackend backend(nullptr, engine_);
        QCOMPARE(backend.loadCodecCatalog(module_, version_), S_OK);
        MainWindow window(&backend);
        window.resize(size);
        window.show();
        auto *view = table(window);
        QTRY_VERIFY(rowNamed(window, name) >= 0);
        view->selectRow(rowNamed(window, name));
        bool inspected = false;
        const auto inspect = [&](QMenu *menu) {
            QVERIFY(menu);
            auto *root = menu->findChild<QAction *>("sevenZipContextGroup");
            QVERIFY(root && root->menu() && root->isEnabled());
            QVERIFY(!root->icon().isNull());
            auto *open = findVerb(*menu, "SevenZip.Open.7z");
            auto *test = findVerb(*menu, "SevenZipTest");
            auto *compress = findVerb(*menu, "SevenZipCompress");
            QVERIFY(open && test && compress);
            QVERIFY(open->isEnabled() && test->isEnabled());
            QVERIFY(compress->isEnabled());
            if (program) {
                QCOMPARE(menu->actions().first(), root);
                const auto actions = menu->actions();
                QVERIFY(actions.indexOf(root) < actions.indexOf(action(window, "openItemAction")));
                QVERIFY(actions.indexOf(action(window, "openItemAction")) <
                        actions.indexOf(action(window, "selectedPropertiesAction")));
                QCOMPARE(actions.last(), action(window, "exitAction"));
                for (const char *name :
                     {"openArchiveAction", "createArchiveAction", "addArchiveAction",
                      "extractSelectedAction", "operationJournalAction", "builtinTextViewAction",
                      "builtinTextEditAction"})
                    QVERIFY(!actions.contains(window.findChild<QAction *>(name)));
                QVERIFY(UiInspection::writeReport(*menu, QString("program-file-menu-%1-%2")
                                                             .arg(language == "ru" ? "ru" : "en")
                                                             .arg(size.width())));
            }
            QMenu *child = root->menu();
            child->popup(menu->mapToGlobal(menu->actionGeometry(root).topRight()));
            QCoreApplication::processEvents();
            int previousBottom = -1;
            for (auto *action : child->actions()) {
                const QRect rect = child->actionGeometry(action);
                QVERIFY(child->rect().contains(rect));
                QVERIFY(rect.top() > previousBottom);
                previousBottom = rect.bottom();
                if (!action->isSeparator()) {
                    const int width = child->fontMetrics().horizontalAdvance(action->text());
                    QVERIFY2(width < rect.width(), qPrintable(action->text()));
                }
            }
            const QString tag = QString("%1-%2-%3")
                                    .arg(program ? "program-shell-menu" : "shell-menu")
                                    .arg(language == "ru" ? "ru" : "en")
                                    .arg(size.width());
            QVERIFY(UiInspection::writeReport(*child, tag));
            child->close();
            menu->close();
            inspected = true;
        };
        if (program) {
            auto *menu = window.findChild<QMenu *>("fileMenu");
            QVERIFY(menu);
            menu->popup(window.mapToGlobal(QPoint(0, window.menuBar()->height())));
            QCoreApplication::processEvents();
            inspect(menu);
        } else {
            QTimer::singleShot(
                0, [&] { inspect(qobject_cast<QMenu *>(QApplication::activePopupWidget())); });
            QVERIFY(QMetaObject::invokeMethod(
                view, "customContextMenuRequested", Qt::DirectConnection,
                Q_ARG(QPoint, view->visualItemRect(view->item(view->currentRow(), 0)).center())));
        }
        QVERIFY(inspected);
    }
    void nativeOpenKeepsExplicitTypeAndCancellation() {
        QFETCH(QString, outcome);
        QFETCH(bool, program);
        QTemporaryDir fixture;
        QVERIFY(fixture.isValid());
        CurrentDirectoryGuard guard;
        const QString fileName = QStringLiteral("данные 空 with space.txt");
        QVERIFY(write(fixture.filePath(fileName), "native context bytes"));
        const QString archiveName = QStringLiteral("архив 空 with space.7z");
        QProcess process;
        process.setWorkingDirectory(fixture.path());
        QStringList args{"a", "-t7z", "-y"};
        if (outcome == "cancel")
            args << "-pfixture-password" << "-mhe=on";
        args << "--" << archiveName << fileName;
        process.start(engine_, args);
        QVERIFY(process.waitForFinished(30000));
        QCOMPARE(process.exitCode(), 0);
        QFile archive(fixture.filePath(archiveName));
        QVERIFY(archive.open(QIODevice::ReadOnly));
        const QByteArray before = archive.readAll();
        archive.close();
        QVERIFY(QDir::setCurrent(fixture.path()));
        const QString settings = qEnvironmentVariable("SEVENZIP_SETTINGS_PATH");
        QFile::remove(settings);
        SaveRegLang(UString("-"), settings);
        CContextMenuInfo ci;
        ci.Flags =
            NContextMenuFlags::kOpen | NContextMenuFlags::kOpenAs | NContextMenuFlags::kCompress;
        ci.Flags_Def = true;
        ci.Cascaded.SetTrueTrue();
        ci.Save(settings);
        ArchiveBackend backend(nullptr, engine_);
        QCOMPARE(backend.loadCodecCatalog(module_, version_), S_OK);
        MainWindow window(&backend);
        window.show();
        auto *view = table(window);
        QTRY_VERIFY(rowNamed(window, archiveName) >= 0);
        view->selectRow(rowNamed(window, archiveName));
        QSignalSpy started(&backend, &ArchiveBackend::engineStarted);
        QStringList errors;
        bool passwordCanceled = false;
        QTimer dismissal;
        connect(&dismissal, &QTimer::timeout, [&] {
            if (auto *password =
                    dynamic_cast<CPasswordDialog *>(QApplication::activeModalWidget())) {
                passwordCanceled = true;
                password->reject();
                return;
            }
            if (auto *box = qobject_cast<QMessageBox *>(QApplication::activeModalWidget())) {
                errors.append(box->text());
                box->accept();
            }
        });
        dismissal.start(5);
        bool invoked = false;
        const auto invoke = [&](QMenu *menu) {
            QVERIFY(menu);
            const QString verb = outcome == "wrong-type" ? "SevenZip.Open.zip" : "SevenZip.Open.7z";
            auto *open = findVerb(*menu, verb);
            QVERIFY(open && open->isEnabled());
            auto *compress = findVerb(*menu, "SevenZipCompress");
            QVERIFY(compress && compress->isEnabled());
            menu->close();
            invoked = true;
            open->trigger();
        };
        if (program) {
            auto *menu = window.findChild<QMenu *>("fileMenu");
            QVERIFY(menu);
            QVERIFY(QMetaObject::invokeMethod(menu, "aboutToShow", Qt::DirectConnection));
            invoke(menu);
        } else {
            QTimer::singleShot(
                0, [&] { invoke(qobject_cast<QMenu *>(QApplication::activePopupWidget())); });
            QVERIFY(QMetaObject::invokeMethod(
                view, "customContextMenuRequested", Qt::DirectConnection,
                Q_ARG(QPoint, view->visualItemRect(view->item(view->currentRow(), 0)).center())));
        }
        dismissal.stop();
        QVERIFY(invoked);
        QCOMPARE(started.size(), 0); // The caller's backend remains independent.
        MainWindow *receiver = nullptr;
        for (auto *widget : QApplication::topLevelWidgets())
            if (widget->objectName() == "shellArchiveWindow")
                receiver = dynamic_cast<MainWindow *>(widget);
        QVERIFY(receiver && receiver != &window);
        QTRY_VERIFY(rowNamed(window, archiveName) >= 0);
        QCOMPARE(view->selectionModel()->selectedRows().size(), outcome == "success" ? 0 : 1);
        if (outcome == "success") {
            QCOMPARE(table(*receiver)->rowCount(), 1);
            QCOMPARE(table(*receiver)->item(0, 0)->text(), fileName);
            QVERIFY(errors.isEmpty());
            QVERIFY(!passwordCanceled);
        } else {
            QTRY_VERIFY(rowNamed(window, archiveName) >= 0);
            QCOMPARE(passwordCanceled, outcome == "cancel");
            QCOMPARE(errors.isEmpty(), outcome == "cancel");
        }
        QVERIFY(!backend.busy());
        QVERIFY(archive.open(QIODevice::ReadOnly));
        QCOMPARE(archive.readAll(), before);
        receiver->close();
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    }
};
QTEST_MAIN(ShellMenuWindowTests)
#include "shell_menu_window_tests.moc"
