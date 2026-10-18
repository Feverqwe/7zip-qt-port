#include "7zip/UI/FileManager/resource.h"
#include "CPP/7zip/UI/FileManager/ComboDialog.h"
#include "CPP/7zip/UI/FileManager/LangUtils.h"
#include "CPP/7zip/UI/FileManager/RegistryUtils.h"
#include "layout_inspection.h"
#include "window_state_support.h"
#include <QScopeGuard>

class SourceSelectionTests final : public QObject, protected MainWindowTestSupport {
    Q_OBJECT
    static FileTable *openPanel(MainWindow &window, unsigned pane, const QString &path) {
        auto *ui = window.findChild<QWidget *>(QString("filePanel%1").arg(pane));
        auto *address = ui->findChild<QLineEdit *>("archivePath");
        address->setFocus();
        address->setText(path);
        QTest::keyClick(address, Qt::Key_Return);
        return static_cast<FileTable *>(ui->findChild<QTableWidget *>("archiveTable"));
    }
    static QStringList selected(CPanel &panel) {
        CRecordVector<UInt32> indices;
        panel.Get_ItemIndices_Selected(indices);
        QStringList names;
        FOR_VECTOR(i, indices)
        names.append(ToQString(panel.GetItemName(indices[i])));
        names.sort();
        return names;
    }
    static int row(FileTable &view, const QString &name) {
        for (int i = 0; i < view.rowCount(); ++i)
            if (view.item(i, 0)->data(Qt::UserRole + 3).toString() == name)
                return i;
        return -1;
    }

