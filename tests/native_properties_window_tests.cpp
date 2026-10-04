#include "window_state_support.h"
#include <QScopeGuard>
#include <atomic>
#include <chrono>
#include <cwchar>
#include <functional>
#include <future>
#include <thread>
#include <windows.h>

namespace {
struct ObservedPropertySheet {
    QString name;
    HWND owner = nullptr;
    HWND window = nullptr;
    bool tabs = false;
    bool visible = false;
    QString diagnostic;
};

BOOL CALLBACK FindPropertySheet(HWND window, LPARAM parameter) {
    auto &sheet = *reinterpret_cast<ObservedPropertySheet *>(parameter);
    wchar_t title[1024] = {};
    GetWindowTextW(window, title, 1024);
    if (!QString::fromWCharArray(title).contains(sheet.name))
        return TRUE;
    DWORD process = 0;
    GetWindowThreadProcessId(window, &process);
    wchar_t className[64] = {};
    GetClassNameW(window, className, 64);
    sheet.diagnostic = QString("class=%1 pid=%2 visible=%3 owner=%4 expectedOwner=%5 title=%6")
                           .arg(QString::fromWCharArray(className))
                           .arg(process)
                           .arg(IsWindowVisible(window))
                           .arg(quintptr(GetWindow(window, GW_OWNER)))
                           .arg(quintptr(sheet.owner))
                           .arg(QString::fromWCharArray(title));
    // The shell can host a sheet in another process with a proxy owner. The
    // unique disposable filename identifies this fixture across that boundary.
    if (std::wcscmp(className, L"#32770") != 0)
        return TRUE;
    // A native Windows property sheet has tabs. The former Qt collector has
    // neither this native class nor the selected file in its caption.
    HWND tabs = FindWindowExW(window, nullptr, L"SysTabControl32", nullptr);
    if (!tabs)
        return TRUE;
    sheet.window = window;
    sheet.tabs = true;
    sheet.visible = IsWindowVisible(window);
    return FALSE;
}

ObservedPropertySheet ObserveAndClose(const QString &name, HWND owner,
                                      const std::atomic<bool> &stop) {
    ObservedPropertySheet sheet{name, owner};
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(12);
    while (!stop && std::chrono::steady_clock::now() < deadline) {
        EnumWindows(FindPropertySheet, reinterpret_cast<LPARAM>(&sheet));
        if (sheet.window && sheet.visible) {
            PostMessageW(sheet.window, WM_CLOSE, 0, 0);
            return sheet;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }
    // A failing visibility regression still owns this uniquely named sheet.
    // Close it so failed fixtures do not leak hidden windows in the guest.
    if (sheet.window)
        PostMessageW(sheet.window, WM_CLOSE, 0, 0);
    return sheet;
}
} // namespace

class NativePropertiesWindowTests final : public QObject, protected MainWindowTestSupport {
    Q_OBJECT
    static int filesystemRow(MainWindow &window, const QString &path) {
        auto *view = table(window);
        for (int row = 0; row < view->rowCount(); ++row)
            if (QDir::cleanPath(QDir::fromNativeSeparators(view->item(row, 1)->text())) == path)
                return row;
        return -1;
    }
  private slots:
    void fileAndFolderUseNativeSheet_data() {
        QTest::addColumn<bool>("flat");
        QTest::addColumn<int>("mode");
        QTest::addColumn<bool>("directory");
        for (bool flat : {false, true})
            for (int mode = 0; mode < 4; ++mode)
                for (bool directory : {false, true})
                    QTest::newRow(qPrintable(
                        QString("flat%1-mode%2-dir%3").arg(flat).arg(mode).arg(directory)))
                        << flat << mode << directory;
    }

