#include "CPP/7zip/UI/FileManager/SysIconUtils.h"
#include "external_editor_fixture.h"
#include "layout_inspection.h"
#include "panel_load_folder.h"
#include "qtfolderprojection.h"
#include "window_state_support.h"
#include <QApplication>
#include <QStyle>
#include <QtTest>

class PanelItemIconsTests final : public QObject, protected MainWindowTestSupport {
    Q_OBJECT
  private slots:
    void sourcePolicy_data() {
        QTest::addColumn<QString>("type");
        QTest::addColumn<bool>("real");
        QTest::addColumn<bool>("query");
        for (const QString &type : {QString("FSFolder"), QString("AltStreamsFolder"),
                                    QString("RootFolder"), QString("7-Zip.zip")})
            for (bool real : {false, true})
                QTest::newRow(qPrintable(type + QString::number(real)))
                    << type << real << (real || (type != "FSFolder" && type != "AltStreamsFolder"));
    }
    void sourcePolicy() {
        QFETCH(QString, type);
        QFETCH(bool, real);
        QFETCH(bool, query);
        auto *spec = new LoadFolder;
        CMyComPtr<IFolderFolder> folder = spec;
        spec->folderType = type.toStdWString().c_str();
        spec->directory = false;
        spec->attributes = FILE_ATTRIBUTE_DIRECTORY;
        CPanel panel(folder);
        panel._showRealFileIcons = real;
        QCOMPARE(panel.LoadListItems(), S_OK);
        QCOMPARE(spec->iconCalls, 0u);
        QCOMPARE(spec->attributeCalls, 0u);
        QIcon icon;
        QCOMPARE(panel.GetItemIcon(0, icon), S_OK);
        QCOMPARE(spec->iconCalls, query ? 1u : 0u);
        QCOMPARE(spec->attributeCalls, 1u);
        QCOMPARE(icon.cacheKey(),
                 g_Ext_to_Icon_Map.GetIcon(0, QString::fromUtf8("first δ")).cacheKey());
        QCOMPARE(panel.GetItemIcon(0, icon), S_OK);
        QCOMPARE(spec->attributeCalls, 1u);
        QCOMPARE(panel.GetItemIcon(2, icon), E_INVALIDARG);
        QVERIFY(icon.isNull());
    }
    void definedSystemIconSkipsAttributes() {
        auto *spec = new LoadFolder;
        CMyComPtr<IFolderFolder> folder = spec;
        spec->attributeResult = E_ABORT;
        spec->iconResult = S_OK;
#ifdef _WIN32
        QCOMPARE(Shell_GetFileInfo_SysIconIndex_for_Path_return_HRESULT(
                     L"__DIR__", FILE_ATTRIBUTE_DIRECTORY, &spec->iconIndex),
                 S_OK);
#else
        spec->iconIndex = g_Ext_to_Icon_Map.GetPlatformIconIndex(
            "icon-policy-test", QApplication::style()->standardIcon(QStyle::SP_DirIcon));
#endif
        QVERIFY(spec->iconIndex > 0);
        CPanel panel(folder);
        QCOMPARE(panel.LoadListItems(), S_OK);
        QIcon icon;
        QCOMPARE(panel.GetItemIcon(0, icon), S_OK);
        QVERIFY(!icon.isNull());
        QCOMPARE(spec->iconCalls, 1u);
        QCOMPARE(spec->attributeCalls, 0u);
    }
    void archiveAttributesUseSourceMask_data() {
        QTest::addColumn<bool>("directory");
        for (bool directory : {false, true})
            QTest::newRow(directory ? "directory" : "file") << directory;
    }
    void archiveAttributesUseSourceMask() {
        QFETCH(bool, directory);
        auto *spec = new LoadFolder;
        CMyComPtr<IFolderFolder> folder = spec;
        spec->folderType = L"7-Zip.zip";
        spec->directory = directory;
        spec->attributes = 0xF1230080 | (directory ? 0 : FILE_ATTRIBUTE_DIRECTORY);
        CPanel panel(folder);
        QCOMPARE(panel.LoadListItems(), S_OK);
        QIcon icon;
        QCOMPARE(panel.GetItemIcon(1, icon), S_OK);
        const UInt32 attributes =
            directory ? ((spec->attributes & 0x3FFF) | FILE_ATTRIBUTE_DIRECTORY)
                      : ((spec->attributes & 0x3FFF) & ~UInt32(FILE_ATTRIBUTE_DIRECTORY));
        QCOMPARE(
            icon.cacheKey(),
            g_Ext_to_Icon_Map.GetIcon(attributes, QStringLiteral("second 文件.txt")).cacheKey());
    }
    void fallbackAndFailure_data() {
        QTest::addColumn<int>("index");
        QTest::addColumn<qint32>("result");
        QTest::newRow("zero") << 0 << qint32(S_OK);
        QTest::newRow("negative") << -1 << qint32(S_OK);
        QTest::newRow("failed-positive") << 7 << qint32(E_FAIL);
    }
    void fallbackAndFailure() {
        QFETCH(int, index);
        QFETCH(qint32, result);
        auto *spec = new LoadFolder;
        CMyComPtr<IFolderFolder> folder = spec;
        spec->iconIndex = index;
        spec->iconResult = result;
        spec->attributeResult = E_ABORT;
        CPanel panel(folder);
        QCOMPARE(panel.LoadListItems(), S_OK);
        QIcon icon;
        QCOMPARE(panel.GetItemIcon(0, icon), E_ABORT);
        QVERIFY(icon.isNull());
        QCOMPARE(panel.GetItemIcon(1, icon), E_ABORT);
        QCOMPARE(spec->attributeCalls, 1u);
        CSelectedState selection;
        QCOMPARE(panel.RefreshListCtrl(selection), E_ABORT);
        spec->attributeResult = S_OK;
        spec->attributes = FILE_ATTRIBUTE_NORMAL;
        QCOMPARE(panel.LoadListItems(), S_OK);
        QCOMPARE(panel.GetItemIcon(1, icon), S_OK);
        QCOMPARE(icon.cacheKey(), g_Ext_to_Icon_Map
                                      .GetIcon(spec->attributes | FILE_ATTRIBUTE_DIRECTORY,
                                               QStringLiteral("second 文件.txt"))
                                      .cacheKey());
    }
    void filesystemRowsUseSourceIcons_data() {
        QTest::addColumn<bool>("real");
        QTest::addColumn<int>("count");
        QTest::addColumn<bool>("archive");
        for (bool real : {false, true})
            for (int count : {1, 301})
                for (bool archive : {false, true})
                    QTest::newRow(qPrintable(QString("%1-%2-%3").arg(real).arg(count).arg(archive)))
                        << real << count << archive;
    }
    void filesystemRowsUseSourceIcons() {
        QFETCH(bool, real);
        QFETCH(int, count);
        QFETCH(bool, archive);
        QFile::remove(qEnvironmentVariable("SEVENZIP_SETTINGS_PATH"));
        QTemporaryDir fixture;
        QVERIFY(fixture.isValid());
        for (int i = 0; i < count; ++i) {
            QFile file(fixture.filePath(QStringLiteral("file δ %1.txt").arg(i)));
            QVERIFY(file.open(QIODevice::WriteOnly));
            QCOMPARE(file.write("control"), qint64(7));
        }
        QString destination = fixture.path();
        if (archive) {
            destination = fixture.filePath(QStringLiteral("archive 文件.zip"));
            QProcess process;
            process.setWorkingDirectory(fixture.path());
            process.start(ExternalEditorFixture::executable(),
                          {"a", "-tzip", destination, "*.txt"});
            QVERIFY(process.waitForFinished(30000));
            QCOMPARE(process.exitStatus(), QProcess::NormalExit);
            QCOMPARE(process.exitCode(), 0);
        }
        ArchiveBackend backend;
        MainWindow window(&backend);
        window.show();
        QVERIFY(QTest::qWaitForWindowActive(&window));
        auto *view = static_cast<FileTable *>(table(window));
        view->sourcePanel->_showRealFileIcons = real;
        auto *address = window.findChild<QLineEdit *>("archivePath");
        address->setText(destination);
        QTest::keyClick(address, Qt::Key_Return);
        QTRY_COMPARE(view->rowCount(), count);
        QTRY_VERIFY(view->sourcePanel->ShouldProcessNotifications());
        QCOMPARE(view->sourcePanel->IsArcFolder(), archive);
        for (int row = 0; row < view->rowCount(); ++row) {
            QIcon source;
            QCOMPARE(view->sourcePanel->GetItemIcon(
                         view->item(row, 0)->data(Qt::UserRole + 6).toUInt(), source),
                     S_OK);
            QVERIFY(!source.isNull());
            QCOMPARE(view->item(row, 0)->icon().cacheKey(), source.cacheKey());
        }
        for (const QSize size : {QSize(980, 620), QSize(640, 400)}) {
            window.resize(size);
            QApplication::processEvents();
            QVERIFY(UiInspection::fullyVisible(*view, window));
            QVERIFY(UiInspection::writeReport(
                window,
                QString("source-icons-%1-%2").arg(QTest::currentDataTag()).arg(size.width())));
        }
        QFile file(fixture.filePath(QStringLiteral("file δ 0.txt")));
        QVERIFY(file.open(QIODevice::ReadOnly));
        QCOMPARE(file.readAll(), QByteArray("control"));
    }
    void projectionSuccessFailureAndCancel_data() {
        QTest::addColumn<int>("stop");
        QTest::newRow("success") << -1;
        QTest::newRow("failure-first") << 0;
        QTest::newRow("failure-later-batch") << 40;
        QTest::newRow("cancel") << -2;
        QTest::newRow("replace") << -3;
    }
    void namespaceIconFailureRecovery_data() {
        QTest::addColumn<qint32>("result");
        QTest::newRow("failure") << qint32(E_FAIL);
        QTest::newRow("cancel") << qint32(E_ABORT);
    }
    void namespaceIconFailureRecovery() {
        QFETCH(qint32, result);
        QFile::remove(qEnvironmentVariable("SEVENZIP_SETTINGS_PATH"));
        ControlledBackend backend;
        MainWindow window(&backend);
        action(window, "openRootAction")->trigger();
        auto *view = static_cast<FileTable *>(table(window));
        auto *spec = new LoadFolder;
        CMyComPtr<IFolderFolder> folder = spec;
        auto &panel = *view->sourcePanel;
        panel.SetNewFolder(folder);
        QCOMPARE(panel.RefreshListCtrl_SaveFocused(), S_OK);
        QCOMPARE(view->rowCount(), 2);
        spec->attributeResult = result;
        QCOMPARE(panel.RefreshListCtrl_SaveFocused(), HRESULT(result));
        QCOMPARE(view->rowCount(), 0);
        QVERIFY(panel.ShouldProcessNotifications());
        CSelectedState state;
        QCOMPARE(panel.RefreshListCtrl(state), HRESULT(result));
        spec->attributeResult = S_OK;
        QCOMPARE(panel.RefreshListCtrl_SaveFocused(), S_OK);
        QCOMPARE(view->rowCount(), 2);
        QCOMPARE(panel.GetFolder(), (IFolderFolder *)folder);
    }
    void projectionSuccessFailureAndCancel() {
        QFETCH(int, stop);
        CMyComPtr<IFolderFolder> folder = new LoadFolder;
        QtFolderProjection projection;
        unsigned rows = 0;
        unsigned completions = 0;
        HRESULT terminal = E_FAIL;
        projection.start(
            folder, 70,
            [&](quint32 index) {
                if (index != rows) {
                    QTest::qFail("Rows must retain source order", __FILE__, __LINE__);
                    return E_FAIL;
                }
                ++rows;
                if (stop == -2) {
                    projection.cancel();
                    return E_ABORT;
                }
                if (stop == -3) {
                    projection.start(
                        folder, 1, [](quint32) { return S_OK; },
                        [&](HRESULT value) {
                            ++completions;
                            terminal = value;
                        });
                    return E_ABORT;
                }
                return int(index) == stop ? E_ABORT : S_OK;
            },
            [&](HRESULT value) {
                ++completions;
                terminal = value;
            });
        QTRY_VERIFY(!projection.active());
        QCOMPARE(rows, stop >= 0 ? unsigned(stop + 1) : (stop == -1 ? 70u : 1u));
        QCOMPARE(completions, stop == -2 ? 0u : 1u);
        if (completions)
            QCOMPARE(terminal, stop >= 0 ? E_ABORT : S_OK);
        QCoreApplication::processEvents();
        QCOMPARE(completions, stop == -2 ? 0u : 1u);
    }
};

QTEST_MAIN(PanelItemIconsTests)
#include "panel_item_icons_tests.moc"
