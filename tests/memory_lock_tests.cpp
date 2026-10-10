#include "CPP/7zip/UI/Common/Bench.h"
#include "CPP/7zip/UI/Common/CompressCall.h"
#include "CPP/7zip/UI/FileManager/RegistryUtils.h"
#include "CPP/7zip/UI/FileManager/SettingsPage.h"
#include "qtmemorylockservices.h"
#include <QCheckBox>
#include <QScopeGuard>
#include <QTemporaryDir>
#include <QtTest>

extern bool g_LargePagesMode;

class MemoryLockRecorder final : public IQtMemoryLockServices {
  public:
    bool supported = true;
    bool enabledResult = true;
    unsigned risk = 0;
    mutable QStringList calls;
    bool IsSupported() const override { return supported; }
    bool AddLockMemoryPrivilege() const override {
        calls << "add";
        return false;
    }
    unsigned Get_LargePages_RiskLevel() const override {
        calls << "risk";
        return risk;
    }
    bool EnablePrivilege_LockMemory(bool enable) const override {
        calls << (enable ? "enable" : "disable");
        return enabledResult;
    }
};

class MemoryLockTests : public QObject {
    Q_OBJECT
  private slots:
    void startupPolicy_data() {
        QTest::addColumn<bool>("supported");
        QTest::addColumn<bool>("saved");
        QTest::addColumn<unsigned>("risk");
        QTest::addColumn<bool>("enabledResult");
        QTest::addColumn<QStringList>("calls");
        QTest::newRow("unsupported") << false << true << 0u << true << QStringList{};
        QTest::newRow("disabled") << true << false << 0u << true << QStringList{"add"};
        QTest::newRow("risk-1") << true << true << 1u << true << QStringList{"add", "risk"};
        QTest::newRow("risk-2") << true << true << 2u << true << QStringList{"add", "risk"};
        QTest::newRow("privilege-refused")
            << true << true << 0u << false << QStringList{"add", "risk", "enable"};
        QTest::newRow("enabled-despite-add-failure")
            << true << true << 0u << true << QStringList{"add", "risk", "enable"};
    }
    void startupPolicy() {
        QFETCH(bool, supported);
        QFETCH(bool, saved);
        QFETCH(unsigned, risk);
        QFETCH(bool, enabledResult);
        QFETCH(QStringList, calls);
        QTemporaryDir fixture;
        const QString path = fixture.filePath(QStringLiteral("настройки 空 memory.ini"));
        SaveLockMemoryEnable(saved, path);
        MemoryLockRecorder services;
        services.supported = supported;
        services.risk = risk;
        services.enabledResult = enabledResult;
        const bool originalMode = g_LargePagesMode;
        const auto restore = qScopeGuard([&] { g_LargePagesMode = originalMode; });
        for (bool previous : {false, true}) {
            g_LargePagesMode = previous;
            services.calls.clear();
            NQtFileManager::SetMemoryLock(path, services);
            QCOMPARE(services.calls, calls);
            QCOMPARE(g_LargePagesMode, supported && saved && risk == 0 ? enabledResult : previous);
            QCOMPARE(ReadLockMemoryEnable(path), saved);
#ifdef Q_OS_WIN
            AString diagnostic;
            Add_LargePages_String(diagnostic);
            QCOMPARE(diagnostic.Find("(LP-") >= 0, g_LargePagesMode);
#endif
        }
    }
    void engineSwitchUsesSourceRiskPolicy() {
        QTemporaryDir fixture;
        const QString path = fixture.filePath("memory.ini");
        MemoryLockRecorder services;
        for (bool enable : {false, true}) {
            SaveLockMemoryEnable(enable, path);
            for (unsigned risk : {0u, 1u, 2u}) {
                services.risk = risk;
                services.calls.clear();
                UString params("existing");
                AddLagePagesSwitch(params, path, &services);
                QCOMPARE(QString::fromWCharArray(params),
                         enable && risk == 0 ? QString("existing -slp") : QString("existing"));
                QCOMPARE(services.calls, enable ? QStringList{"risk"} : QStringList{});
            }
        }
    }
    void settingsApplyUpdatesCurrentProcessAndPersists() {
        QTemporaryDir fixture;
        const QString path = fixture.filePath(QStringLiteral("настройки 空 memory.ini"));
        MemoryLockRecorder services;
        CSettingsPage page(path, nullptr, false, &services);
        auto *option = page.findChild<QCheckBox *>("largePagesOption");
        QVERIFY(option);
        QVERIFY(option->isEnabled());
        QCOMPARE(page.OnApply(), 0L);
        QVERIFY(services.calls.isEmpty());
        QVERIFY(!QFileInfo::exists(path));
        for (bool enable : {true, false, true}) {
            // Source saves the requested setting even if token adjustment fails.
            services.enabledResult = !enable;
            option->setChecked(enable);
            QVERIFY(page.IsModified());
            QCOMPARE(page.OnApply(), 0L);
            QCOMPARE(services.calls, QStringList{enable ? QString("enable") : QString("disable")});
            services.calls.clear();
            QCOMPARE(ReadLockMemoryEnable(path), enable);
            QVERIFY(!page.IsModified());
            QCOMPARE(page.OnApply(), 0L);
            QVERIFY(services.calls.isEmpty());
        }
        CSettingsPage cancelled(path, nullptr, false, &services);
        cancelled.findChild<QCheckBox *>("largePagesOption")->setChecked(false);
        cancelled.reject();
        QVERIFY(services.calls.isEmpty());
        QVERIFY(ReadLockMemoryEnable(path));
    }
    void unsupportedSettingsStayDisabled() {
        QTemporaryDir fixture;
        const QString path = fixture.filePath("memory.ini");
        MemoryLockRecorder services;
        services.supported = false;
        SaveLockMemoryEnable(true, path);
        CSettingsPage page(path, nullptr, false, &services);
        auto *option = page.findChild<QCheckBox *>("largePagesOption");
        QVERIFY(!option->isEnabled());
        option->setChecked(true);
        QCOMPARE(page.OnApply(), 0L);
        QVERIFY(services.calls.isEmpty());
        QVERIFY(ReadLockMemoryEnable(path));
    }
};
QTEST_MAIN(MemoryLockTests)
#include "memory_lock_tests.moc"
