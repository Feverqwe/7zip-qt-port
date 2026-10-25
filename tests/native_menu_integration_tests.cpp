// This Windows-only desktop fixture uses Vista's parent-process attribute API.
#define Z7_WIN32_WINNT_MIN 0x0600
#include "Common/MyTypes.h"

#include "7zip/UI/Explorer/ContextMenuFlags.h"
#include "CPP/7zip/UI/Explorer/RegistryContextMenu.h"
#include "CPP/7zip/UI/FileManager/MenuPage.h"
#include "CPP/7zip/UI/FileManager/RegistryUtils.h"
#include "Common/MyCom.h"
#include "Common/StringConvert.h"
#include "Windows/COM.h"
#include "Windows/Shell.h"
#include "qtmenupageservices.h"
#include "qtnativemenupageservices.h"
#include <QCheckBox>
#include <QDir>
#include <QFile>
#include <QListWidget>
#include <QProcess>
#include <QProcessEnvironment>
#include <QScopeGuard>
#include <QTemporaryDir>
#include <QtTest>
#include <shlobj.h>

namespace {
constexpr wchar_t kOwner[] = L"SevenzipMenuTestPID";
constexpr wchar_t kApproved[] =
    L"Software\\Microsoft\\Windows\\CurrentVersion\\Shell Extensions\\Approved";
class OwnedRegistration {
    QStringList _keys;
    DWORD _owner = GetCurrentProcessId();
    using DeleteTree = LSTATUS(WINAPI *)(HKEY, LPCWSTR);
    const DeleteTree _deleteTree = reinterpret_cast<DeleteTree>(
        GetProcAddress(GetModuleHandleW(L"advapi32.dll"), "RegDeleteTreeW"));

    static QStringList Paths() {
        QStringList paths{QStringLiteral("Software\\Classes\\CLSID\\") +
                          QString::fromWCharArray(kSevenzipShellClsid)};
        for (const auto &prefix : {"*", "Folder", "Directory", "Drive"})
            for (const auto &type : {"ContextMenuHandlers", "DragDropHandlers"})
                paths.append(QStringLiteral("Software\\Classes\\") + prefix + "\\shellex\\" + type +
                             "\\SevenzipDesktop");
        return paths;
    }

