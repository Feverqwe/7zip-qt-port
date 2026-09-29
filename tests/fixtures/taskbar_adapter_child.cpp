#include "taskbar_adapter_child.h"
#include "Common/MyWindows.h"
#include "desktopidentity.h"
#include "qttaskbarprogress.h"
#include <QApplication>
#include <QSocketNotifier>
#include <QThread>
#include <QWidget>
#include <cstdio>
#include <unistd.h>

int RunTaskbarAdapterChild(int argc, char **argv) {
    const QByteArray address = qgetenv("SEVENZIP_PRIVATE_DBUS_ADDRESS");
    if (address.isEmpty() || qgetenv("DBUS_SESSION_BUS_ADDRESS") != address)
        return 3;
    const QByteArray mode(argv[1]);
    if (mode == "--no-gui") {
        QCoreApplication app(argc, argv);
        CQtTaskbarProgress progress;
        return progress.HrInit() == S_OK ? 1 : 0;
    }
    QApplication app(argc, argv);
    app.setDesktopFileName(mode == "--empty-id" ? QString()
                           : mode == "--invalid-id"
                               ? QStringLiteral("../evil")
                               : QString::fromLatin1(NDesktopIdentity::kFileName));
    QWidget window;
    auto progress = std::make_unique<CQtTaskbarProgress>();
    const HRESULT initialized = progress->HrInit();
    std::printf("%ld\n", long(initialized));
    std::fflush(stdout);
    if (mode != "--adapter-child")
        return initialized == S_OK ? 1 : 0;
    if (initialized != S_OK)
        return 2;

    QSocketNotifier input(STDIN_FILENO, QSocketNotifier::Read);
    QObject::connect(&input, &QSocketNotifier::activated, &app, [&] {
        char line[128];
        if (!std::fgets(line, sizeof(line), stdin)) {
            app.quit();
            return;
        }
        HRESULT result = E_INVALIDARG;
        unsigned state;
        unsigned long long completed, total;
        if (std::sscanf(line, "v %llu %llu", &completed, &total) == 2)
            result = progress
                         ? progress->SetProgressValue(&window, UInt64(completed), UInt64(total))
                         : E_FAIL;
        else if (std::sscanf(line, "s %u", &state) == 1)
            result = progress ? progress->SetProgressState(&window, TBPFLAG(state)) : E_FAIL;
        else if (line[0] == 'n')
            result = progress ? progress->SetProgressState(nullptr, TBPF_NORMAL) : E_FAIL;
        else if (line[0] == 't' && progress) {
            HRESULT initialize = S_OK, changed = S_OK, value = S_OK;
            QThread *worker = QThread::create([&] {
                initialize = progress->HrInit();
                changed = progress->SetProgressState(&window, TBPF_ERROR);
                value = progress->SetProgressValue(&window, 1, 2);
            });
            worker->start();
            worker->wait();
            delete worker;
            std::printf("%ld %ld %ld\n", long(initialize), long(changed), long(value));
            std::fflush(stdout);
            return;
        } else if (line[0] == 'd') {
            progress.reset();
            result = S_OK;
        } else if (line[0] == 'x') {
            app.quit();
            result = S_OK;
        }
        std::printf("%ld\n", long(result));
        std::fflush(stdout);
    });
    return app.exec();
}
