#include "CPP/7zip/UI/FileManager/LangUtils.h"
#include "layout_inspection.h"
#include "window_state_support.h"

class PanelColumnMenuTests final : public QObject, protected MainWindowTestSupport {
    Q_OBJECT
    static void toggle(QHeaderView &header, const QString &label) {
        QTimer::singleShot(0, [&] {
            auto *menu = qobject_cast<QMenu *>(QApplication::activePopupWidget());
            QVERIFY(menu);
            bool found = false;
            for (auto *option : menu->actions())
                if (option->text() == label) {
                    QVERIFY(option->isEnabled());
                    option->trigger();
                    found = true;
                    break;
                }
            QVERIFY(found);
            menu->close();
        });
        QVERIFY(QMetaObject::invokeMethod(&header, "customContextMenuRequested",
                                          Qt::DirectConnection, Q_ARG(QPoint, QPoint(10, 5))));
    }
    static QList<int> visibleOrder(FileTable &view) {
        QList<int> result;
        for (int visual = 0; visual < view.columnCount(); ++visual) {
            const int logical = view.horizontalHeader()->logicalIndex(visual);
            if (!view.isColumnHidden(logical))
                result.append(logical);
        }
        return result;
    }
    static void inspectAndCancel(FileTable &view) {
        const auto before = visibleOrder(view);
        const auto &columns = view.sourcePanel->GetColumns();
        QTimer::singleShot(0, [&] {
            auto *menu = qobject_cast<QMenu *>(QApplication::activePopupWidget());
            QVERIFY(menu);
            int actionIndex = 0;
            for (unsigned index = 0; index < columns.Size(); ++index) {
                const auto &prop = columns[index];
                const int logical = view.columnForProperty(prop.ID);
                if (logical < 0)
                    continue;
                QVERIFY(actionIndex < menu->actions().size());
                const auto *option = menu->actions()[actionIndex++];
                QCOMPARE(option->text(), view.horizontalHeaderItem(logical)->text());
                QVERIFY(option->isCheckable());
                QCOMPARE(option->isChecked(), prop.IsVisible);
                QCOMPARE(option->isEnabled(), index != 0);
            }
            QCOMPARE(actionIndex, menu->actions().size());
            menu->close();
        });
        QVERIFY(QMetaObject::invokeMethod(view.horizontalHeader(), "customContextMenuRequested",
                                          Qt::DirectConnection, Q_ARG(QPoint, QPoint(10, 5))));
        QCOMPARE(visibleOrder(view), before);
    }
  private slots:
    void menuRetainsSourceAppendAndOwner_data() {
        QTest::addColumn<unsigned>("pane");
        QTest::addColumn<bool>("russian");
        QTest::addColumn<bool>("inactive");
        for (unsigned pane : {0u, 1u})
            for (bool russian : {false, true})
                for (bool inactive : {false, true})
                    QTest::newRow(
                        qPrintable(QString("%1-%2-%3").arg(pane).arg(russian).arg(inactive)))
                        << pane << russian << inactive;
    }
    void menuRetainsSourceAppendAndOwner() {
        QFETCH(unsigned, pane);
        QFETCH(bool, russian);
        QFETCH(bool, inactive);
        const QString settings = qEnvironmentVariable("SEVENZIP_SETTINGS_PATH");
        QFile::remove(settings);
        SaveRegLang(UString(russian ? L"ru" : L"-"), settings);
        ReloadLang(settings);
        QTemporaryDir fixture;
        QVERIFY(fixture.isValid());
        QFile bytes(fixture.filePath(QStringLiteral("control 文件.txt")));
        QVERIFY(bytes.open(QIODevice::WriteOnly));
        QCOMPARE(bytes.write("untouched"), qint64(9));
        bytes.close();
        QFile smaller(fixture.filePath("z.txt"));
        QVERIFY(smaller.open(QIODevice::WriteOnly));
        QCOMPARE(smaller.write("x"), qint64(1));
        smaller.close();
        ControlledBackend backend;
        MainWindow window(&backend);
        window.show();
        QVERIFY(QTest::qWaitForWindowActive(&window));
        action(window, "twoPanelsAction")->trigger();
        FileTable *views[2];
        QLineEdit *addresses[2];
        for (unsigned index : {0u, 1u}) {
            auto *ui = window.findChild<QWidget *>(QString("filePanel%1").arg(index));
            views[index] = static_cast<FileTable *>(ui->findChild<QTableWidget *>("archiveTable"));
            addresses[index] = ui->findChild<QLineEdit *>("archivePath");
            addresses[index]->setFocus();
            addresses[index]->setText(fixture.path());
            QTest::keyClick(addresses[index], Qt::Key_Return);
            QTRY_COMPARE(views[index]->rowCount(), 2);
            views[index]->sourcePanel->Set_AutoRefresh_Mode(false);
            views[index]->selectRow(0);
        }
        addresses[inactive ? 1 - pane : pane]->setFocus();
        auto &view = *views[pane];
        auto &peer = *views[1 - pane];
        auto &panel = *view.sourcePanel;
        CMyComPtr<IFolderFolder> provider = panel.GetFolder();
        CMyComPtr<IFolderFolder> peerProvider = peer.sourcePanel->GetFolder();
        const auto peerOrder = visibleOrder(peer);
        QWidget *const focused = QApplication::focusWidget();
        inspectAndCancel(view);
        QCOMPARE(window.focusWidget(), focused);
        window.activateWindow();
        QTRY_COMPARE(QApplication::focusWidget(), focused);
        const int column = view.columnForProperty(kpidATime);
        QVERIFY(column >= 0);
        QVERIFY(view.isColumnHidden(column));
        const QString label = view.horizontalHeaderItem(column)->text();
        toggle(*view.horizontalHeader(), label);
        QVERIFY(!view.isColumnHidden(column));
        const CPropColumns shownCatalog = panel.GetColumns();
        const int catalogWidth = view.horizontalHeader()->sectionSize(column);
        view.horizontalHeader()->moveSection(view.horizontalHeader()->visualIndex(column), 1);
        view.horizontalHeader()->resizeSection(column, 173);
        QVERIFY(panel.GetColumns().IsEqualTo(shownCatalog));
        toggle(*view.horizontalHeader(), label);
        QVERIFY(view.isColumnHidden(column));
        toggle(*view.horizontalHeader(), label);
        QVERIFY(!view.isColumnHidden(column));
        QCOMPARE(visibleOrder(view).last(), column);
        QCOMPARE(view.horizontalHeader()->sectionSize(column), catalogWidth);
        QCOMPARE(panel.GetFolder(), (IFolderFolder *)provider);
        QCOMPARE(peer.sourcePanel->GetFolder(), (IFolderFolder *)peerProvider);
        QCOMPARE(visibleOrder(peer), peerOrder);
        QCOMPARE(view.selectionModel()->selectedRows().size(), 1);
        QCOMPARE(peer.selectionModel()->selectedRows().size(), 1);
        QCOMPARE(window.focusWidget(), focused);
        inspectAndCancel(view);
        view.sortItems(2, Qt::AscendingOrder);
        QCOMPARE(view.item(0, 0)->data(Qt::UserRole + 3).toString(), QString("z.txt"));
        const auto firstIndex = view.item(0, 0)->data(Qt::UserRole + 6);
        toggle(*view.horizontalHeader(), view.horizontalHeaderItem(2)->text());
        QCOMPARE(view.item(0, 0)->data(Qt::UserRole + 6), firstIndex);
        panel.OnReload();
        QCOMPARE(view.item(0, 0)->data(Qt::UserRole + 3).toString(),
                 QStringLiteral("control 文件.txt"));
        QCOMPARE(visibleOrder(view).last(), column);
        QCOMPARE(view.horizontalHeader()->sectionSize(column), catalogWidth);
        for (const QSize size : {QSize(980, 620), QSize(640, 400)}) {
            window.resize(size);
            QApplication::processEvents();
            QVERIFY(UiInspection::fullyVisible(view, window));
            QVERIFY(UiInspection::fullyVisible(peer, window));
            QVERIFY(UiInspection::writeReport(
                window,
                QString("column-menu-%1-%2").arg(QTest::currentDataTag()).arg(size.width())));
        }
        QVERIFY(bytes.open(QIODevice::ReadOnly));
        QCOMPARE(bytes.readAll(), QByteArray("untouched"));
        QCOMPARE(backend.listCalls, 0);
    }
};
QTEST_MAIN(PanelColumnMenuTests)
#include "panel_column_menu_tests.moc"
