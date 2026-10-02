#include "Windows/Synchronization.h"
#include "Windows/Thread.h"
#include "processpriority.h"
#include <QCoreApplication>
#include <QFile>
#include <QProcess>
#include <QTemporaryDir>
#include <QtTest>
#include <cstdio>
#ifdef __APPLE__
#include <cerrno>
#include <libproc.h>
#include <sys/proc_info.h>
#include <sys/resource.h>
#include <unistd.h>
#elif defined(__linux__)
#include <cerrno>
#include <sched.h>
#endif

namespace {
HRESULT ObserveNative(bool &background) {
#ifdef _WIN32
    const DWORD priority = ::GetPriorityClass(::GetCurrentProcess());
    if (priority == 0)
        return HRESULT_FROM_WIN32(::GetLastError());
    background = priority == IDLE_PRIORITY_CLASS;
#elif defined(__APPLE__)
    errno = 0;
    proc_bsdinfo info{};
    const int size = proc_pidinfo(getpid(), PROC_PIDTBSDINFO, 0, &info, sizeof(info));
    if (size != sizeof(info))
        return HRESULT_FROM_WIN32(errno != 0 ? errno : EIO);
    background = (info.pbi_flags & 0x18000) != 0;
#else
    const int policy = sched_getscheduler(0);
    if (policy < 0)
        return HRESULT_FROM_WIN32(errno);
    background = (policy & ~SCHED_RESET_ON_FORK) == SCHED_BATCH;
#endif
    return S_OK;
}

struct CWorkerObservation {
    NWindows::NSynchronization::CAutoResetEvent Request;
    NWindows::NSynchronization::CManualResetEvent Done;
    NWindows::NSynchronization::CCriticalSection Lock;
    bool Stop = false;
    bool Background = false;
    HRESULT Result = E_FAIL;
};
static THREAD_FUNC_DECL ObserveWorker(void *parameter) {
    auto &data = *static_cast<CWorkerObservation *>(parameter);
    for (;;) {
        if (data.Request.Lock() != S_OK)
            return 0;
        {
            NWindows::NSynchronization::CCriticalSectionLock lock(data.Lock);
            if (data.Stop)
                return 0;
            data.Result = ObserveNative(data.Background);
        }
        data.Done.Set();
    }
}

int Child() {
    CWorkerObservation data;
    if (data.Request.Create() != S_OK || data.Done.Create() != S_OK)
        return 2;
    NWindows::CThread worker;
    if (worker.Create(ObserveWorker, &data) != S_OK)
        return 3;
    char input[32];
    while (std::fgets(input, sizeof(input), stdin) && input[0] != 'x') {
        bool main = false;
        HRESULT result = ObserveNative(main);
        data.Done.Reset();
        data.Request.Set();
        if (data.Done.Lock() != S_OK)
            break;
        {
            NWindows::NSynchronization::CCriticalSectionLock lock(data.Lock);
            if (result == S_OK)
                result = data.Result;
            std::printf("%u %d %d\n", unsigned(result), int(main), int(data.Background));
            std::fflush(stdout);
        }
    }
    {
        NWindows::NSynchronization::CCriticalSectionLock lock(data.Lock);
        data.Stop = true;
    }
    data.Request.Set();
    return worker.Wait_Close() == S_OK ? 0 : 4;
}

class CChildProcess {
  public:
    QProcess Process;
    ~CChildProcess() {
        if (Process.state() != QProcess::NotRunning) {
            Process.write("x\n");
            Process.waitForBytesWritten(1000);
            if (!Process.waitForFinished(1000)) {
                Process.kill();
                Process.waitForFinished(1000);
            }
        }
    }
    QByteArray Query() {
        Process.write("q\n");
        if (!Process.waitForBytesWritten(1000))
            return {};
        for (unsigned i = 0; !Process.canReadLine() && i < 3; ++i)
            if (!Process.waitForReadyRead(1000))
                return {};
        return Process.readLine().replace("\r\n", "\n");
    }
};

class CRestoreCurrent {
  public:
    bool Original = false;
    bool Valid = false;
    CRestoreCurrent() { Valid = NProcessPriority::GetBackground(0, Original) == S_OK; }
    ~CRestoreCurrent() {
        if (Valid)
            NProcessPriority::SetBackground(0, Original);
    }
};
} // namespace

class ProcessPriorityTests : public QObject {
    Q_OBJECT

  private slots:
    void currentProcessReversibleAndIdempotent() {
        CRestoreCurrent restore;
        QVERIFY(restore.Valid);
        for (const bool background : {true, true, false, false}) {
            QCOMPARE(NProcessPriority::SetBackground(0, background), S_OK);
            bool actual = !background;
            QCOMPARE(NProcessPriority::GetBackground(0, actual), S_OK);
            QCOMPARE(actual, background);
            bool native = !background;
            QCOMPARE(ObserveNative(native), S_OK);
            QCOMPARE(native, background);
        }
    }
    void invalidIDFailsAndPreservesOutput() {
        bool before = false;
        QCOMPARE(NProcessPriority::GetBackground(0, before), S_OK);
        bool output = true;
        QVERIFY(NProcessPriority::GetBackground(~UInt32(0), output) != S_OK);
        QVERIFY(output);
        QVERIFY(NProcessPriority::SetBackground(~UInt32(0), true) != S_OK);
        bool after = !before;
        QCOMPARE(NProcessPriority::GetBackground(0, after), S_OK);
        QCOMPARE(after, before);
    }
    void ownedChildAndExistingWorkerBothChangeAndRestore() {
        QTemporaryDir temporary;
        QVERIFY(temporary.isValid());
        const QString executable = temporary.filePath(QStringLiteral("процесс с пробелом") +
#ifdef _WIN32
                                                      ".exe"
#else
                                                      ""
#endif
        );
        QVERIFY(QFile::copy(QCoreApplication::applicationFilePath(), executable));
        CChildProcess child;
        child.Process.start(executable, {"--priority-child"});
        QVERIFY(child.Process.waitForStarted(3000));
        const auto pid = UInt32(child.Process.processId());
        QVERIFY(pid != 0);
        QCOMPARE(child.Query(), QByteArray("0 0 0\n"));
        for (const bool background : {true, false, true, false}) {
            QCOMPARE(NProcessPriority::SetBackground(pid, background), S_OK);
            bool actual = !background;
            QCOMPARE(NProcessPriority::GetBackground(pid, actual), S_OK);
            QCOMPARE(actual, background);
            QCOMPARE(child.Query(), background ? QByteArray("0 1 1\n") : QByteArray("0 0 0\n"));
        }
        child.Process.write("x\n");
        QVERIFY(child.Process.waitForFinished(3000));
        QCOMPARE(child.Process.exitCode(), 0);
    }
};

int main(int argc, char **argv) {
    QCoreApplication app(argc, argv);
    if (app.arguments().contains("--priority-child"))
        return Child();
    ProcessPriorityTests tests;
    return QTest::qExec(&tests, argc, argv);
}
#include "process_priority_tests.moc"
