#include "CPP/7zip/UI/Explorer/RegistryContextMenu.h"
#include <QHash>
#include <QtTest>

namespace {
using namespace NQtShellRegistry;
QString qs(const UString &s) { return QString::fromWCharArray(s.Ptr()); }
class CRegistry final : public IServices {
    class CHandle final : public IKey {
        CRegistry &_registry;
        QString _path;
        ERoot _root;
        UInt32 _view;

      public:
        CHandle(CRegistry &registry, QString path, ERoot root, UInt32 view)
            : _registry(registry), _path(std::move(path)), _root(root), _view(view) {}
        LONG QueryValue(const wchar_t *name, UString &value) override {
            const QString key = name ? QString::fromWCharArray(name) : QString();
            const auto &values = _registry.Values[_path];
            if (!values.contains(key))
                return 2;
            value = values[key].toStdWString().c_str();
            return 0;
        }
        LONG SetValue(const wchar_t *name, const UString &value) override {
            if (_registry.FailWrite && _path.endsWith("inprocserver32"))
                return 5;
            _registry.Values[_path].insert(name ? QString::fromWCharArray(name) : QString(),
                                           qs(value));
            return 0;
        }
        LONG Create(const UString &path, UInt32 access, std::unique_ptr<IKey> &key) override {
            return _registry.Open(
                _root, UString((_registry.RawPath(_path) + "\\" + qs(path)).toStdWString().c_str()),
                access, true, key);
        }
    };
    QString Address(ERoot root, const QString &path, UInt32 view) const {
        return QString::number(int(root)) + ":" + QString::number(view) + ":" + path.toLower();
    }

  public:
    QHash<QString, QHash<QString, QString>> Values;
    bool FailWrite = false, DenyApproved = false;
    QString RawPath(const QString &address) const { return address.section(':', 2); }
    LONG Open(ERoot root, const UString &path, UInt32 access, bool create,
              std::unique_ptr<IKey> &key) override {
        key.reset();
        if (DenyApproved && root == ERoot::Machine)
            return 5;
        const UInt32 view = access & (kWow32 | kWow64);
        const QString address = Address(root, qs(path), view);
        if (!create && !Values.contains(address))
            return 2;
        if (create)
            Values[address];
        key = std::make_unique<CHandle>(*this, address, root, view);
        return 0;
    }
    LONG Delete(ERoot root, const UString &path, UInt32 wow) override {
        const QString address = Address(root, qs(path), wow);
        return Values.remove(address) ? 0 : 2;
    }
    QString Value(const QString &path, UInt32 view, const QString &name = {}) const {
        return Values.value(Address(ERoot::Classes, path, view)).value(name);
    }
};
const UString kPath(L"C:\\Installed build 空\\sevenzip-shell.dll");
QString clsid() { return QString::fromWCharArray(kSevenzipShellClsid); }
QString classPath() { return "CLSID\\" + clsid(); }
} // namespace

class ShellRegistryTests final : public QObject {
    Q_OBJECT
  private slots:
    void sourceRegistrationViewsAndSharedHandlers() {
        CRegistry registry;
        for (const wchar_t *foreign :
             {L"CLSID\\{23170F69-40C1-278A-1000-000100020000}\\InprocServer32",
              L"*\\shellex\\ContextMenuHandlers\\7-Zip",
              L"Drive\\shellex\\ContextMenuHandlers\\7-Zip"}) {
            std::unique_ptr<IKey> key;
            QCOMPARE(
                registry.Open(ERoot::Classes, UString(foreign), kAllAccess | kWow64, true, key),
                LONG(0));
            QCOMPARE(key->SetValue(nullptr, UString(L"official handler preserved")), LONG(0));
        }
        const auto foreignValues = registry.Values;
        QVERIFY(!CheckContextMenuHandler(kPath, kWow64, registry));
        QCOMPARE(SetContextMenuHandler(true, kPath, kWow64, registry), LONG(0));
        QVERIFY(CheckContextMenuHandler(kPath, kWow64, registry));
        QVERIFY(!CheckContextMenuHandler(kPath, kWow32, registry));
        QCOMPARE(registry.Value(classPath() + "\\InprocServer32", kWow64), qs(kPath));
        QCOMPARE(registry.Value(classPath() + "\\InprocServer32", kWow64, "ThreadingModel"),
                 QString("Apartment"));
        for (const QString prefix : {QString("*"), QString("Folder"), QString("Directory")})
            QCOMPARE(
                registry.Value(prefix + "\\shellex\\ContextMenuHandlers\\SevenzipDesktop", kWow64),
                clsid());
        for (const QString prefix : {QString("Directory"), QString("Drive")})
            QCOMPARE(
                registry.Value(prefix + "\\shellex\\DragDropHandlers\\SevenzipDesktop", kWow64),
                clsid());
        QVERIFY(registry.Value("Drive\\shellex\\ContextMenuHandlers\\SevenzipDesktop", kWow64)
                    .isEmpty());
        QCOMPARE(SetContextMenuHandler(true, kPath, kWow32, registry), LONG(0));
        QCOMPARE(SetContextMenuHandler(false, kPath, kWow64, registry), LONG(0));
        QVERIFY(!CheckContextMenuHandler(kPath, kWow64, registry));
        QVERIFY(CheckContextMenuHandler(kPath, kWow32, registry));
        QVERIFY(
            !registry.Value("*\\shellex\\ContextMenuHandlers\\SevenzipDesktop", kWow64).isEmpty());
        for (auto it = foreignValues.cbegin(); it != foreignValues.cend(); ++it)
            QCOMPARE(registry.Values.value(it.key()), it.value());
    }
    void failuresAndIdentityAreObservable() {
        CRegistry registry;
        registry.DenyApproved = true;
        QCOMPARE(SetContextMenuHandler(true, kPath, 0, registry), LONG(0));
        QVERIFY(CheckContextMenuHandler(kPath, 0, registry));
        QVERIFY(!CheckContextMenuHandler(UString(L"C:\\wrong.dll"), 0, registry));
        QVERIFY(CheckContextMenuHandler(UString(L"c:\\installed BUILD 空\\SEVENZIP-SHELL.DLL"), 0,
                                        registry));
        registry.FailWrite = true;
        QCOMPARE(SetContextMenuHandler(true, UString(L"C:\\replacement.dll"), 0, registry),
                 LONG(5));
        QVERIFY(CheckContextMenuHandler(kPath, 0, registry));
        for (const QString &key : registry.Values.keys()) {
            QVERIFY(!key.contains("23170f69"));
            QVERIFY(!key.endsWith("\\7-zip"));
        }
    }
};
QTEST_APPLESS_MAIN(ShellRegistryTests)
#include "shell_registry_tests.moc"
