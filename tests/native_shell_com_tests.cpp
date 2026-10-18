#include "Common/MyTypes.h"

#include "7zip/UI/Explorer/ContextMenuFlags.h"
#include "CPP/7zip/UI/Explorer/ContextMenu.h"
#include "CPP/7zip/UI/FileManager/RegistryUtils.h"
#include "Common/MyCom.h"
#include "Windows/COM.h"
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QLibrary>
#include <QProcess>
#include <QProcessEnvironment>
#include <QScopeGuard>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QtTest>
#include <array>

namespace {
constexpr GUID kClass = {
    0x52D06B47, 0xDA74, 0x4A5F, {0x8B, 0x5E, 0x44, 0x94, 0xC7, 0x29, 0xCC, 0x9C}};
using CanUnload = HRESULT(STDAPICALLTYPE *)();
using GetClass = HRESULT(STDAPICALLTYPE *)(REFCLSID, REFIID, void **);
struct TaskString {
    LPWSTR value = nullptr;
    ~TaskString() { CoTaskMemFree(value); }
};
} // namespace

class NativeShellComTests final : public QObject {
    Q_OBJECT
  private slots:
    void actualNativeHostHasNoQtApplication_data() {
        QTest::addColumn<QString>("language");
        QTest::addColumn<QString>("caption");
        QTest::newRow("en") << QString("-") << QString("Open archive");
        QTest::newRow("ru") << QString("ru") << QString::fromUtf8("Открыть архив");
    }
    void actualNativeHostHasNoQtApplication() {
        QFETCH(QString, language);
        QFETCH(QString, caption);
        QTemporaryDir fixture;
        QVERIFY(fixture.isValid());
        const QString settings = fixture.filePath("language.ini");
        SaveRegLang(UString(language.toStdWString().c_str()), settings);
        CContextMenuInfo menuSettings;
        menuSettings.Load(settings);
        menuSettings.Flags_Def = true;
        menuSettings.Flags = NContextMenuFlags::kOpen;
        menuSettings.Cascaded.SetTrueTrue();
        menuSettings.Save(settings);
        const QString path = fixture.filePath(QStringLiteral("host архив 密碼 space.zip"));
        QFile file(path);
        QVERIFY(file.open(QIODevice::WriteOnly));
        QCOMPARE(file.write("fixture"), qint64(7));
        file.close();
        QProcess host;
        auto environment = QProcessEnvironment::systemEnvironment();
        environment.insert("SEVENZIP_SETTINGS_PATH", settings);
        host.setProcessEnvironment(environment);
        host.setProgram(QString::fromUtf8(NATIVE_SHELL_HOST));
        host.setArguments({QDir::toNativeSeparators(QString::fromUtf8(NATIVE_SHELL_LIBRARY)),
                           QDir::toNativeSeparators(path), caption});
        host.start();
        QVERIFY(host.waitForStarted());
        QVERIFY(host.waitForFinished(30000));
        const QByteArray diagnostics = host.readAllStandardError() + host.readAllStandardOutput();
        QCOMPARE(host.exitStatus(), QProcess::NormalExit);
        QVERIFY2(host.exitCode() == 0, diagnostics.constData());
        QVERIFY(file.open(QIODevice::ReadOnly));
        QCOMPARE(file.readAll(), QByteArray("fixture"));
    }

