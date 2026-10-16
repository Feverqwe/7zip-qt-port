#include "qtexternalprocess.h"
#include <QScopeGuard>
#include <QtTest>
#include <TlHelp32.h>
#include <array>

namespace {
struct ProcessRecord {
    DWORD id;
    DWORD parent;
    quint64 created;
    bool readable;
    const wchar_t *name;
};

struct SnapshotFixture {
    std::array<ProcessRecord, 2> records;
    unsigned position = 0;
    unsigned openHandles = 0;
    bool excessiveAccess = false;
};

// Win32 import callbacks have no context parameter. This pointer belongs only
// to this test executable and its scoped fixture; Qt DLL imports stay native.
thread_local SnapshotFixture *currentFixture = nullptr;

template <typename Function> Function native(const char *name) {
    return reinterpret_cast<Function>(GetProcAddress(GetModuleHandleW(L"kernel32.dll"), name));
}

HANDLE WINAPI snapshot(DWORD flags, DWORD process) {
    if (!currentFixture)
        return native<decltype(&CreateToolhelp32Snapshot)>("CreateToolhelp32Snapshot")(flags,
                                                                                       process);
    ++currentFixture->openHandles;
    currentFixture->position = 0;
    return currentFixture;
}

BOOL entry(PROCESSENTRY32W *value) {
    if (currentFixture->position >= currentFixture->records.size())
        return FALSE;
    const auto &record = currentFixture->records[currentFixture->position++];
    value->th32ProcessID = record.id;
    value->th32ParentProcessID = record.parent;
    wcscpy_s(value->szExeFile, record.name);
    return TRUE;
}

BOOL WINAPI first(HANDLE handle, LPPROCESSENTRY32W value) {
    if (!currentFixture || handle != currentFixture)
        return native<decltype(&Process32FirstW)>("Process32FirstW")(handle, value);
    currentFixture->position = 0;
    return entry(value);
}

BOOL WINAPI next(HANDLE handle, LPPROCESSENTRY32W value) {
    if (!currentFixture || handle != currentFixture)
        return native<decltype(&Process32NextW)>("Process32NextW")(handle, value);
    return entry(value);
}

HANDLE WINAPI open(DWORD access, BOOL inherit, DWORD id) {
    if (!currentFixture)
        return native<decltype(&OpenProcess)>("OpenProcess")(access, inherit, id);
    currentFixture->excessiveAccess |=
        (access & ~PROCESS_QUERY_LIMITED_INFORMATION) != 0 || inherit;
    for (auto &record : currentFixture->records)
        if (record.id == id) {
            ++currentFixture->openHandles;
            return &record;
        }
    SetLastError(ERROR_ACCESS_DENIED);
    return nullptr;
}

BOOL WINAPI times(HANDLE handle, LPFILETIME created, LPFILETIME exited, LPFILETIME kernel,
                  LPFILETIME user) {
    if (currentFixture)
        for (const auto &record : currentFixture->records)
            if (handle == &record) {
                if (!record.readable) {
                    SetLastError(ERROR_ACCESS_DENIED);
                    return FALSE;
                }
                created->dwLowDateTime = DWORD(record.created);
                created->dwHighDateTime = DWORD(record.created >> 32);
                return TRUE;
            }
    return native<decltype(&GetProcessTimes)>("GetProcessTimes")(handle, created, exited, kernel,
                                                                 user);
}

BOOL WINAPI close(HANDLE handle) {
    if (currentFixture) {
        bool owned = handle == currentFixture;
        for (const auto &record : currentFixture->records)
            owned |= handle == &record;
        if (owned) {
            --currentFixture->openHandles;
            return TRUE;
        }
    }
    return native<decltype(&CloseHandle)>("CloseHandle")(handle);
}
} // namespace

// Resolve only this executable's import pointers to deterministic SDK fixtures.
// This neither patches another process nor creates real OS processes/handles.
extern "C" {
decltype(&CreateToolhelp32Snapshot) __imp_CreateToolhelp32Snapshot = snapshot;
HANDLE WINAPI CreateToolhelp32Snapshot(DWORD flags, DWORD process) {
    return snapshot(flags, process);
}
BOOL WINAPI Process32FirstW(HANDLE handle, LPPROCESSENTRY32W value) { return first(handle, value); }
BOOL WINAPI Process32NextW(HANDLE handle, LPPROCESSENTRY32W value) { return next(handle, value); }
decltype(&OpenProcess) __imp_OpenProcess = open;
decltype(&GetProcessTimes) __imp_GetProcessTimes = times;
decltype(&CloseHandle) __imp_CloseHandle = close;
}

class ProcessSnapshotTests final : public QObject {
    Q_OBJECT
  private slots:
    void parentIdentity_data() {
        QTest::addColumn<quint64>("parentCreated");
        QTest::addColumn<quint64>("childCreated");
        QTest::addColumn<bool>("parentReadable");
        QTest::addColumn<bool>("childReadable");
        QTest::addColumn<quint32>("expectedParent");
        QTest::newRow("actual-child")
            << quint64(100) << quint64(200) << true << true << quint32(10);
        QTest::newRow("recycled-parent-id")
            << quint64(200) << quint64(100) << true << true << quint32(0);
        QTest::newRow("equal-clock-tick")
            << quint64(100) << quint64(100) << true << true << quint32(10);
        QTest::newRow("unavailable-parent-time")
            << quint64(200) << quint64(100) << false << true << quint32(10);
        QTest::newRow("unavailable-child-time")
            << quint64(200) << quint64(100) << true << false << quint32(10);
    }

    void parentIdentity() {
        QFETCH(quint64, parentCreated);
        QFETCH(quint64, childCreated);
        QFETCH(bool, parentReadable);
        QFETCH(bool, childReadable);
        QFETCH(quint32, expectedParent);
        SnapshotFixture fixture{{{{10, 0, parentCreated, parentReadable, L"parent.exe"},
                                  {20, 10, childCreated, childReadable, L"child.exe"}}}};
        const auto previous = currentFixture;
        currentFixture = &fixture;
        const auto restore = qScopeGuard([&] { currentFixture = previous; });
        QtNativeProcessServices services;
        CObjectVector<CSnapshotProcess> records;
        CRecordVector<UInt32> roots;
        roots.Add(10);
        services.GetSnapshot(records, roots);
        QCOMPARE(records.Size(), 2u);
        QCOMPARE(records[0].Id, UInt32(10));
        QCOMPARE(records[0].ParentId, UInt32(0));
        QCOMPARE(records[1].Id, UInt32(20));
        QCOMPARE(records[1].ParentId, UInt32(expectedParent));
        QCOMPARE(QString::fromWCharArray(records[1].Name), QStringLiteral("child.exe"));
        QCOMPARE(fixture.openHandles, 0u);
        QVERIFY(!fixture.excessiveAccess);
    }
};

QTEST_GUILESS_MAIN(ProcessSnapshotTests)
#include "process_snapshot_tests.moc"
