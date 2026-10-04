#include "7zip/UI/FileManager/resource.h"
#include "CPP/7zip/UI/FileManager/App.h"
#include "CPP/7zip/UI/FileManager/FSFolder.h"
#include "CPP/7zip/UI/FileManager/Panel.h"
#include "Windows/Menu.h"
#include "qtnativeshellmenupresentation.h"
#include <QCryptographicHash>
#include <QFontMetrics>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMenu>
#include <QProcess>
#include <QScopeGuard>
#include <QScreen>
#include <QStyle>
#include <QTableWidget>
#include <QTemporaryDir>
#include <QtTest>

namespace {
HWND FindWindowFor(DWORD pid, const wchar_t *className, bool rightmost = false) {
    struct Search {
        DWORD pid;
        const wchar_t *className;
        bool rightmost;
        HWND found = nullptr;
        LONG left = LONG_MIN;
    } search{pid, className, rightmost};
    EnumWindows(
        [](HWND window, LPARAM parameter) -> BOOL {
            auto &search = *reinterpret_cast<Search *>(parameter);
            DWORD pid = 0;
            GetWindowThreadProcessId(window, &pid);
            wchar_t name[256] = {};
            GetClassNameW(window, name, 256);
            RECT rectangle{};
            if (pid == search.pid && IsWindowVisible(window) &&
                wcscmp(name, search.className) == 0 && GetWindowRect(window, &rectangle) &&
                (!search.rightmost || rectangle.left > search.left)) {
                search.found = window;
                search.left = rectangle.left;
            }
            return TRUE;
        },
        LPARAM(&search));
    return search.found;
}

QJsonArray NativeRows(HWND owner, HMENU menu) {
    QJsonArray rows;
    for (int position = 0; position < GetMenuItemCount(menu); ++position) {
        wchar_t label[2048] = {};
        MENUITEMINFOW item{};
        item.cbSize = sizeof(item);
        item.fMask = MIIM_STRING | MIIM_FTYPE | MIIM_STATE | MIIM_BITMAP | MIIM_SUBMENU;
        item.dwTypeData = label;
        item.cch = 2048;
        if (!GetMenuItemInfoW(menu, UINT(position), TRUE, &item))
            return {};
        RECT bounds{};
        GetMenuItemRect(owner, menu, UINT(position), &bounds);
        rows.append(QJsonObject{{"text", QString::fromWCharArray(label)},
                                {"enabled", !(item.fState & (MFS_DISABLED | MFS_GRAYED))},
                                {"checked", bool(item.fState & MFS_CHECKED)},
                                {"default", bool(item.fState & MFS_DEFAULT)},
                                {"separator", bool(item.fType & MFT_SEPARATOR)},
                                {"ownerDraw", bool(item.fType & MFT_OWNERDRAW)},
                                {"bitmap", item.hbmpItem != nullptr},
                                {"submenu", item.hSubMenu != nullptr},
                                {"bounds", QJsonArray{int(bounds.left), int(bounds.top),
                                                      int(bounds.right - bounds.left),
                                                      int(bounds.bottom - bounds.top)}}});
    }
    return rows;
}

bool Send(HWND owner, UINT message, WPARAM wParam, LPARAM lParam) {
    DWORD_PTR result = 0;
    return SendMessageTimeoutW(owner, message, wParam, lParam, SMTO_ABORTIFHUNG, 5000, &result);
}
UINT WindowDpi(HWND owner) {
    using GetDpi = UINT(WINAPI *)(HWND);
    const auto getDpi = reinterpret_cast<GetDpi>(
        GetProcAddress(GetModuleHandleW(L"user32.dll"), "GetDpiForWindow"));
    return getDpi ? getDpi(owner) : 0;
}
bool OwnReferenceKey(DWORD pid, const QString &path) {
    if (!path.startsWith(QStringLiteral("Software\\SevenzipDesktopReference-")))
        return false;
    HKEY key = nullptr;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, reinterpret_cast<LPCWSTR>(path.utf16()), 0,
                      KEY_QUERY_VALUE, &key) != ERROR_SUCCESS)
        return false;
    DWORD owner = 0, type = 0, bytes = sizeof(owner);
    const auto queried = RegQueryValueExW(key, L"SevenzipReferenceOwnerPID", nullptr, &type,
                                          reinterpret_cast<BYTE *>(&owner), &bytes);
    RegCloseKey(key);
    return queried == ERROR_SUCCESS && type == REG_DWORD && bytes == sizeof(owner) && owner == pid;
}
void RemoveReferenceKey(DWORD pid, const QString &path) {
    using DeleteTree = LSTATUS(WINAPI *)(HKEY, LPCWSTR);
    const auto deleteTree = reinterpret_cast<DeleteTree>(
        GetProcAddress(GetModuleHandleW(L"advapi32.dll"), "RegDeleteTreeW"));
    if (!deleteTree || !OwnReferenceKey(pid, path))
        return;
    const auto name = reinterpret_cast<LPCWSTR>(path.utf16());
    deleteTree(HKEY_CURRENT_USER, name);
    RegDeleteKeyW(HKEY_CURRENT_USER, name);
}
} // namespace