  public:
    OwnedRegistration() = default;
    explicit OwnedRegistration(DWORD child) : _keys(Paths()), _owner(child) {}
    LONG Reserve() {
        const QString expected = qEnvironmentVariable("SEVENZIP_WINDOWS_RUNNER_HOST");
        wchar_t computer[MAX_COMPUTERNAME_LENGTH + 1]{};
        DWORD length = MAX_COMPUTERNAME_LENGTH + 1;
        if (!_deleteTree || expected.isEmpty() || !GetComputerNameW(computer, &length) ||
            QString::fromWCharArray(computer).compare(expected, Qt::CaseInsensitive) != 0)
            return ERROR_ACCESS_DENIED;
        HKEY approved = nullptr;
        if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, kApproved, 0, KEY_QUERY_VALUE, &approved) ==
            ERROR_SUCCESS) {
            const LONG exists =
                RegQueryValueExW(approved, kSevenzipShellClsid, nullptr, nullptr, nullptr, nullptr);
            RegCloseKey(approved);
            if (exists != ERROR_FILE_NOT_FOUND)
                return ERROR_ALREADY_EXISTS;
        }
        for (const auto &path : Paths()) {
            HKEY key = nullptr;
            DWORD disposition = 0;
            const LONG opened =
                RegCreateKeyExW(HKEY_CURRENT_USER, reinterpret_cast<LPCWSTR>(path.utf16()), 0,
                                nullptr, 0, KEY_SET_VALUE, nullptr, &key, &disposition);
            if (opened != ERROR_SUCCESS)
                return opened;
            if (disposition != REG_CREATED_NEW_KEY) {
                RegCloseKey(key);
                return ERROR_ALREADY_EXISTS;
            }
            _keys.append(path);
            const DWORD owner = GetCurrentProcessId();
            const LONG marked = RegSetValueExW(
                key, kOwner, 0, REG_DWORD, reinterpret_cast<const BYTE *>(&owner), sizeof(owner));
            RegCloseKey(key);
            if (marked != ERROR_SUCCESS) {
                // This key was just created by this reservation and has no
                // children yet; release it even if its marker write fails.
                RegDeleteKeyW(HKEY_CURRENT_USER, reinterpret_cast<LPCWSTR>(path.utf16()));
                _keys.removeLast();
                return marked;
            }
        }
        return ERROR_SUCCESS;
    }
    void Clear() {
        bool foundOwner = false;
        for (auto it = _keys.crbegin(); it != _keys.crend(); ++it) {
            HKEY key = nullptr;
            const auto path = reinterpret_cast<LPCWSTR>(it->utf16());
            if (RegOpenKeyExW(HKEY_CURRENT_USER, path, 0, KEY_QUERY_VALUE, &key) != ERROR_SUCCESS)
                continue;
            DWORD owner = 0, type = 0, bytes = sizeof(owner);
            const LONG result = RegQueryValueExW(key, kOwner, nullptr, &type,
                                                 reinterpret_cast<BYTE *>(&owner), &bytes);
            RegCloseKey(key);
            if (result == ERROR_SUCCESS && type == REG_DWORD && owner == _owner) {
                foundOwner = true;
                _deleteTree(HKEY_CURRENT_USER, path);
                RegDeleteKeyW(HKEY_CURRENT_USER, path);
            }
        }
        // A marker proves this reservation passed the absent Approved guard.
        if (foundOwner) {
            HKEY approved = nullptr;
            if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, kApproved, 0, KEY_SET_VALUE, &approved) ==
                ERROR_SUCCESS) {
                RegDeleteValueW(approved, kSevenzipShellClsid);
                RegCloseKey(approved);
            }
        }
    }
    bool Absent() const {
        for (const auto &path : _keys) {
            HKEY key = nullptr;
            if (RegOpenKeyExW(HKEY_CURRENT_USER, reinterpret_cast<LPCWSTR>(path.utf16()), 0,
                              KEY_READ, &key) == ERROR_SUCCESS) {
                RegCloseKey(key);
                return false;
            }
        }
        return true;
    }
    ~OwnedRegistration() { Clear(); }
};
} // namespace

