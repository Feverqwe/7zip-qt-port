#include "Windows/ErrorMsg.h"
#include "Windows/Synchronization.h"
#include "Windows/Thread.h"
#include <QtTest>
#include <cerrno>

namespace NSync = NWindows::NSynchronization;

struct NativeWorker {
    NSync::CManualResetEvent Started;
    NSync::CCriticalSection Lock;
    unsigned Counter = 0;

    static THREAD_FUNC_DECL run(void *context) {
        auto &worker = *static_cast<NativeWorker *>(context);
        if (worker.Started.Lock() == 0)
            for (unsigned i = 0; i < 10000; ++i) {
                NSync::CCriticalSectionLock lock(worker.Lock);
                ++worker.Counter;
            }
        return THREAD_FUNC_RET_ZERO;
    }
};

class NativeServicesTests : public QObject {
    Q_OBJECT

    static QString error(DWORD code) {
        const UString text = NWindows::NError::MyFormatMessage(code);
        return QString::fromWCharArray(text);
    }

  private slots:
    void nativeEventCriticalSectionAndJoinedThreadReuse() {
        NativeWorker worker;
        QCOMPARE(worker.Started.Create(), WRes(0));
        QVERIFY(worker.Started.IsCreated());
        NWindows::CThread first;
        NWindows::CThread second;
        QVERIFY(!first.IsCreated() && !second.IsCreated());
        QCOMPARE(first.Create(NativeWorker::run, &worker), WRes(0));
        const WRes secondResult = second.Create(NativeWorker::run, &worker);
        // Release a successfully started worker even if the second creation failed.
        const WRes started = worker.Started.Set();
        const WRes firstJoined = first.Wait_Close();
        const WRes secondJoined = secondResult == 0 ? second.Wait_Close() : secondResult;
        QCOMPARE(secondResult, WRes(0));
        QCOMPARE(started, WRes(0));
        QCOMPARE(firstJoined, WRes(0));
        QCOMPARE(secondJoined, WRes(0));
        QVERIFY(!first.IsCreated() && !second.IsCreated());
        QCOMPARE(worker.Counter, 20000u);
        QCOMPARE(first.Create(NativeWorker::run, &worker), WRes(0));
        QCOMPARE(first.Wait_Close(), WRes(0));
        QCOMPARE(worker.Counter, 30000u);
        QCOMPARE(first.Close(), WRes(0));
        QCOMPARE(first.Close(), WRes(0));
        QCOMPARE(worker.Started.Reset(), WRes(0));
        QCOMPARE(worker.Started.Set(), WRes(0));
        QCOMPARE(worker.Started.Lock(), WRes(0));
        QCOMPARE(worker.Started.Close(), WRes(0));
        QVERIFY(!worker.Started.IsCreated());
        QCOMPARE(worker.Started.Close(), WRes(0));
        QCOMPARE(worker.Started.CreateIfNotCreated_Reset(), WRes(0));
        QCOMPARE(worker.Started.Set(), WRes(0));
        QCOMPARE(worker.Started.Lock(), WRes(0));
    }

    void nativeAutoResetEventAndSemaphore() {
        NSync::CAutoResetEvent event;
        QCOMPARE(event.Create(), WRes(0));
        QCOMPARE(event.Set(), WRes(0));
        QCOMPARE(event.Lock(), WRes(0));
        QCOMPARE(event.CreateIfNotCreated_Reset(), WRes(0));
        QCOMPARE(event.Set(), WRes(0));
        QCOMPARE(event.Lock(), WRes(0));
        NSync::CSemaphore semaphore;
        QCOMPARE(semaphore.Create(1, 2), WRes(0));
        QCOMPARE(semaphore.Lock(), WRes(0));
        QCOMPARE(semaphore.Release(1), WRes(0));
        QCOMPARE(semaphore.Lock(), WRes(0));
    }

    void originalErrorFormattingPreservesNativeAndUnknownCodes() {
        QCOMPARE(error(DWORD(0xe1234567)), QString("Error #E1234567"));
        QCOMPARE(error(DWORD(MY_HRES_ERROR_INTERNAL_ERROR)),
                 QString("Internal Error: The failure in hardware (RAM or CPU), OS or program"));
        const UString native = NWindows::NError::MyFormatMessage(HRESULT(E_ABORT));
        const QString message = QString::fromWCharArray(native);
        QVERIFY(!message.isEmpty());
        QVERIFY(!message.endsWith("\r\n"));
#ifdef _WIN32
        const DWORD code = DWORD(HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER));
#else
        const DWORD code = DWORD(HRESULT_FROM_WIN32(EINVAL));
#endif
        const QString systemError = error(code);
        QVERIFY(!systemError.isEmpty());
        QVERIFY(!systemError.endsWith("\r\n"));
#ifndef _WIN32
        QVERIFY(systemError.startsWith(QString("errno=%1 : ").arg(EINVAL)));
        QCOMPARE(message, QString("E_ABORT : Operation aborted"));
#endif
    }
};

QTEST_GUILESS_MAIN(NativeServicesTests)
#include "native_services_tests.moc"
