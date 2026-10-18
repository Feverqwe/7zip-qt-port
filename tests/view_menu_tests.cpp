#include "7zip/PropID.h"
#include "CPP/7zip/UI/FileManager/LangUtils.h"
#include "Windows/TimeUtils.h"
#include "layout_inspection.h"
#include "window_state_support.h"
#include <QScopeGuard>

class ViewMenuTests final : public QObject, protected MainWindowTestSupport {
    Q_OBJECT
    static int row(FileTable &view, const QString &name) {
        for (int i = 0; i < view.rowCount(); ++i)
            if (view.item(i, 0)->text().normalized(QString::NormalizationForm_C) == name)
                return i;
        return -1;
    }
    static FileTable *openPanel(MainWindow &window, unsigned pane, const QString &path) {
        auto *panel = window.findChild<QWidget *>(QString("filePanel%1").arg(pane));
        auto *address = panel->findChild<QLineEdit *>("archivePath");
        address->setFocus();
        address->setText(path);
        QTest::keyClick(address, Qt::Key_Return);
        return static_cast<FileTable *>(panel->findChild<QTableWidget *>("archiveTable"));
    }
    static void openView(MainWindow &window) {
        auto *view = window.findChild<QMenu *>("viewMenu");
        view->popup(window.mapToGlobal(QPoint(20, 40)));
        QApplication::processEvents();
        QVERIFY(view->isVisible());
    }

