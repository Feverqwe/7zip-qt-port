#include "platform/shell/qtnativemenupageservices.h"
#include "platform/shell/qtmenupageservices.h"
#include <QCoreApplication>
#include <QDir>
#include <QMessageBox>
#ifdef _WIN32
#include "Common/StringConvert.h"
#include "Windows/ErrorMsg.h"
#include "Windows/FileFind.h"
#include "Windows/Registry.h"
#include "platform/shell/RegistryContextMenu.h"
#endif
namespace {
#ifdef _WIN32
using namespace NQtShellRegistry;
class CNativeKey final : public IKey {
    NWindows::NRegistry::CKey _key;

  public:
    LONG Open(HKEY root, const UString &path, UInt32 access, bool create) {
        return create ? _key.Create(root, path.Ptr(), REG_NONE, REG_OPTION_NON_VOLATILE, access)
                      : _key.Open(root, path.Ptr(), access);
    }
    LONG QueryValue(const wchar_t *name, UString &value) override {
        return _key.QueryValue(name, value);
    }
    LONG SetValue(const wchar_t *name, const UString &value) override {
        return _key.SetValue(name, value.Ptr());
    }
    LONG Create(const UString &path, UInt32 access, std::unique_ptr<IKey> &key) override {
        auto child = std::make_unique<CNativeKey>();
        const LONG result = child->Open(_key, path, access, true);
        key = result == 0 ? std::move(child) : nullptr;
        return result;
    }
};
class CNativeRegistry final : public IServices {
    static UString Path(ERoot root, const UString &path) {
        return root == ERoot::Classes ? UString(L"Software\\Classes\\") + path : path;
    }
    static HKEY Root(ERoot root) {
        return root == ERoot::Classes ? HKEY_CURRENT_USER : HKEY_LOCAL_MACHINE;
    }

  public:
    LONG Open(ERoot root, const UString &path, UInt32 access, bool create,
              std::unique_ptr<IKey> &key) override {
        auto opened = std::make_unique<CNativeKey>();
        const LONG result = opened->Open(Root(root), Path(root, path), access, create);
        key = result == 0 ? std::move(opened) : nullptr;
        return result;
    }
    LONG Delete(ERoot root, const UString &path, UInt32 wow) override {
        return RegDeleteKeyExW(Root(root), Path(root, path).Ptr(), wow, 0);
    }
};
#endif
class CNativeMenuPage final : public IQtMenuPageServices {
#ifdef _WIN32
    CNativeRegistry _registry;
#endif
    bool _commands;

  public:
    explicit CNativeMenuPage(bool commands) : _commands(commands) {}
    bool CommandsAvailable() const override { return _commands; }
    bool ShellIntegrationSupported() const override {
#ifdef _WIN32
        return true;
#else
        return false;
#endif
    }
    bool Is64Bit() const override { return sizeof(void *) == 8; }
    bool OtherBitnessAvailable() const override {
#ifdef _WIN32
        BOOL wow = FALSE;
        return Is64Bit() || (IsWow64Process(GetCurrentProcess(), &wow) && wow);
#else
        return false;
#endif
    }
    FString GetModuleDirPrefix() const override {
        QString path = QDir::toNativeSeparators(QCoreApplication::applicationDirPath());
        path += QDir::separator();
        return us2fs(UString(path.toStdWString().c_str()));
    }
    const char *GetDllName(unsigned index) const override {
        return index == 0 ? "7-zip.dll" : (Is64Bit() ? "7-zip32.dll" : "7-zip64.dll");
    }
    bool DoesFileExist(const FString &path) const override {
#ifdef _WIN32
        return NWindows::NFile::NFind::DoesFileExist_Raw(path);
#else
        (void)path;
        return false;
#endif
    }
    bool CheckContextMenuHandler(const UString &path, UInt32 wow) override {
#ifdef _WIN32
        return ::CheckContextMenuHandler(path, wow, _registry);
#else
        (void)path;
        (void)wow;
        return false;
#endif
    }
    LONG SetContextMenuHandler(bool value, const UString &path, UInt32 wow) override {
#ifdef _WIN32
        return ::SetContextMenuHandler(value, path, wow, _registry);
#else
        (void)value;
        (void)path;
        (void)wow;
        return E_NOTIMPL;
#endif
    }
    void ShowMenuErrorMessage(LONG error, QWidget *parent) override {
#ifdef _WIN32
        QMessageBox::critical(parent, QStringLiteral("7-Zip"),
                              QString::fromWCharArray(NWindows::NError::MyFormatMessage(error)));
#else
        (void)error;
        (void)parent;
#endif
    }
};
} // namespace
std::shared_ptr<IQtMenuPageServices> CreateNativeMenuPageServices(bool commandsAvailable) {
    return std::make_shared<CNativeMenuPage>(commandsAvailable);
}
