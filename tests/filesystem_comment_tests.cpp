#include "7zip/PropID.h"
#include "7zip/UI/FileManager/resource.h"
#include "CPP/7zip/UI/FileManager/ComboDialog.h"
#include "CPP/7zip/UI/FileManager/FSFolder.h"
#include "CPP/7zip/UI/FileManager/LangUtils.h"
#include "Windows/PropVariant.h"
#include "layout_inspection.h"
#include "window_state_support.h"
#include <QScopeGuard>
#include <atomic>

namespace {
bool Write(const QString &path, const QByteArray &bytes) {
    QFile file(path);
    return file.open(QIODevice::WriteOnly) && file.write(bytes) == bytes.size();
}
QByteArray Read(const QString &path) {
    QFile file(path);
    return file.open(QIODevice::ReadOnly) ? file.readAll() : QByteArray();
}
QString Comment(IFolderFolder *folder, UInt32 index) {
    NWindows::NCOM::CPropVariant value;
    if (folder->GetProperty(index, kpidComment, &value) != S_OK || value.vt != VT_BSTR)
        return {};
    return QString::fromWCharArray(value.bstrVal);
}
UInt32 Find(IFolderFolder *folder, const QString &name) {
    UInt32 count = 0;
    folder->GetNumberOfItems(&count);
    for (UInt32 i = 0; i < count; ++i) {
        NWindows::NCOM::CPropVariant value;
        folder->GetProperty(i, kpidName, &value);
        if (value.vt == VT_BSTR && QString::fromWCharArray(value.bstrVal) == name)
            return i;
    }
    return count;
}
} // namespace

