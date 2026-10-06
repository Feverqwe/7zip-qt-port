#include "CPP/7zip/UI/FileManager/FSFolder.h"
#include "CPP/7zip/UI/FileManager/LangUtils.h"
#include "CPP/7zip/UI/FileManager/Panel.h"
#include "filetable.h"
#include "layout_inspection.h"
#include "window_state_support.h"
#include <QFile>
#include <QMessageBox>
#include <QScopeGuard>
#include <QTemporaryDir>
#include <QTimer>
#include <QtTest>

class PanelRenameStateTests final : public QObject, protected MainWindowTestSupport {
    Q_OBJECT
    static bool write(const QString &path, const QByteArray &bytes) {
        QFile file(path);
        return file.open(QIODevice::WriteOnly) && file.write(bytes) == bytes.size();
    }
  private slots:
    void editorPreservesSavedSelection_data() {
        QTest::addColumn<unsigned>("sourceIndex");
        QTest::addColumn<QString>("language");
        QTest::addColumn<bool>("collision");
        QTest::addColumn<bool>("invalid");
        for (unsigned index : {0u, 1u})
            for (const QString language : {"-", "ru"})
                for (int outcome : {0, 1, 2})
                    QTest::newRow(
                        qPrintable(QString("%1-%2-%3").arg(index).arg(language).arg(outcome)))
                        << index << language << (outcome == 1) << (outcome == 2);
    }
    void editorPreservesSavedSelection() {
        QFETCH(unsigned, sourceIndex);
        QFETCH(QString, language);
        QFETCH(bool, collision);
        QFETCH(bool, invalid);
        const QString settings = qEnvironmentVariable("SEVENZIP_SETTINGS_PATH");
        QFile::remove(settings);
        SaveRegLang(ToUString(language), settings);
        ReloadLang(settings);
        CFmSettings preferences;
        preferences.Load(settings);
        preferences.ShowDots = false;
        preferences.AlternativeSelection = false;
        preferences.Save(settings);
        const auto reset = qScopeGuard([&] {
            SaveRegLang(L"-", settings);
            ReloadLang(settings);
        });
        QTemporaryDir fixture;
        QVERIFY(fixture.isValid());
        const QString original = QStringLiteral("original 文件.txt");
        const QString peer = QStringLiteral("peer δ.txt");
        const QString renamed = invalid     ? QStringLiteral("..")
                                : collision ? peer
                                            : QStringLiteral("renamed 空.txt");
        QVERIFY(write(fixture.filePath(original), "original bytes"));
        QVERIFY(write(fixture.filePath(peer), "peer bytes"));
        ControlledBackend backend;
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
        for (const auto &table : window.findChildren<QTableWidget *>("archiveTable"))
            static_cast<FileTable *>(table)->sourcePanel->Set_AutoRefresh_Mode(false);
        const auto findRow = [&](const QString &name) {
            for (int row = 0; row < view->rowCount(); ++row)
                if (view->item(row, 0)->data(Qt::UserRole + 3).toString() == name)
                    return row;
            return -1;
        };
        const int sourceRow = findRow(original);
        const int peerRow = findRow(peer);
        QVERIFY(sourceRow >= 0 && peerRow >= 0);
        auto *peerUi = window.findChild<QWidget *>(QString("filePanel%1").arg(1 - sourceIndex));
        auto *peerView =
            static_cast<FileTable *>(peerUi->findChild<QTableWidget *>("archiveTable"));
        CMyComPtr<IFolderFolder> peerFolder = peerView->sourcePanel->GetFolder();
        const UString peerPrefix = peerView->sourcePanel->GetCurrentFolderPrefix();
        const int peerRows = peerView->rowCount();
        view->selectRow(sourceRow);
        view->setFocus();
        QVERIFY(action(window, "renameAction")->isEnabled());
        action(window, "renameAction")->trigger();
        auto *editor = view->findChild<QLineEdit *>();
        QVERIFY(editor);
        // A selection change during the editor must survive its source snapshot.
        view->selectionModel()->select(view->model()->index(peerRow, 0),
                                       QItemSelectionModel::Select | QItemSelectionModel::Rows);
        QCOMPARE(view->selectionModel()->selectedRows().size(), 2);
        bool errorShown = false;
        QTimer closeError;
        connect(&closeError, &QTimer::timeout, &window, [&] {
            for (QWidget *widget : QApplication::topLevelWidgets())
                if (auto *box = qobject_cast<QMessageBox *>(widget); box && box->isVisible()) {
                    errorShown = true;
                    box->accept();
                }
        });
        closeError.start(2);
        CMyComPtr<IFolderFolder> retained = view->sourcePanel->GetFolder();
        auto *filesystem = dynamic_cast<NFsFolder::CFSFolder *>((IFolderFolder *)retained);
        QVERIFY(filesystem);
        unsigned loads = 0;
        const auto resetCheckBreak = qScopeGuard([&] { filesystem->CheckBreak = {}; });
        filesystem->CheckBreak = [&](const QString &) {
            ++loads;
            return true;
        };
        editor->setText(renamed);
        QTest::keyClick(editor, Qt::Key_Return);
        if (collision || invalid)
            QTRY_VERIFY(errorShown);
        QTRY_COMPARE(view->item(view->currentRow(), 0)->data(Qt::UserRole + 3).toString(),
                     collision || invalid ? original : renamed);
        QCOMPARE(view->selectionModel()->selectedRows().size(), 2);
        closeError.stop();
        filesystem->CheckBreak = {};
        QCOMPARE(loads == 0, invalid);
        QVERIFY(view->sourcePanel->ShouldDisplayItemText());
        QVERIFY(view->sourcePanel->ShouldProcessNotifications());
        QCOMPARE(view->sourcePanel->GetFolder(), (IFolderFolder *)retained);
        QCOMPARE(peerView->sourcePanel->GetFolder(), (IFolderFolder *)peerFolder);
        QCOMPARE(peerView->sourcePanel->GetCurrentFolderPrefix(), peerPrefix);
        QCOMPARE(peerView->rowCount(), peerRows);
        CRecordVector<UInt32> selected;
        view->sourcePanel->Get_ItemIndices_Selected(selected);
        QCOMPARE(selected.Size(), 2u);
        for (const QSize size : {QSize(980, 620), QSize(640, 400)}) {
            window.resize(size);
            QApplication::processEvents();
            QVERIFY(UiInspection::fullyVisible(*view, window));
            QVERIFY(UiInspection::writeReport(
                window,
                QString("rename-selection-%1-%2").arg(QTest::currentDataTag()).arg(size.width())));
        }
        for (const auto &entry :
             {qMakePair(collision || invalid ? original : renamed, QByteArray("original bytes")),
              qMakePair(peer, QByteArray("peer bytes"))}) {
            QFile payload(fixture.filePath(entry.first));
            QVERIFY(payload.open(QIODevice::ReadOnly));
            QCOMPARE(payload.readAll(), entry.second);
        }
    }
    void sourceStateAndDeferredText_data() {
        QTest::addColumn<bool>("alternative");
        QTest::addColumn<bool>("collision");
        QTest::addColumn<bool>("cancelLoad");
        for (bool alternative : {false, true})
            for (bool collision : {false, true})
                for (bool cancelLoad : {false, true})
                    QTest::newRow(qPrintable(
                        QString("%1-%2-%3").arg(alternative).arg(collision).arg(cancelLoad)))
                        << alternative << collision << cancelLoad;
    }
    void sourceStateAndDeferredText() {
        QFETCH(bool, alternative);
        QFETCH(bool, collision);
        QFETCH(bool, cancelLoad);
        QTemporaryDir fixture;
        QVERIFY(fixture.isValid());
        const QString original = QStringLiteral("original 文件.txt");
        const QString peer = QStringLiteral("peer δ.txt");
        const QString renamed = collision ? peer : QStringLiteral("renamed 空.txt");
        QVERIFY(write(fixture.filePath(original), "original bytes"));
        QVERIFY(write(fixture.filePath(peer), "peer bytes"));
        auto *filesystem = new NFsFolder::CFSFolder(fixture.path());
        CMyComPtr<IFolderFolder> folder = filesystem;
        CPanel panel(folder);
        FileTable view(0, 2);
        view.sourcePanel = &panel;
        view.textFolder = folder;
        view.propertyIds = {kpidName, kpidSize};
        view.setSelectionBehavior(QAbstractItemView::SelectRows);
        panel.AttachSelectionView(&view);
        panel.SetAlternativeSelection(alternative);
        QCOMPARE(panel.LoadListItems(), S_OK);
        const auto publish = [&] {
            UInt32 count = 0;
            folder->GetNumberOfItems(&count);
            view.setRowCount(int(count));
            for (UInt32 i = 0; i < count; ++i)
                for (int col = 0; col < 2; ++col) {
                    auto *item = new ArchiveTableItem(ToQString(panel.GetItemName(i)));
                    item->setData(Qt::UserRole + 6, i);
                    view.setItem(int(i), col, item);
                }
        };
        publish();
        QCOMPARE(panel.RefreshListCtrl(), S_OK);
        int row = -1;
        for (int i = 0; i < view.rowCount(); ++i)
            if (panel.GetItemName(UInt32(i)) == ToUString(original))
                row = i;
        QVERIFY(row >= 0);
        view.setCurrentCell(row, 0);
        panel.SelectAll(true);
        const QString cachedName = view.item(row, 0)->text();
        QCOMPARE(cachedName, original);
        QTimer closeError;
        connect(&closeError, &QTimer::timeout, &view, [] {
            for (QWidget *widget : QApplication::topLevelWidgets())
                if (auto *box = qobject_cast<QMessageBox *>(widget); box && box->isVisible())
                    box->accept();
        });
        closeError.start(2);
        const HRESULT result = panel.OnEndLabelEdit(UInt32(row), ToUString(renamed));
        closeError.stop();
        QCOMPARE(result == S_OK, !collision);
        const bool blank = view.item(row, 0)->text().isEmpty();
        const auto state = panel.TakeRefreshSelection();
        QVERIFY2(blank && state.has_value(),
                 qPrintable(QString("blank=%1 pending=%2").arg(blank).arg(state.has_value())));
        QCOMPARE(view.item(row, 0)->data(Qt::EditRole).toString(), original);
        QCOMPARE(view.item(row, 0)->data(Qt::UserRole + 6).toUInt(), UInt32(row));
        QVERIFY(state->FocusedName_Defined);
        QVERIFY(state->SelectFocused);
        QCOMPARE(state->FocusedName, ToUString(collision ? original : renamed));
        QCOMPARE(state->SelectedNames.Size(), 2u);
        QVERIFY(state->SelectedNames.FindInSorted(ToUString(original)) >= 0);
        QVERIFY(state->SelectedNames.FindInSorted(ToUString(peer)) >= 0);
        QVERIFY(!panel.TakeRefreshSelection().has_value());
        if (cancelLoad) {
            filesystem->CheckBreak = [](const QString &) { return false; };
            QCOMPARE(panel.LoadListItems(), E_ABORT);
            QCOMPARE(panel.RefreshListCtrl(*state), E_ABORT);
            QCOMPARE(view.rowCount(), 0);
            filesystem->CheckBreak = {};
        }
        QCOMPARE(panel.LoadListItems(), S_OK);
        publish();
        QCOMPARE(panel.RefreshListCtrl(*state), S_OK);
        const QString focused = collision ? original : renamed;
        QCOMPARE(view.item(view.currentRow(), 0)->text(), focused);
        QCOMPARE(view.selectionModel()->selectedRows().size(), alternative ? 1 : 2);
        CRecordVector<UInt32> selected;
        panel.Get_ItemIndices_Selected(selected);
        QCOMPARE(selected.Size(), alternative && !collision ? 1u : 2u);
        QVERIFY(panel.ShouldProcessNotifications());
        QCOMPARE(QFileInfo::exists(fixture.filePath(original)), collision);
        for (const auto &entry : {qMakePair(focused, QByteArray("original bytes")),
                                  qMakePair(peer, QByteArray("peer bytes"))}) {
            QFile payload(fixture.filePath(entry.first));
            QVERIFY(payload.open(QIODevice::ReadOnly));
            QCOMPARE(payload.readAll(), entry.second);
        }
    }
};
QTEST_MAIN(PanelRenameStateTests)
#include "panel_rename_state_tests.moc"
