#include "qtnativemenupageservices.h"
#include "qtmenupageservices.h"
#ifdef _WIN32
#include "CPP/7zip/UI/Explorer/RegistryContextMenu.h"
#include "Common/StringConvert.h"
#include "Windows/ErrorMsg.h"
#include "Windows/FileFind.h"
#include "Windows/Registry.h"
#include <QCoreApplication>
#include <QDir>
#include <QMessageBox>
namespace {
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
class CNativeMenuPage final : public IQtMenuPageServices {
    CNativeRegistry _registry;
    bool _commands;

  public:
    explicit CNativeMenuPage(bool commands) : _commands(commands) {}
    bool CommandsAvailable() const override { return _commands; }
    bool Is64Bit() const override { return sizeof(void *) == 8; }
    bool OtherBitnessAvailable() const override {
        BOOL wow = FALSE;
        return Is64Bit() || (IsWow64Process(GetCurrentProcess(), &wow) && wow);
    }
    FString GetModuleDirPrefix() const override {
        QString path = QDir::toNativeSeparators(QCoreApplication::applicationDirPath());
        path += QDir::separator();
        return us2fs(UString(path.toStdWString().c_str()));
    }
    const char *GetDllName(unsigned index) const override {
        return index == 0 ? "sevenzip_shell.dll"
                          : (Is64Bit() ? "sevenzip_shell32.dll" : "sevenzip_shell64.dll");
    }
    bool DoesFileExist(const FString &path) const override {
        return NWindows::NFile::NFind::DoesFileExist_Raw(path);
    }
    bool CheckContextMenuHandler(const UString &path, UInt32 wow) override {
        return ::CheckContextMenuHandler(path, wow, _registry);
    }
    LONG SetContextMenuHandler(bool value, const UString &path, UInt32 wow) override {
        return ::SetContextMenuHandler(value, path, wow, _registry);
    }
    void ShowMenuErrorMessage(LONG error, QWidget *parent) override {
        QMessageBox::critical(parent, QStringLiteral("7-Zip"),
                              QString::fromWCharArray(NWindows::NError::MyFormatMessage(error)));
    }
};
} // namespace
#endif
std::shared_ptr<IQtMenuPageServices> CreateNativeMenuPageServices(bool commandsAvailable) {
#ifdef _WIN32
    return std::make_shared<CNativeMenuPage>(commandsAvailable);
#else
    (void)commandsAvailable;
    return {};
#endif
}