class NativeRenderedSystemMenuTests final : public QObject {
    Q_OBJECT
    void ValidateReference(const QString &referenceDirectory, QJsonObject &provenance) {
        QFile manifest(referenceDirectory + "/reference.json");
        QVERIFY(manifest.open(QIODevice::ReadOnly));
        provenance = QJsonDocument::fromJson(manifest.readAll()).object();
        QCOMPARE(provenance["producer"].toString(),
                 QStringLiteral("sevenzip-desktop/build-windows-reference"));
        QCOMPARE(provenance["revision"].toString(), QStringLiteral(SEVENZIP_UPSTREAM_REVISION));
        QFile executable(referenceDirectory + "/7zFM.exe");
        QVERIFY(executable.open(QIODevice::ReadOnly));
        QCOMPARE(
            QString::fromLatin1(
                QCryptographicHash::hash(executable.readAll(), QCryptographicHash::Sha256).toHex()),
            provenance["binarySha256"].toString());
    }
    void SaveReference(HWND owner, HWND popup, const QJsonArray &original,
                       const QJsonObject &provenance, const QString &reports) {
        QFile report(reports + "/native-system-rendered-reference.json");
        QVERIFY(report.open(QIODevice::WriteOnly));
        QJsonObject evidence{{"provenance", provenance},
                             {"captureKind", "runtime"},
                             {"actions", original},
                             {"dpi", int(WindowDpi(owner))}};
        NONCLIENTMETRICSW metrics{};
        metrics.cbSize = sizeof(metrics);
        QVERIFY(SystemParametersInfoW(SPI_GETNONCLIENTMETRICS, sizeof(metrics), &metrics, 0));
        evidence["menuFont"] =
            QJsonObject{{"family", QString::fromWCharArray(metrics.lfMenuFont.lfFaceName)},
                        {"height", int(metrics.lfMenuFont.lfHeight)},
                        {"weight", int(metrics.lfMenuFont.lfWeight)}};
        QVERIFY(report.write(QJsonDocument(evidence).toJson()) > 0);
        if (qEnvironmentVariable("SEVENZIP_CAPTURE_SCREENSHOTS") == "1")
            QVERIFY(QGuiApplication::primaryScreen()
                        ->grabWindow(WId(popup))
                        .save(reports + "/native-system-rendered-reference.png"));
    }
    void CompareBridge(const QString &directory, const QJsonArray &original,
                       const QString &reports) {
        CMyComPtr<IFolderFolder> folder = new NFsFolder::CFSFolder(directory);
        QCOMPARE(folder->LoadItems(), S_OK);
        CPanel panel(folder);
        QWidget ownerWindow;
        ownerWindow.resize(980, 620);
        QTableWidget list(&ownerWindow);
        panel.AttachPresentation(&list, nullptr);
        panel.LoadFullPath();
        CRecordVector<UInt32> selected;
        selected.Add(0);
        CMyComPtr<IContextMenu> provider;
        const HMENU native = CreatePopupMenu();
        QVERIFY(native);
        NWindows::CMenu menu;
        menu.Attach(native);
        NWindows::CMenuDestroyer release(menu);
        panel.CreateSystemMenu(menu, false, selected, provider);
        QVERIFY(provider);
        const auto queried = NativeRows(nullptr, GetSubMenu(native, 0));
        QtNativeShellMenuPresentation presentation(native, provider, kMenuCmdID_Plugin_Start + 400);
        menu.Detach();
        QMenu target;
        presentation.Render(target, [](unsigned) {});
        QCOMPARE(target.actions().size(), 1);
        auto *system = target.actions().first()->menu();
        QVERIFY(system);
        QJsonArray projected;
        for (const auto *action : system->actions())
            projected.append(QJsonObject{{"text", action->text()},
                                         {"enabled", action->isEnabled()},
                                         {"separator", action->isSeparator()},
                                         {"icon", !action->icon().isNull()}});
        QFile qtReport(reports + "/native-system-rendered-qt.json");
        QVERIFY(qtReport.open(QIODevice::WriteOnly));
        const QJsonObject font{{"family", system->font().family()},
                               {"pixelSize", system->font().pixelSize()},
                               {"pointSize", system->font().pointSizeF()},
                               {"height", QFontMetrics(system->font()).height()},
                               {"style", system->style()->metaObject()->className()},
                               {"ratio", system->devicePixelRatioF()}};
        QVERIFY(qtReport.write(
                    QJsonDocument(
                        QJsonObject{{"queried", queried}, {"projected", projected}, {"font", font}})
                        .toJson()) > 0);
        system->popup(QPoint(350, 80));
        QVERIFY(QTest::qWaitForWindowExposed(system));
        if (qEnvironmentVariable("SEVENZIP_CAPTURE_SCREENSHOTS") == "1")
            QVERIFY(system->grab().save(reports + "/native-system-rendered-qt.png"));
        system->hide();
        QCOMPARE(system->actions().size(), queried.size());
        QCOMPARE(queried.size(), original.size());
        for (int i = 0; i < original.size(); ++i) {
            const auto source = original[i].toObject();
            const auto *action = system->actions()[i];
            QCOMPARE(action->text(), source["text"].toString());
            if (!action->isSeparator())
                QCOMPARE(action->isEnabled(), source["enabled"].toBool());
            QCOMPARE(action->isChecked(), source["checked"].toBool());
            QCOMPARE(action->isSeparator(), source["separator"].toBool());
            QCOMPARE(action->menu() != nullptr, source["submenu"].toBool());
            QCOMPARE(system->defaultAction() == action, source["default"].toBool());
            if (source["bitmap"].toBool() && !source["ownerDraw"].toBool())
                QVERIFY(!action->icon().isNull());
        }
    }