  private slots:
    void selection_data() {
        QTest::addColumn<unsigned>("pane");
        QTest::addColumn<QString>("language");
        QTest::addColumn<bool>("alternative");
        for (unsigned pane : {0u, 1u})
            for (const QString language : {"-", "ru"})
                for (bool alternative : {false, true})
                    QTest::newRow(
                        qPrintable(QString("%1-%2-%3").arg(pane).arg(language).arg(alternative)))
                        << pane << language << alternative;
    }
    void selection() {
        QFETCH(unsigned, pane);
        QFETCH(QString, language);
        QFETCH(bool, alternative);
        const QString settings = qEnvironmentVariable("SEVENZIP_SETTINGS_PATH");
        QFile::remove(settings);
        SaveRegLang(ToUString(language), settings);
        ReloadLang(settings);
        const auto reset = qScopeGuard([&] {
            SaveRegLang(L"-", settings);
            ReloadLang(settings);
        });
        CFmSettings preferences;
        preferences.Load(settings);
        preferences.ShowDots = false;
        preferences.AlternativeSelection = alternative;
        preferences.Save(settings);
        QTemporaryDir fixture, peerDirectory;
        QVERIFY(fixture.isValid());
        QVERIFY(peerDirectory.isValid());
        const QString first = QStringLiteral("first 空.txt");
        const QString second = QStringLiteral("second 空.txt");
        for (const QString &name : {first, second, QString("keep.bin")}) {
            QFile file(fixture.filePath(name));
            QVERIFY(file.open(QIODevice::WriteOnly));
            QCOMPARE(file.write("protected input"), qint64(15));
        }
        QVERIFY(QDir(fixture.path()).mkdir("folder.txt"));
        ControlledBackend backend;
        MainWindow window(&backend);
        window.show();
        window.activateWindow();
        QVERIFY(QTest::qWaitForWindowActive(&window));
        action(window, "twoPanelsAction")->trigger();
        auto *peer = openPanel(window, 1 - pane, peerDirectory.path());
        QTRY_COMPARE(peer->rowCount(), 0);
        CMyComPtr<IFolderFolder> peerFolder = peer->sourcePanel->GetFolder();
        auto *view = openPanel(window, pane, fixture.path());
        QTRY_COMPARE(view->rowCount(), 4);
        auto &panel = *view->sourcePanel;
        panel.Set_AutoRefresh_Mode(false);
        CMyComPtr<IFolderFolder> folder = panel.GetFolder();
        view->setCurrentCell(row(*view, "keep.bin"), 0);
        view->selectRow(row(*view, "keep.bin"));
        panel.SelectSpec(L"keep.bin", true);
        QCOMPARE(selected(panel), QStringList{"keep.bin"});
        const int focus = view->currentRow();
        bool observed = false;
        const auto answer = [&](bool selectMode, bool accept, const QString &mask, bool reports) {
            observed = false;
            QTimer::singleShot(0, &window, [&, selectMode, accept, mask, reports] {
                auto *dialog = window.findChild<CComboDialog *>("comboDialog");
                if (!dialog)
                    return;
                const auto close = qScopeGuard([&] {
                    if (dialog->isVisible())
                        dialog->reject();
                });
                observed = true;
                QCOMPARE(dialog->windowTitle(),
                         ToQString(LangString(selectMode ? IDS_SELECT : IDS_DESELECT)));
                QCOMPARE(dialog->findChild<QLabel *>("comboPrompt")->text(),
                         ToQString(LangString(IDS_SELECT_MASK)));
                auto *combo = dialog->findChild<QComboBox *>("comboValue");
                QCOMPARE(combo->currentText(), QString("*"));
                if (reports)
                    for (int width : {360, 260}) {
                        dialog->resize(width, 150);
                        dialog->layout()->activate();
                        QVERIFY(UiInspection::fullyVisible(*combo, *dialog));
                        QVERIFY(
                            UiInspection::labelFits(*dialog->findChild<QLabel *>("comboPrompt")));
                        QVERIFY(UiInspection::writeReport(*dialog,
                                                          QString("source-selection-%1-%2-%3-%4-%5")
                                                              .arg(pane)
                                                              .arg(language)
                                                              .arg(alternative)
                                                              .arg(selectMode)
                                                              .arg(width)));
                    }
                combo->setEditText(mask);
                if (accept)
                    dialog->accept();
                else
                    dialog->reject();
            });
        };
        answer(true, false, "*", false);
        action(window, "selectMaskAction")->trigger();
        QVERIFY(observed);
        QCOMPARE(selected(panel), QStringList{"keep.bin"});
        QCOMPARE(view->currentRow(), focus);
        answer(true, true, QStringLiteral("*空.txt"), true);
        action(window, "selectMaskAction")->trigger();
        QVERIFY(observed);
        QCOMPARE(selected(panel), (QStringList{first, "keep.bin", second}));
        answer(false, true, "keep.*", true);
        action(window, "deselectMaskAction")->trigger();
        QVERIFY(observed);
        QCOMPARE(selected(panel), (QStringList{first, second}));
        action(window, "deselectAllAction")->trigger();
        QVERIFY(selected(panel).isEmpty());
        view->setCurrentCell(row(*view, first), 0);
        action(window, "selectTypeAction")->trigger();
        QCOMPARE(selected(panel), (QStringList{first, second}));
        action(window, "deselectTypeAction")->trigger();
        QVERIFY(selected(panel).isEmpty());
        action(window, "selectAllAction")->trigger();
        QCOMPARE(selected(panel).size(), 4);
        action(window, "invertSelectionAction")->trigger();
        QVERIFY(selected(panel).isEmpty());
        peer->setFocus();
        answer(true, true, first, false);
        panel.SelectSpec(true);
        QVERIFY(observed);
        QCOMPARE(selected(panel), QStringList{first});
        QCOMPARE(peer->sourcePanel->GetFolder(), (IFolderFolder *)peerFolder);
        QCOMPARE(panel.GetFolder(), (IFolderFolder *)folder);
        QVERIFY(selected(*peer->sourcePanel).isEmpty());
        QCOMPARE(backend.listCalls, 0);
        QCOMPARE(backend.extractCalls, 0);
        for (const QString &name : {first, second, QString("keep.bin")}) {
            QFile file(fixture.filePath(name));
            QVERIFY(file.open(QIODevice::ReadOnly));
            QCOMPARE(file.readAll(), QByteArray("protected input"));
        }
    }
};
#include "review_test_main.h"
SEVENZIP_UI_TEST_MAIN(SourceSelectionTests)
#include "source_selection_tests.moc"