class FilesystemCommentTests final : public QObject, protected MainWindowTestSupport {
    Q_OBJECT
    static int FsRow(MainWindow &window, const QString &name) {
        for (int row = 0; row < table(window)->rowCount(); ++row)
            if (table(window)->item(row, 0)->data(Qt::UserRole + 3).toString() == name)
                return row;
        return -1;
    }
  private slots:
    void sourceCommentOwnsPublication() {
        QTemporaryDir fixture;
        QVERIFY(fixture.isValid());
        const QString name = QStringLiteral("comment 空.txt");
        QVERIFY(Write(fixture.filePath(name), "protected bytes"));
        ArchiveBackend backend;
        MainWindow window(&backend);
        window.show();
        window.openPath(fixture.path());
        QTRY_VERIFY(FsRow(window, name) >= 0);
        table(window)->selectRow(FsRow(window, name));
        auto &panel = *static_cast<FileTable *>(table(window))->sourcePanel;
        panel.Set_AutoRefresh_Mode(false);
        CMyComPtr<IFolderFolder> folder = panel.GetFolder();
        unsigned publications = 0;
        bool guarded = false;
        panel.AttachReloadPresentation([&](const CSelectedState &state) {
            ++publications;
            guarded = !panel.ShouldProcessNotifications() && state.FocusedName_Defined &&
                      ToQString(state.FocusedName) == name && panel.GetFolder() == folder;
            return folder->LoadItems();
        });
        bool accept = false;
        QTimer responder;
        connect(&responder, &QTimer::timeout, &window, [&] {
            auto *dialog = dynamic_cast<CComboDialog *>(QApplication::activeModalWidget());
            if (!dialog)
                return;
            if (accept) {
                dialog->findChild<QComboBox *>("comboValue")->setEditText("published 空");
                dialog->accept();
            } else
                dialog->reject();
        });
        responder.start(2);
        action(window, "commentAction")->trigger();
        QCOMPARE(publications, 0u);
        accept = true;
        action(window, "commentAction")->trigger();
        QCOMPARE(publications, 1u);
        QVERIFY(guarded);
        QCOMPARE(Comment(folder, Find(folder, name)), QStringLiteral("published 空"));
        QVERIFY(panel.ShouldProcessNotifications());
        QCOMPARE(Read(fixture.filePath(name)), QByteArray("protected bytes"));
    }
    void commentReloadBoundary_data() {
        QTest::addColumn<unsigned>("sourceIndex");
        QTest::addColumn<QString>("language");
        QTest::addColumn<QString>("outcome");
        for (unsigned index : {0u, 1u})
            for (const QString language : {"-", "ru"})
                for (const QString outcome : {"cancel", "accepted", "load-cancel", "load-failure"})
                    QTest::newRow(
                        qPrintable(QString("%1-%2-%3").arg(index).arg(language).arg(outcome)))
                        << index << language << outcome;
    }
    void commentReloadBoundary() {
        QFETCH(unsigned, sourceIndex);
        QFETCH(QString, language);
        QFETCH(QString, outcome);
        const QString settings = qEnvironmentVariable("SEVENZIP_SETTINGS_PATH");
        QFile::remove(settings);
        SaveRegLang(ToUString(language), settings);
        ReloadLang(settings);
        CFmSettings preferences;
        preferences.Load(settings);
        preferences.ShowDots = false;
        preferences.Save(settings);
        const auto resetLanguage = qScopeGuard([&] {
            SaveRegLang(L"-", settings);
            ReloadLang(settings);
        });
        QTemporaryDir fixture;
        QVERIFY(fixture.isValid());
        const QString name = QStringLiteral("comment 文件.bin");
        QVERIFY(Write(fixture.filePath(name), "original bytes"));
        QVERIFY(Write(fixture.filePath("peer.bin"), "peer bytes"));
        ArchiveBackend backend;
        MainWindow window(&backend);
        window.show();
        QVERIFY(QTest::qWaitForWindowActive(&window));
        action(window, "twoPanelsAction")->trigger();
        auto *ui = window.findChild<QWidget *>(QString("filePanel%1").arg(sourceIndex));
        auto *view = static_cast<FileTable *>(ui->findChild<QTableWidget *>("archiveTable"));
        auto *address = ui->findChild<QLineEdit *>("archivePath");
        address->setFocus();
        address->setText(fixture.path());
        QTest::keyClick(address, Qt::Key_Return);
        QTRY_COMPARE(view->rowCount(), 2);
        int row = -1;
        for (int i = 0; i < view->rowCount(); ++i)
            if (view->item(i, 0)->data(Qt::UserRole + 3).toString() == name)
                row = i;
        QVERIFY(row >= 0);
        view->selectRow(row);
        view->setFocus();
        auto &panel = *view->sourcePanel;
        panel.Set_AutoRefresh_Mode(false);
        CMyComPtr<IFolderFolder> folder = panel.GetFolder();
        auto *filesystem = dynamic_cast<NFsFolder::CFSFolder *>((IFolderFolder *)folder);
        QVERIFY(filesystem);
        auto *peerUi = window.findChild<QWidget *>(QString("filePanel%1").arg(1 - sourceIndex));
        auto *peerView =
            static_cast<FileTable *>(peerUi->findChild<QTableWidget *>("archiveTable"));
        peerView->sourcePanel->Set_AutoRefresh_Mode(false);
        CMyComPtr<IFolderFolder> peerFolder = peerView->sourcePanel->GetFolder();
        const int peerRows = peerView->rowCount();
        std::atomic<unsigned> loads{0};
        std::atomic<bool> unguardedReload{false};
        bool observed = false;
        QTimer responder;
        connect(&responder, &QTimer::timeout, &window, [&] {
            auto *dialog = dynamic_cast<CComboDialog *>(QApplication::activeModalWidget());
            if (!dialog || observed)
                return;
            observed = true;
            const auto close = qScopeGuard([&] {
                if (dialog->isVisible())
                    dialog->reject();
            });
            QVERIFY(Write(fixture.filePath("added δ.txt"), "external bytes"));
            filesystem->CheckBreak = [&](const QString &) {
                ++loads;
                if (panel.ShouldProcessNotifications())
                    unguardedReload = true;
                if (outcome == "load-failure")
                    throw std::bad_alloc();
                return outcome != "load-cancel";
            };
            dialog->findChild<QComboBox *>("comboValue")->setEditText("saved 空");
            if (outcome == "cancel")
                QTest::keyClick(dialog, Qt::Key_Escape);
            else
                dialog->accept();
        });
        responder.start(2);
        QTRY_VERIFY(action(window, "commentAction")->isEnabled());
        action(window, "commentAction")->trigger();
        responder.stop();
        QVERIFY(observed);
        const unsigned calls = loads.load();
        filesystem->CheckBreak = {};
        QVERIFY(!unguardedReload.load());
        if (outcome == "cancel") {
            QCOMPARE(calls, 0u);
            QCOMPARE(view->rowCount(), 2);
            QCOMPARE(view->currentRow(), row);
            QCOMPARE(view->selectionModel()->selectedRows().size(), 1);
            QVERIFY(!QFileInfo::exists(fixture.filePath("descript.ion")));
        } else {
            QVERIFY(calls > 0);
            QCOMPARE(view->rowCount(), outcome == "accepted" ? 4 : 0);
            QVERIFY(Read(fixture.filePath("descript.ion")).contains("saved 空"));
        }
        QVERIFY(panel.ShouldProcessNotifications());
        QVERIFY(peerView->sourcePanel->ShouldProcessNotifications());
        QVERIFY(view->updatesEnabled());
        QVERIFY(peerView->updatesEnabled());
        QVERIFY(view->isEnabled());
        QVERIFY(peerView->isEnabled());
        QCOMPARE(panel.GetFolder(), (IFolderFolder *)folder);
        QCOMPARE(peerView->sourcePanel->GetFolder(), (IFolderFolder *)peerFolder);
        QCOMPARE(peerView->rowCount(), peerRows);
        QVERIFY(action(window, "refreshAction")->isEnabled());
        for (const QSize size : {QSize(980, 620), QSize(640, 400)}) {
            window.resize(size);
            QApplication::processEvents();
            QVERIFY(UiInspection::fullyVisible(*view, window));
            QVERIFY(UiInspection::fullyVisible(*peerView, window));
            QVERIFY(UiInspection::fullyVisible(*address, window));
            QVERIFY(UiInspection::writeReport(
                window,
                QString("comment-reload-%1-%2").arg(QTest::currentDataTag()).arg(size.width())));
        }
        action(window, "refreshAction")->trigger();
        QTRY_COMPARE(view->rowCount(), outcome == "cancel" ? 3 : 4);
        QCOMPARE(Read(fixture.filePath(name)), QByteArray("original bytes"));
        QCOMPARE(Read(fixture.filePath("peer.bin")), QByteArray("peer bytes"));
        QCOMPARE(Read(fixture.filePath("added δ.txt")), QByteArray("external bytes"));
    }
    void sourceWriterPreservesOtherPairsAndClears() {
        QTemporaryDir fixture;
        QVERIFY(fixture.isValid());
        const QString name = QStringLiteral("имя 空 with spaces.bin");
        const QByteArray payload("binary\0bytes", 12);
        QVERIFY(Write(fixture.filePath(name), payload));
        const QString description = fixture.filePath("descript.ion");
        QVERIFY(Write(description, "\"other name\" retained\r\n"));
        const auto permissions = QFile::permissions(description);
        CMyComPtr<IFolderFolder> folder = new NFsFolder::CFSFolder(fixture.path());
        QCOMPARE(folder->LoadItems(), S_OK);
        const UInt32 index = Find(folder, name);
        QVERIFY(Comment(folder, index).isEmpty()); // Prime the original read cache.
        CMyComPtr<IFolderOperations> operations;
        QCOMPARE(folder.QueryInterface(IID_IFolderOperations, &operations), S_OK);
        NWindows::NCOM::CPropVariant value(L"  Комментарий 空  ");
        QCOMPARE(operations->SetProperty(index, kpidComment, &value, nullptr), S_OK);
        QCOMPARE(Comment(folder, index), QStringLiteral("Комментарий 空"));
        const QByteArray written = Read(description);
        QVERIFY(written.startsWith("\xEF\xBB\xBF\r\n"));
        QVERIFY(written.contains("\"other name\" retained\r\n"));
        QVERIFY(written.contains(('"' + name + "\" Комментарий 空\r\n").toUtf8()));
        QCOMPARE(QFile::permissions(description), permissions);
        QCOMPARE(Read(fixture.filePath(name)), payload);
        value = UInt32(3);
        QCOMPARE(operations->SetProperty(index, kpidComment, &value, nullptr), E_INVALIDARG);
        QCOMPARE(Read(description), written);
        QCOMPARE(operations->SetProperty(UInt32(-1), kpidComment, &value, nullptr), E_INVALIDARG);
        QCOMPARE(operations->SetProperty(index, kpidName, &value, nullptr), E_NOTIMPL);
        value = L"   ";
        QCOMPARE(operations->SetProperty(index, kpidComment, &value, nullptr), S_OK);
        QVERIFY(Comment(folder, index).isEmpty());
        QVERIFY(!Read(description).contains(name.toUtf8()));
        value = L"again";
        QCOMPARE(operations->SetProperty(index, kpidComment, &value, nullptr), S_OK);
        value.Clear();
        QCOMPARE(operations->SetProperty(index, kpidComment, &value, nullptr), S_OK);
        QVERIFY(Comment(folder, index).isEmpty());
        QCOMPARE(Read(fixture.filePath(name)), payload);
    }
    void sourceSaveFailureKeepsOriginalReturnPolicy() {
        QTemporaryDir fixture;
        QVERIFY(fixture.isValid());
        QVERIFY(Write(fixture.filePath("payload"), "unchanged"));
        QVERIFY(QDir(fixture.path()).mkdir("descript.ion"));
        CMyComPtr<IFolderFolder> folder = new NFsFolder::CFSFolder(fixture.path());
        QCOMPARE(folder->LoadItems(), S_OK);
        const UInt32 index = Find(folder, "payload");
        QVERIFY(Comment(folder, index).isEmpty());
        CMyComPtr<IFolderOperations> operations;
        QCOMPARE(folder.QueryInterface(IID_IFolderOperations, &operations), S_OK);
        NWindows::NCOM::CPropVariant value(L"not saved");
        // Original SetProperty ignores SaveComments's false return.
        QCOMPARE(operations->SetProperty(index, kpidComment, &value, nullptr), S_OK);
        QVERIFY(QFileInfo(fixture.filePath("descript.ion")).isDir());
        QCOMPARE(folder->LoadItems(), S_OK);
        QVERIFY(Comment(folder, Find(folder, "payload")).isEmpty());
        QCOMPARE(Read(fixture.filePath("payload")), QByteArray("unchanged"));
    }
    void flatNestedItemRetainsSourceRefusal() {
        QTemporaryDir fixture;
        QVERIFY(fixture.isValid());
        QVERIFY(QDir(fixture.path()).mkdir("sub"));
        QVERIFY(Write(fixture.filePath("sub/leaf.bin"), "unchanged"));
        CMyComPtr<IFolderFolder> folder = new NFsFolder::CFSFolder(fixture.path());
        CMyComPtr<IFolderSetFlatMode> flat;
        QCOMPARE(folder.QueryInterface(IID_IFolderSetFlatMode, &flat), S_OK);
        QCOMPARE(flat->SetFlatMode(1), S_OK);
        QCOMPARE(folder->LoadItems(), S_OK);
        CMyComPtr<IFolderOperations> operations;
        QCOMPARE(folder.QueryInterface(IID_IFolderOperations, &operations), S_OK);
        NWindows::NCOM::CPropVariant value(L"refused");
        QCOMPARE(operations->SetProperty(Find(folder, "leaf.bin"), kpidComment, &value, nullptr),
                 E_NOTIMPL);
        QVERIFY(!QFileInfo::exists(fixture.filePath("descript.ion")));
        QVERIFY(!QFileInfo::exists(fixture.filePath("sub/descript.ion")));
        QCOMPARE(Read(fixture.filePath("sub/leaf.bin")), QByteArray("unchanged"));
    }
    void actualFilesystemCommand_data() {
        QTest::addColumn<QString>("language");
        QTest::addColumn<bool>("cancel");
        for (const QString &language : {"-", "ru"})
            for (bool cancel : {false, true})
                QTest::newRow(qPrintable(language + QString::number(cancel))) << language << cancel;
    }
    void actualFilesystemCommand() {
        QFETCH(QString, language);
        QFETCH(bool, cancel);
        const QString settings = qEnvironmentVariable("SEVENZIP_SETTINGS_PATH");
        QFile::remove(settings);
        SaveRegLang(UString(language.toStdWString().c_str()), settings);
        ReloadLang(settings);
        CFmSettings preferences;
        preferences.Load(settings);
        preferences.ShowDots = true;
        preferences.Save(settings);
        const auto resetLanguage = qScopeGuard([&] {
            SaveRegLang(L"-", settings);
            ReloadLang(settings);
        });
        QTemporaryDir fixture;
        QVERIFY(fixture.isValid());
        const QString name = QStringLiteral("имя 空 with spaces.bin");
        QVERIFY(Write(fixture.filePath(name), "unchanged bytes"));
        QVERIFY(Write(fixture.filePath("other.bin"), "other bytes"));
        const QString description = fixture.filePath("descript.ion");
        const QByteArray initial = ('"' + name + "\" initial\r\nother.bin retained\r\n").toUtf8();
        QVERIFY(Write(description, initial));
        ArchiveBackend backend;
        MainWindow window(&backend);
        window.show();
        window.openPath(fixture.path());
        QTRY_VERIFY(FsRow(window, name) >= 0);
        const int row = FsRow(window, name);
        table(window)->setCurrentCell(row, 0);
        table(window)->selectAll();
        const int parent = FsRow(window, "..");
        QVERIFY(parent >= 0);
        table(window)->selectionModel()->select(table(window)->model()->index(parent, 0),
                                                QItemSelectionModel::Deselect |
                                                    QItemSelectionModel::Rows);
        QTRY_VERIFY(action(window, "commentAction")->isEnabled());
        bool observed = false;
        QTimer responder;
        connect(&responder, &QTimer::timeout, this, [&] {
            auto *dialog = dynamic_cast<CComboDialog *>(QApplication::activeModalWidget());
            if (!dialog || observed)
                return;
            observed = true;
            const auto close = qScopeGuard([&] {
                if (dialog->isVisible())
                    dialog->reject();
            });
            QVERIFY(dialog->windowTitle().contains(name));
            auto *edit = dialog->findChild<QComboBox *>("comboValue");
            QCOMPARE(edit->currentText(), QStringLiteral("initial"));
            QCOMPARE(dialog->findChild<QLabel *>("comboPrompt")->text(),
                     QString::fromWCharArray(LangString(IDS_COMMENT2)));
            for (int width : {360, 260}) {
                dialog->resize(width, 150);
                QApplication::processEvents();
                QVERIFY(UiInspection::fullyVisible(*edit, *dialog));
                auto *buttons = dialog->findChild<QDialogButtonBox *>("comboButtons");
                QVERIFY(UiInspection::fullyVisible(*buttons, *dialog));
                QVERIFY(UiInspection::writeReport(*dialog, QString("filesystem-comment-%1-%2-%3")
                                                               .arg(language == "ru" ? "ru" : "en")
                                                               .arg(cancel)
                                                               .arg(width)));
            }
            edit->setEditText(QStringLiteral("  changed 空  "));
            if (cancel)
                QTest::keyClick(dialog, Qt::Key_Escape);
            else
                dialog->findChild<QDialogButtonBox *>("comboButtons")
                    ->button(QDialogButtonBox::Ok)
                    ->click();
        });
        responder.start(2);
        if (cancel) {
            window.activateWindow();
            QVERIFY(QTest::qWaitForWindowActive(&window));
            table(window)->setFocus();
            QTRY_VERIFY(table(window)->hasFocus());
            action(window, "commentAction")->trigger();
            QTRY_VERIFY(observed);
        } else
            action(window, "commentAction")->trigger();
        responder.stop();
        QVERIFY(observed);
        QCOMPARE(table(window)->currentRow(), FsRow(window, name));
        QCOMPARE(table(window)->selectionModel()->selectedRows().size(), 3);
        QCOMPARE(Read(fixture.filePath(name)), QByteArray("unchanged bytes"));
        QCOMPARE(Read(fixture.filePath("other.bin")), QByteArray("other bytes"));
        if (cancel)
            QCOMPARE(Read(description), initial);
        else {
            CMyComPtr<IFolderFolder> reopened = new NFsFolder::CFSFolder(fixture.path());
            QCOMPARE(reopened->LoadItems(), S_OK);
            QCOMPARE(Comment(reopened, Find(reopened, name)), QStringLiteral("changed 空"));
            QCOMPARE(Comment(reopened, Find(reopened, "other.bin")), QStringLiteral("retained"));
        }
        table(window)->setCurrentCell(parent, 0);
        table(window)->selectRow(parent);
        QTRY_VERIFY(!action(window, "commentAction")->isEnabled());
    }
};
QTEST_MAIN(FilesystemCommentTests)
#include "filesystem_comment_tests.moc"