  private slots:
    void activationAndCommands_data() {
        QTest::addColumn<unsigned>("pane");
        QTest::addColumn<QString>("language");
        for (unsigned pane : {0u, 1u})
            for (const QString &language : {QString("-"), QString("ru")})
                QTest::newRow(qPrintable(QString("pane%1-%2").arg(pane).arg(language)))
                    << pane << language;
    }
    void activationAndCommands() {
        QFETCH(unsigned, pane);
        QFETCH(QString, language);
        QFile::remove(qEnvironmentVariable("SEVENZIP_SETTINGS_PATH"));
        SaveRegLang(ToUString(language), qEnvironmentVariable("SEVENZIP_SETTINGS_PATH"));
        const bool originalUTC = g_Timestamp_Show_UTC;
        const auto restore = qScopeGuard([&] { g_Timestamp_Show_UTC = originalUTC; });
        QTemporaryDir fixture;
        QVERIFY(fixture.isValid());
        const QString name = QStringLiteral("файл δ with space.txt");
        QFile file(fixture.filePath(name));
        QVERIFY(file.open(QIODevice::WriteOnly));
        QCOMPARE(file.write("protected menu fixture"), qint64(22));
        file.close();
        ControlledBackend backend;
        MainWindow window(&backend);
        window.show();
        if (!action(window, "twoPanelsAction")->isChecked())
            action(window, "twoPanelsAction")->trigger();
        auto *peer = openPanel(window, 1 - pane, fixture.path());
        auto *source = openPanel(window, pane, fixture.path());
        QTRY_VERIFY(row(*source, name) >= 0 && row(*peer, name) >= 0);
        source->setCurrentCell(row(*source, name), 0);
        source->selectRow(row(*source, name));
        source->setFocus();
        auto *provider = source->sourcePanel->GetFolder();
        auto *peerProvider = peer->sourcePanel->GetFolder();
        auto *view = window.findChild<QMenu *>("viewMenu");
        auto *time = window.findChild<QMenu *>("timestampMenu");
        auto *utc = action(window, "timestampUtcAction");
        openView(window);
        FILETIME now;
        NWindows::NTime::GetCurUtcFileTime(now);
        char expected[64];
        QVERIFY(ConvertUtcFileTimeToString(now, expected, kTimestampPrintLevel_DAY));
        QCOMPARE(time->title(), QString::fromLatin1(expected));
        const int levels[] = {kTimestampPrintLevel_DAY, kTimestampPrintLevel_MIN,
                              kTimestampPrintLevel_SEC, kTimestampPrintLevel_NTFS,
                              kTimestampPrintLevel_NS};
        for (unsigned i = 0; i < 5; ++i) {
            auto *item = action(window, qPrintable(QString("timestampLevel%1Action").arg(i)));
            QVERIFY(!item->text().isEmpty());
            QVERIFY(item->isVisible() && item->isEnabled());
            QCOMPARE(item->isChecked(), source->sourcePanel->_timestampLevel == levels[i]);
        }
        view->hide();
        action(window, "timestampLevel4Action")->trigger();
        QCOMPARE(source->sourcePanel->_timestampLevel, kTimestampPrintLevel_NS);
        QCOMPARE(peer->sourcePanel->_timestampLevel, kTimestampPrintLevel_NS);
        utc->trigger();
        const bool utcValue = g_Timestamp_Show_UTC;
        action(window, "viewMode1Action")->trigger();
        action(window, "sort3Action")->trigger();
        source->sourcePanel->SortItemsWithPropID(kpidCTime);
        openView(window);
        QVERIFY(action(window, "sort0Action")->isChecked()); // Source fallback for other columns.
        QVERIFY(!action(window, "sort3Action")->isChecked());
        action(window, "selectAllAction")->trigger();
        QCOMPARE(source->sourcePanel->GetSortID(), PROPID(kpidCTime));
        QVERIFY(action(window, "sort0Action")->isChecked());
        QVERIFY(!action(window, "sort3Action")->isChecked());
        QVERIFY(action(window, "timestampLevel4Action")->isChecked());
        QVERIFY(action(window, "viewMode1Action")->isChecked());
        QVERIFY(action(window, "twoPanelsAction")->isChecked());
        QCOMPARE(action(window, "flatViewAction")->isChecked(), source->sourcePanel->GetFlatMode());
        QCOMPARE(utc->isChecked(), utcValue);
        for (const QSize size : {QSize(980, 620), QSize(640, 400)}) {
            view->hide();
            window.resize(size);
            openView(window);
            const QString tag = QString("view-menu-%1-%2x%3")
                                    .arg(QTest::currentDataTag())
                                    .arg(size.width())
                                    .arg(size.height());
            for (unsigned index : {0u, 1u}) {
                auto *panel = window.findChild<QWidget *>(QString("filePanel%1").arg(index));
                auto *list = panel->findChild<QWidget *>("archiveTable");
                if (!list->isVisible())
                    list = panel->findChild<QWidget *>("iconList");
                QVERIFY(UiInspection::fullyVisible(*list, window));
                QVERIFY(UiInspection::fullyVisible(*panel->findChild<QWidget *>("archivePath"),
                                                   window));
            }
            QVERIFY(UiInspection::writeReport(window, tag + "-main"));
            QVERIFY(UiInspection::writeReport(*view, tag));
            view->hide();
            time->popup(window.mapToGlobal(QPoint(30, 50)));
            QApplication::processEvents();
            QVERIFY(UiInspection::writeReport(*time, tag + "-time"));
            time->hide();
        }
        QCOMPARE(source->sourcePanel->GetFolder(), provider);
        QCOMPARE(peer->sourcePanel->GetFolder(), peerProvider);
        QVERIFY(source->sourcePanel->ShouldProcessNotifications());
        QVERIFY(peer->sourcePanel->ShouldProcessNotifications());
        QVERIFY(file.open(QIODevice::ReadOnly));
        QCOMPARE(file.readAll(), QByteArray("protected menu fixture"));
        MainWindow reopened(&backend);
        QVERIFY(QMetaObject::invokeMethod(reopened.findChild<QMenu *>("viewMenu"), "aboutToShow",
                                          Qt::DirectConnection));
        QVERIFY(action(reopened, "timestampLevel4Action")->isChecked());
        QCOMPARE(action(reopened, "timestampUtcAction")->isChecked(), utcValue);
    }
};

QTEST_MAIN(ViewMenuTests)
#include "view_menu_tests.moc"