class NativeMenuIntegrationTests final : public QObject {
    Q_OBJECT
    void OpenStandardDesktopProcess(HANDLE parent, HANDLE &shell) {
        DWORD pid = 0;
        QVERIFY(GetWindowThreadProcessId(GetShellWindow(), &pid));
        HANDLE process =
            OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION | PROCESS_CREATE_PROCESS, FALSE, pid);
        QVERIFY(process);
        auto closeProcess = qScopeGuard([&] { CloseHandle(process); });
        HANDLE token = nullptr;
        QVERIFY(OpenProcessToken(process, TOKEN_QUERY, &token));
        const auto closeToken = qScopeGuard([&] { CloseHandle(token); });
        DWORD bytes = 0;
        QByteArray ownUser(sizeof(TOKEN_USER) + SECURITY_MAX_SID_SIZE, 0);
        QByteArray shellUser(ownUser.size(), 0);
        QVERIFY(
            GetTokenInformation(parent, TokenUser, ownUser.data(), DWORD(ownUser.size()), &bytes));
        QVERIFY(GetTokenInformation(token, TokenUser, shellUser.data(), DWORD(shellUser.size()),
                                    &bytes));
        QVERIFY(EqualSid(reinterpret_cast<TOKEN_USER *>(ownUser.data())->User.Sid,
                         reinterpret_cast<TOKEN_USER *>(shellUser.data())->User.Sid));
        DWORD ownSession = 0, shellSession = 0;
        QVERIFY(
            GetTokenInformation(parent, TokenSessionId, &ownSession, sizeof(ownSession), &bytes));
        QVERIFY(GetTokenInformation(token, TokenSessionId, &shellSession, sizeof(shellSession),
                                    &bytes));
        QCOMPARE(shellSession, ownSession);
        TOKEN_ELEVATION elevation{};
        QVERIFY(GetTokenInformation(token, TokenElevation, &elevation, sizeof(elevation), &bytes));
        QVERIFY(!elevation.TokenIsElevated);
        shell = process;
        closeProcess.dismiss();
    }
    void RunStandardTokenChild(HANDLE token, const QTemporaryDir &fixture,
                               const QProcessEnvironment &environment) {
        HANDLE shell = nullptr;
        OpenStandardDesktopProcess(token, shell);
        QVERIFY(shell);
        const auto closeShell = qScopeGuard([&] { CloseHandle(shell); });
        SIZE_T attributeBytes = 0;
        InitializeProcThreadAttributeList(nullptr, 1, 0, &attributeBytes);
        QVERIFY(attributeBytes > 0);
        QByteArray attributes(qsizetype(attributeBytes), 0);
        auto *list = reinterpret_cast<LPPROC_THREAD_ATTRIBUTE_LIST>(attributes.data());
        QVERIFY(InitializeProcThreadAttributeList(list, 1, 0, &attributeBytes));
        const auto deleteAttributes = qScopeGuard([&] { DeleteProcThreadAttributeList(list); });
        QVERIFY(UpdateProcThreadAttribute(list, 0, PROC_THREAD_ATTRIBUTE_PARENT_PROCESS, &shell,
                                          sizeof(shell), nullptr, nullptr));
        const std::wstring application =
            QDir::toNativeSeparators(QCoreApplication::applicationFilePath()).toStdWString();
        const QString log = fixture.filePath("standard-token-child.txt");
        std::wstring command =
            (QStringLiteral("\"") + QString::fromStdWString(application) +
             "\" sourcePageRegistersRequeriesActivatesAndUnregisters -silent -o \"" +
             QDir::toNativeSeparators(log) + ",txt\"")
                .toStdWString();
        const QString block = environment.toStringList().join(QChar(0)) + QChar(0) + QChar(0);
        STARTUPINFOEXW startup{};
        startup.StartupInfo.cb = sizeof(startup);
        startup.lpAttributeList = list;
        PROCESS_INFORMATION process{};
        // The desktop token belongs to this same user and session. Run the per-user COM
        // consumer with its ordinary token even when privileged fixtures drive
        // the parent gate; no registry lookup override or security policy change.
        QVERIFY2(CreateProcessW(
                     application.c_str(), command.data(), nullptr, nullptr, FALSE,
                     CREATE_UNICODE_ENVIRONMENT | CREATE_NO_WINDOW | EXTENDED_STARTUPINFO_PRESENT,
                     const_cast<ushort *>(block.utf16()), nullptr, &startup.StartupInfo, &process),
                 qPrintable(QString("CreateProcessW error %1").arg(GetLastError())));
        OwnedRegistration emergency(process.dwProcessId);
        const auto stop = qScopeGuard([&] {
            if (WaitForSingleObject(process.hProcess, 0) == WAIT_TIMEOUT) {
                TerminateProcess(process.hProcess, 1);
                WaitForSingleObject(process.hProcess, 5000);
            }
            emergency.Clear();
            CloseHandle(process.hThread);
            CloseHandle(process.hProcess);
        });
        QCOMPARE(WaitForSingleObject(process.hProcess, 45000), DWORD(WAIT_OBJECT_0));
        DWORD code = 1;
        QVERIFY(GetExitCodeProcess(process.hProcess, &code));
        QFile output(log);
        QVERIFY(output.open(QIODevice::ReadOnly));
        const QByteArray text = output.readAll();
        QVERIFY2(code == 0, text.constData());
        emergency.Clear();
        QVERIFY(emergency.Absent());
    }
    void RunUnicodeSettingsChild() {
        QTemporaryDir fixture;
        QProcess child;
        auto environment = QProcessEnvironment::systemEnvironment();
        environment.insert("SEVENZIP_MENU_TEST_CHILD", "1");
        environment.insert("SEVENZIP_SETTINGS_PATH", fixture.filePath("menu 空.ini"));
        HANDLE token = nullptr;
        QVERIFY(OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &token));
        const auto closeToken = qScopeGuard([&] { CloseHandle(token); });
        TOKEN_ELEVATION elevation{};
        DWORD bytes = 0;
        QVERIFY(GetTokenInformation(token, TokenElevation, &elevation, sizeof(elevation), &bytes));
        if (elevation.TokenIsElevated) {
            RunStandardTokenChild(token, fixture, environment);
            return;
        }
        child.setProcessEnvironment(environment);
        child.start(QCoreApplication::applicationFilePath(),
                    {"sourcePageRegistersRequeriesActivatesAndUnregisters", "-silent"});
        QVERIFY(child.waitForStarted(5000));
        OwnedRegistration emergency(DWORD(child.processId()));
        const auto stop = qScopeGuard([&] {
            if (child.state() != QProcess::NotRunning) {
                child.kill();
                child.waitForFinished(5000);
            }
            emergency.Clear();
        });
        QVERIFY(child.waitForFinished(45000));
        const QByteArray output = child.readAllStandardOutput() + child.readAllStandardError();
        QVERIFY2(child.exitStatus() == QProcess::NormalExit && child.exitCode() == 0,
                 output.constData());
        emergency.Clear();
        QVERIFY(emergency.Absent());
    }
    void QueryRegisteredDll(const QString &fileName) {
        NWindows::NCOM::CComInitializer initialized;
        const GUID classID = {
            0x52D06B47, 0xDA74, 0x4A5F, {0x8B, 0x5E, 0x44, 0x94, 0xC7, 0x29, 0xCC, 0x9C}};
        CMyComPtr<IContextMenu> menu;
        QCOMPARE(CoCreateInstance(classID, nullptr, CLSCTX_INPROC_SERVER, IID_IContextMenu,
                                  reinterpret_cast<void **>(&menu)),
                 S_OK);
        CMyComPtr<IShellExtInit> selection;
        QCOMPARE(menu.QueryInterface(IID_IShellExtInit, &selection), S_OK);
        CMyComPtr<IShellItem> item;
        const auto native = QDir::toNativeSeparators(fileName).toStdWString();
        QCOMPARE(SHCreateItemFromParsingName(native.c_str(), nullptr, IID_IShellItem,
                                             reinterpret_cast<void **>(&item)),
                 S_OK);
        CMyComPtr<IDataObject> data;
        QCOMPARE(item->BindToHandler(nullptr, BHID_DataObject, IID_IDataObject,
                                     reinterpret_cast<void **>(&data)),
                 S_OK);
        QCOMPARE(selection->Initialize(nullptr, data, nullptr), S_OK);
        const HMENU popup = CreatePopupMenu();
        QVERIFY(popup);
        const auto destroy = qScopeGuard([&] { DestroyMenu(popup); });
        const HRESULT queried = menu->QueryContextMenu(popup, 0, 100, 999, CMF_NORMAL);
        QVERIFY(SUCCEEDED(queried) && HRESULT_CODE(queried) > 0);
        QCOMPARE(HRESULT_CODE(queried), 1);
        QCOMPARE(GetMenuItemCount(popup), 2); // Source flat menu separator and command.
        MENUITEMINFOW command{};
        command.cbSize = sizeof(command);
        command.fMask = MIIM_SUBMENU | MIIM_CHECKMARKS;
        QVERIFY(GetMenuItemInfoW(popup, 1, TRUE, &command));
        QVERIFY(!command.hSubMenu && command.hbmpUnchecked);
        bool extract = false;
        for (unsigned i = 0; i < HRESULT_CODE(queried); ++i) {
            wchar_t verb[256]{};
            if (SUCCEEDED(menu->GetCommandString(i, GCS_VERBW, nullptr,
                                                 reinterpret_cast<LPSTR>(verb), 256)))
                extract |= QString::fromWCharArray(verb) == "SevenZipExtractHere";
        }
        QVERIFY(extract);
        NWindows::NShell::CItemIDList pidl;
        QCOMPARE(SHParseDisplayName(native.c_str(), nullptr, &pidl, 0, nullptr), S_OK);
        CMyComPtr<IShellFolder> parent;
        PCUITEMID_CHILD child = nullptr;
        QCOMPARE(SHBindToParent(pidl, IID_IShellFolder, reinterpret_cast<void **>(&parent), &child),
                 S_OK);
        CMyComPtr<IContextMenu> system;
        QCOMPARE(parent->GetUIObjectOf(nullptr, 1, &child, IID_IContextMenu, nullptr,
                                       reinterpret_cast<void **>(&system)),
                 S_OK);
        const HMENU systemPopup = CreatePopupMenu();
        QVERIFY(systemPopup);
        const auto destroySystem = qScopeGuard([&] { DestroyMenu(systemPopup); });
        const HRESULT systemCount = system->QueryContextMenu(systemPopup, 0, 1, 0x7fff, CMF_NORMAL);
        QVERIFY(SUCCEEDED(systemCount));
        bool discovered = false;
        for (unsigned i = 0; i < HRESULT_CODE(systemCount); ++i) {
            wchar_t verb[256]{};
            if (SUCCEEDED(system->GetCommandString(i, GCS_VERBW, nullptr,
                                                   reinterpret_cast<LPSTR>(verb), 256)))
                discovered |= QString::fromWCharArray(verb) == "SevenZipExtractHere";
        }
        QVERIFY(discovered);
    }

  private slots:
    void sourcePageRegistersRequeriesActivatesAndUnregisters() {
        if (qEnvironmentVariable("SEVENZIP_MENU_TEST_CHILD") != "1") {
            RunUnicodeSettingsChild();
            return;
        }
        HANDLE token = nullptr;
        QVERIFY(OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &token));
        const auto closeToken = qScopeGuard([&] { CloseHandle(token); });
        TOKEN_ELEVATION elevation{};
        DWORD bytes = 0;
        QVERIFY(GetTokenInformation(token, TokenElevation, &elevation, sizeof(elevation), &bytes));
        QVERIFY(!elevation.TokenIsElevated);
        OwnedRegistration owned;
        QCOMPARE(owned.Reserve(), LONG(ERROR_SUCCESS));
        QTemporaryDir fixture;
        const QString settings = qEnvironmentVariable("SEVENZIP_SETTINGS_PATH");
        QVERIFY(settings.endsWith(QStringLiteral("menu 空.ini")));
        QCOMPARE(FmSettingsPath(), settings);
        auto services = CreateNativeMenuPageServices();
        FString library = services->GetModuleDirPrefix();
        library += services->GetDllName(0);
        QVERIFY(services->DoesFileExist(library));
        CMenuPage page(settings);
        page.show();
        QVERIFY(QTest::qWaitForWindowExposed(&page));
        auto *native = page.findChild<QCheckBox *>("shellIntegration");
        QVERIFY(native->isEnabled() && !native->isChecked());
        native->click();
        auto *items = page.findChild<QListWidget *>("menuItems");
        QVERIFY(items->isEnabled());
        for (int i = 0; i < items->count(); ++i) {
            auto *item = items->item(i);
            item->setCheckState(item->data(Qt::UserRole).toUInt() == NContextMenuFlags::kExtractHere
                                    ? Qt::Checked
                                    : Qt::Unchecked);
        }
        page.findChild<QCheckBox *>("cascadedMenu")->click();
        page.findChild<QCheckBox *>("menuIcons")->click();
        QVERIFY(page.IsModified());
        QCOMPARE(page.OnApply(), 0L);
        QVERIFY(native->isChecked() && !page.IsModified());
        QVERIFY(services->CheckContextMenuHandler(fs2us(library), 0));
        SHChangeNotify(SHCNE_ASSOCCHANGED, SHCNF_IDLIST, nullptr, nullptr);
        QFile input(fixture.filePath("архив 空 with spaces.zip"));
        QVERIFY(input.open(QIODevice::WriteOnly));
        QCOMPARE(input.write("fixture"), qint64(7));
        input.close();
        QueryRegisteredDll(input.fileName());
        QVERIFY(!QTest::currentTestFailed());
        native->click();
        QCOMPARE(page.OnApply(), 0L);
        QVERIFY(!native->isChecked() && !services->CheckContextMenuHandler(fs2us(library), 0));
        CoFreeUnusedLibrariesEx(0, 0);
        owned.Clear();
        QVERIFY(owned.Absent());
    }
};
QTEST_MAIN(NativeMenuIntegrationTests)
#include "native_menu_integration_tests.moc"