    void settingsOwnerIsIndependentOfHostIdentity() {
        const QString oldName = QCoreApplication::applicationName();
        const QByteArray overridePath = qgetenv("SEVENZIP_SETTINGS_PATH");
        struct Restore {
            QString name;
            QByteArray path;
            ~Restore() {
                QCoreApplication::setApplicationName(name);
                qputenv("SEVENZIP_SETTINGS_PATH", path);
                QStandardPaths::setTestModeEnabled(false);
            }
        } restore{oldName, overridePath};
        QStandardPaths::setTestModeEnabled(true);
        qunsetenv("SEVENZIP_SETTINGS_PATH");
        QCoreApplication::setApplicationName("Sevenzip");
        const QString project = FmSettingsPath();
        QCoreApplication::setApplicationName("Explorer host fixture");
        QCOMPARE(FmSettingsPath(), project);
        QCOMPARE(project,
                 QDir(QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation))
                     .filePath("Sevenzip/toolbar.ini"));
    }

    void factoryInterfaceIdentityAndUnload() {
        NWindows::NCOM::CComInitializer initialized;
        QLibrary library(QString::fromUtf8(NATIVE_SHELL_LIBRARY));
        QVERIFY2(library.load(), qPrintable(library.errorString()));
        const auto canUnload = reinterpret_cast<CanUnload>(library.resolve("DllCanUnloadNow"));
        const auto getClass = reinterpret_cast<GetClass>(library.resolve("DllGetClassObject"));
        QVERIFY(canUnload);
        QVERIFY(getClass);
        QCOMPARE(canUnload(), S_OK);
        CMyComPtr<IClassFactory> factory;
        QCOMPARE(getClass(GUID_NULL, IID_IClassFactory, reinterpret_cast<void **>(&factory)),
                 CLASS_E_CLASSNOTAVAILABLE);
        QVERIFY(!factory);
        QCOMPARE(getClass(kClass, IID_IClassFactory, reinterpret_cast<void **>(&factory)), S_OK);
        QCOMPARE(canUnload(), S_FALSE);
        CMyComPtr<IContextMenu> menu;
        QCOMPARE(
            factory->CreateInstance(factory, IID_IContextMenu, reinterpret_cast<void **>(&menu)),
            CLASS_E_NOAGGREGATION);
        QVERIFY(!menu);
        QCOMPARE(factory->CreateInstance(nullptr, GUID_NULL, reinterpret_cast<void **>(&menu)),
                 E_NOINTERFACE);
        QVERIFY(!menu);
        QCOMPARE(
            factory->CreateInstance(nullptr, IID_IContextMenu, reinterpret_cast<void **>(&menu)),
            S_OK);
        CMyComPtr<IShellExtInit> selection;
        CMyComPtr<IExplorerCommand> command;
        CMyComPtr<IEnumExplorerCommand> enumeration;
        QCOMPARE(menu.QueryInterface(IID_IShellExtInit, &selection), S_OK);
        QCOMPARE(menu.QueryInterface(IID_IExplorerCommand, &command), S_OK);
        QCOMPARE(menu.QueryInterface(IID_IEnumExplorerCommand, &enumeration), S_OK);
        QCOMPARE(selection->Initialize(nullptr, nullptr, nullptr), E_INVALIDARG);
        QCOMPARE(menu->QueryContextMenu(nullptr, 0, 100, 999, CMF_NORMAL), HRESULT(0));
        QCOMPARE(menu->GetCommandString(0, GCS_VALIDATEW, nullptr, nullptr, 0), S_FALSE);
        CMINVOKECOMMANDINFO invalid{};
        invalid.cbSize = sizeof(invalid);
        invalid.lpVerb = "SevenZipUnknown";
        QCOMPARE(menu->InvokeCommand(&invalid), E_INVALIDARG);
        CMyComPtr<IUnknown> identity;
        QCOMPARE(menu.QueryInterface(IID_IUnknown, &identity), S_OK);
        for (IUnknown *interfacePointer :
             {static_cast<IUnknown *>(selection), static_cast<IUnknown *>(command),
              static_cast<IUnknown *>(enumeration)}) {
            CMyComPtr<IUnknown> other;
            QCOMPARE(
                interfacePointer->QueryInterface(IID_IUnknown, reinterpret_cast<void **>(&other)),
                S_OK);
            QCOMPARE(static_cast<IUnknown *>(other), static_cast<IUnknown *>(identity));
        }
        factory.Release();
        QCOMPARE(canUnload(), S_FALSE);
        identity.Release();
        menu.Release();
        selection.Release();
        command.Release();
        QCOMPARE(canUnload(), S_FALSE);
        enumeration.Release();
        QCOMPARE(canUnload(), S_OK);
        QVERIFY(library.unload());
    }

    void serverLocksRetainModuleAcrossFactoryLifetimes() {
        NWindows::NCOM::CComInitializer initialized;
        QLibrary library(QString::fromUtf8(NATIVE_SHELL_LIBRARY));
        QVERIFY2(library.load(), qPrintable(library.errorString()));
        const auto canUnload = reinterpret_cast<CanUnload>(library.resolve("DllCanUnloadNow"));
        const auto getClass = reinterpret_cast<GetClass>(library.resolve("DllGetClassObject"));
        QVERIFY(canUnload && getClass);
        QCOMPARE(canUnload(), S_OK);
        unsigned locks = 0;
        const auto cleanup = qScopeGuard([&] {
            {
                CMyComPtr<IClassFactory> factory;
                if (locks && getClass(kClass, IID_IClassFactory,
                                      reinterpret_cast<void **>(&factory)) == S_OK)
                    while (locks) {
                        factory->LockServer(FALSE);
                        --locks;
                    }
            }
            library.unload();
        });
        CMyComPtr<IClassFactory> factory;
        QCOMPARE(getClass(kClass, IID_IClassFactory, reinterpret_cast<void **>(&factory)), S_OK);
        for (unsigned i = 0; i < 2; ++i) {
            QCOMPARE(factory->LockServer(TRUE), S_OK);
            ++locks;
        }
        factory.Release();
        QCOMPARE(canUnload(), S_FALSE);
        while (locks) {
            QCOMPARE(getClass(kClass, IID_IClassFactory, reinterpret_cast<void **>(&factory)),
                     S_OK);
            QCOMPARE(factory->LockServer(FALSE), S_OK);
            --locks;
            factory.Release();
            QCOMPARE(canUnload(), locks ? S_FALSE : S_OK);
        }
    }

    void enumeratedChildrenOutliveRootAndRetainModule() {
        NWindows::NCOM::CComInitializer initialized;
        QTemporaryDir fixture;
        QVERIFY(fixture.isValid());
        const QString path = fixture.filePath(QStringLiteral("архив 密碼 with space.zip"));
        QFile file(path);
        QVERIFY(file.open(QIODevice::WriteOnly));
        QCOMPARE(file.write("fixture"), qint64(7));
        file.close();
        const std::wstring selected = QDir::toNativeSeparators(path).toStdWString();
        CMyComPtr<IShellItem> item;
        QCOMPARE(SHCreateItemFromParsingName(selected.c_str(), nullptr, IID_IShellItem,
                                             reinterpret_cast<void **>(&item)),
                 S_OK);
        CMyComPtr<IShellItemArray> items;
        QCOMPARE(SHCreateShellItemArrayFromShellItem(item, IID_IShellItemArray,
                                                     reinterpret_cast<void **>(&items)),
                 S_OK);
        QLibrary library(QString::fromUtf8(NATIVE_SHELL_LIBRARY));
        QVERIFY2(library.load(), qPrintable(library.errorString()));
        const auto canUnload = reinterpret_cast<CanUnload>(library.resolve("DllCanUnloadNow"));
        const auto getClass = reinterpret_cast<GetClass>(library.resolve("DllGetClassObject"));
        QVERIFY(canUnload && getClass);
        CMyComPtr<IClassFactory> factory;
        QCOMPARE(getClass(kClass, IID_IClassFactory, reinterpret_cast<void **>(&factory)), S_OK);
        CMyComPtr<IExplorerCommand> root;
        QCOMPARE(factory->CreateInstance(nullptr, IID_IExplorerCommand,
                                         reinterpret_cast<void **>(&root)),
                 S_OK);
        CMyComPtr<IShellExtInit> selection;
        CMyComPtr<IContextMenu> legacy;
        QCOMPARE(root.QueryInterface(IID_IShellExtInit, &selection), S_OK);
        QCOMPARE(root.QueryInterface(IID_IContextMenu, &legacy), S_OK);
        CMyComPtr<IDataObject> data;
        QCOMPARE(item->BindToHandler(nullptr, BHID_DataObject, IID_IDataObject,
                                     reinterpret_cast<void **>(&data)),
                 S_OK);
        QCOMPARE(selection->Initialize(nullptr, data, nullptr), S_OK);
        const HMENU popup = CreatePopupMenu();
        QVERIFY(popup);
        const HRESULT queried = legacy->QueryContextMenu(popup, 0, 100, 999, CMF_NORMAL);
        QVERIFY(SUCCEEDED(queried) && HRESULT_CODE(queried) > 0);
        QVERIFY(GetMenuItemCount(popup) > 0);
        QVERIFY(DestroyMenu(popup));
        selection.Release();
        legacy.Release();
        TaskString title;
        QCOMPARE(root->GetTitle(items, &title.value), S_OK);
        QCOMPARE(QString::fromWCharArray(title.value), QStringLiteral("7-Zip"));
        EXPCMDFLAGS flags{};
        QCOMPARE(root->GetFlags(&flags), S_OK);
        QCOMPARE(flags, ECF_HASSUBCOMMANDS);
        TaskString icon;
        QCOMPARE(root->GetIcon(items, &icon.value), S_OK);
        QCOMPARE(QDir::cleanPath(QDir::fromNativeSeparators(QString::fromWCharArray(icon.value))),
                 QFileInfo(QString::fromUtf8(NATIVE_SHELL_LIBRARY)).absoluteFilePath());
        HICON largeIcon = nullptr, smallIcon = nullptr;
        QCOMPARE(ExtractIconExW(icon.value, 0, &largeIcon, &smallIcon, 1), UINT(2));
        QVERIFY(largeIcon && smallIcon);
        DestroyIcon(largeIcon);
        DestroyIcon(smallIcon);
        CMyComPtr<IEnumExplorerCommand> enumeration;
        QCOMPARE(root->EnumSubCommands(&enumeration), S_OK);
        std::array<IExplorerCommand *, 128> children{};
        ULONG fetched = 0;
        QCOMPARE(enumeration->Next(ULONG(children.size()), children.data(), &fetched), S_FALSE);
        QVERIFY(fetched > 1 && fetched < children.size());
        CObjectVector<CMyComPtr<IExplorerCommand>> retained;
        for (ULONG i = 0; i < fetched; ++i)
            retained.AddNew().Attach(children[i]);
        for (size_t i = fetched; i < children.size(); ++i)
            QVERIFY(!children[i]);
        QCOMPARE(enumeration->Reset(), S_OK);
        CMyComPtr<IExplorerCommand> first;
        QCOMPARE(enumeration->Next(1, &first, &fetched), S_OK);
        QCOMPARE(fetched, ULONG(1));
        TaskString firstTitle, retainedTitle;
        QCOMPARE(first->GetTitle(items, &firstTitle.value), S_OK);
        QCOMPARE(retained[0]->GetTitle(items, &retainedTitle.value), S_OK);
        QCOMPARE(QString::fromWCharArray(firstTitle.value),
                 QString::fromWCharArray(retainedTitle.value));
        first.Release();
        CMyComPtr<IEnumExplorerCommand> clone;
        QCOMPARE(enumeration->Clone(&clone), E_NOTIMPL);
        QVERIFY(!clone);
        QCOMPARE(enumeration->Skip(1), E_NOTIMPL);
        factory.Release();
        root.Release();
        enumeration.Release();
        QCOMPARE(canUnload(), S_FALSE);
        bool nested = false;
        FOR_VECTOR(index, retained) {
            const auto &child = retained[index];
            TaskString name;
            QCOMPARE(child->GetTitle(nullptr, &name.value), S_OK);
            QVERIFY(name.value && *name.value);
            EXPCMDSTATE state{};
            QCOMPARE(child->GetState(items, FALSE, &state), S_OK);
            QCOMPARE(state, ECS_ENABLED);
            QCOMPARE(child->GetFlags(&flags), S_OK);
            if (flags == ECF_HASSUBCOMMANDS) {
                CMyComPtr<IEnumExplorerCommand> submenu;
                QCOMPARE(child->EnumSubCommands(&submenu), S_OK);
                CMyComPtr<IExplorerCommand> command;
                QCOMPARE(submenu->Next(1, &command, nullptr), S_OK);
                TaskString subTitle;
                QCOMPARE(command->GetTitle(nullptr, &subTitle.value), S_OK);
                QVERIFY(subTitle.value && *subTitle.value);
                nested = true;
            }
        }
        QVERIFY(nested);
        retained.Clear();
        QCOMPARE(canUnload(), S_OK);
        QVERIFY(library.unload());
        QVERIFY(file.open(QIODevice::ReadOnly));
        QCOMPARE(file.readAll(), QByteArray("fixture"));
    }
};

QTEST_GUILESS_MAIN(NativeShellComTests)
#include "native_shell_com_tests.moc"