    void fileAndFolderUseNativeSheet() {
        QFETCH(bool, flat);
        QFETCH(int, mode);
        QFETCH(bool, directory);
        QTemporaryDir fixture;
        QVERIFY(fixture.isValid());
        // Unique disposable title identifies only this fixture's native sheet.
        const QString name =
            QFileInfo(fixture.path()).fileName() + QStringLiteral(" файл 密碼 space");
        const QString path = fixture.filePath(name);
        if (directory)
            QVERIFY(QDir().mkdir(path));
        else {
            QFile file(path);
            QVERIFY(file.open(QIODevice::WriteOnly));
            QCOMPARE(file.write("fixture"), qint64(7));
        }
        ControlledBackend backend;
        MainWindow window(&backend);
        window.show();
        auto *address = window.findChild<QLineEdit *>("archivePath");
        address->setText(fixture.path());
        QTest::keyClick(address, Qt::Key_Return);
        action(window, "flatViewAction")->setChecked(flat);
        action(window, qPrintable(QString("viewMode%1Action").arg(mode)))->trigger();
        auto *view = table(window);
        QTRY_VERIFY_WITH_TIMEOUT(filesystemRow(window, path) >= 0, 5000);
        const int row = filesystemRow(window, path);
        QVERIFY(row >= 0);
        view->clearSelection();
        view->selectionModel()->select(view->model()->index(row, 0),
                                       QItemSelectionModel::Select | QItemSelectionModel::Rows);
        // The user invokes Properties from an exposed File Manager window.
        // A synchronous fixture can reach this action before Qt has exposed it.
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        QVERIFY(IsWindowVisible(reinterpret_cast<HWND>(window.winId())));
        std::atomic<bool> stop{false};
        const auto finish = qScopeGuard([&] { stop = true; });
        auto observer = std::async(std::launch::async, ObserveAndClose, name,
                                   reinterpret_cast<HWND>(window.winId()), std::cref(stop));
        QTimer portableResponder;
        portableResponder.setInterval(10);
        bool portableShown = false;
        connect(&portableResponder, &QTimer::timeout, &window, [&] {
            if (auto *dialog = window.findChild<QDialog *>("selectedPropertiesDialog")) {
                portableShown = true;
                stop = true;
                dialog->reject();
            }
        });
        portableResponder.start();
        action(window, directory ? "selectedPropertiesAction" : "archiveInfoAction")->trigger();
        QTRY_VERIFY_WITH_TIMEOUT(
            observer.wait_for(std::chrono::milliseconds(0)) == std::future_status::ready, 15000);
        const ObservedPropertySheet sheet = observer.get();
        QVERIFY(!portableShown);
        QVERIFY2(sheet.window && sheet.tabs && sheet.visible, qPrintable(sheet.diagnostic));
        QTRY_VERIFY(!IsWindow(sheet.window));
        QCOMPARE(view->selectionModel()->selectedRows().size(), 1);
        QCOMPARE(backend.extractCalls, 0);
        QVERIFY(QFileInfo::exists(path));
        if (!directory) {
            QFile file(path);
            QVERIFY(file.open(QIODevice::ReadOnly));
            QCOMPARE(file.readAll(), QByteArray("fixture"));
        }
    }

    void missingSelectionDoesNotOpenPortableFallback() {
        QTemporaryDir fixture;
        QVERIFY(fixture.isValid());
        const QString path = fixture.filePath(QStringLiteral("missing файл 密碼.txt"));
        QFile file(path);
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.close();
        ControlledBackend backend;
        MainWindow window(&backend);
        window.show();
        auto *address = window.findChild<QLineEdit *>("archivePath");
        address->setText(fixture.path());
        QTest::keyClick(address, Qt::Key_Return);
        action(window, "autoRefreshAction")->setChecked(false);
        auto *view = table(window);
        const int row = filesystemRow(window, path);
        QVERIFY(row >= 0);
        view->selectRow(row);
        QVERIFY(QFile::remove(path));
        bool portableShown = false;
        QTimer responder;
        responder.setInterval(10);
        connect(&responder, &QTimer::timeout, &window, [&] {
            if (auto *dialog = window.findChild<QDialog *>("selectedPropertiesDialog")) {
                portableShown = true;
                dialog->reject();
            }
        });
        responder.start();
        action(window, "selectedPropertiesAction")->trigger();
        QTest::qWait(20);
        QVERIFY(!portableShown);
        QCOMPARE(view->selectionModel()->selectedRows().size(), 1);
        QCOMPARE(backend.extractCalls, 0);
        QVERIFY(!QFileInfo::exists(path));
    }
};

QTEST_MAIN(NativePropertiesWindowTests)
#include "native_properties_window_tests.moc"