  private slots:
    void pinnedNativeWindowAndQtBridge() {
        const QString referenceDirectory = QStringLiteral(SEVENZIP_NATIVE_REFERENCE_DIR);
        QJsonObject provenance;
        ValidateReference(referenceDirectory, provenance);
        QVERIFY(!QTest::currentTestFailed());
        const QString registryKey = provenance["registryKey"].toString();
        QVERIFY(registryKey.startsWith(QStringLiteral("Software\\SevenzipDesktopReference-")));
        QTemporaryDir fixture;
        QVERIFY(fixture.isValid());
        QFile file(fixture.filePath(QStringLiteral("native System файл 密碼.txt")));
        QVERIFY(file.open(QIODevice::WriteOnly));
        QCOMPARE(file.write("native context bytes"), qint64(20));
        file.close();
        QProcess reference;
        reference.setProgram(referenceDirectory + "/7zFM.exe");
        reference.setArguments({fixture.path()});
        reference.start();
        QVERIFY(reference.waitForStarted());
        const DWORD pid = DWORD(reference.processId());
        HWND owner = nullptr;
        const auto close = qScopeGuard([&] {
            if (owner)
                PostMessageW(owner, WM_CLOSE, 0, 0);
            if (!reference.waitForFinished(5000)) {
                reference.kill();
                reference.waitForFinished();
            }
            RemoveReferenceKey(pid, registryKey);
        });
        QTRY_VERIFY_WITH_TIMEOUT((owner = FindWindowFor(pid, L"7-Zip::FM")), 15000);
        QVERIFY(SetWindowPos(owner, nullptr, 40, 40, 980, 620, SWP_NOZORDER));
        QVERIFY(Send(owner, WM_COMMAND, IDM_SELECT_ALL, 0));
        QVERIFY(SetForegroundWindow(owner));
        QVERIFY(PostMessageW(owner, WM_SYSCOMMAND, SC_KEYMENU, L'f'));
        QTRY_VERIFY(FindWindowFor(pid, L"#32768"));
        // File opens with its first (System) group selected; Right enters it.
        QVERIFY(PostMessageW(owner, WM_KEYDOWN, VK_RIGHT, 0));
        QTest::qWait(300);
        const HMENU fileMenu = GetSubMenu(GetMenu(owner), 0);
        const HMENU systemMenu = GetSubMenu(fileMenu, 0);
        QVERIFY(systemMenu);
        const QJsonArray original = NativeRows(owner, systemMenu);
        QVERIFY(!original.isEmpty());
        const HWND popup = FindWindowFor(pid, L"#32768", true);
        QVERIFY(popup);
        const QString reports = qEnvironmentVariable("SEVENZIP_UI_REPORT_DIR");
        QVERIFY(!reports.isEmpty() && QDir().mkpath(reports));
        SaveReference(owner, popup, original, provenance, reports);
        QVERIFY(!QTest::currentTestFailed());
        QVERIFY(PostMessageW(owner, WM_CANCELMODE, 0, 0));
        QTRY_VERIFY(!FindWindowFor(pid, L"#32768"));

        CompareBridge(fixture.path(), original, reports);
        QVERIFY(!QTest::currentTestFailed());
        QVERIFY(PostMessageW(owner, WM_CLOSE, 0, 0));
        QVERIFY(reference.waitForFinished(5000));
        QCOMPARE(reference.exitCode(), 0);
        QVERIFY(!OwnReferenceKey(pid, registryKey));
    }
};

QTEST_MAIN(NativeRenderedSystemMenuTests)
#include "native_rendered_system_menu_tests.moc"
