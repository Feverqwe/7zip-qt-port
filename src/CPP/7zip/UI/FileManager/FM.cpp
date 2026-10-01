// Adapted from 7-Zip CPP/7zip/UI/FileManager/FM.cpp (0766b73).
// Igor Pavlov, LGPL-2.1-or-later; see third_party/7zip/License.txt.
// Partial port: WM_CLOSE, WM_COMMAND/toolbar dispatch and IsLargePageSupported.
// Full startup/window placement/drop retain their mandatory plan owners.
#include "App.h"
#include "Common/MyWindows.h"
#include "MyLoadMenu.h"
#include "qtfilemanagercommandservices.h"
#include "qtfilemanagerwindowservices.h"

static void ExecuteCommand(unsigned commandID, CApp &g_App,
                           IQtFileManagerMenuCommandServices &services) {
    CPanel::CDisableTimerProcessing disableTimerProcessing1(g_App.Panels[0]);
    CPanel::CDisableTimerProcessing disableTimerProcessing2(g_App.Panels[1]);
    switch (commandID) {
    case kMenuCmdID_Toolbar_Add:
        services.AddToArchive();
        break;
    case kMenuCmdID_Toolbar_Extract:
        services.ExtractArchives();
        break;
    case kMenuCmdID_Toolbar_Test:
        services.TestArchives();
        break;
    }
}

bool WndProc(unsigned wmId, CApp &g_App, IQtFileManagerMenuCommandServices &services) {
    if (g_App.QtOperations().closed() || !services.CanExecuteCommand(wmId))
        return false;
    // Exit holds no native folder input. Its Qt close event must be allowed to
    // perform WM_CLOSE; every other synchronous command protects live callers.
    if (wmId == 8) // IDCLOSE
        return OnMenuCommand(wmId, services);
    QtAppOperationState::CallScope call(g_App.QtOperations());
    // Original WM_COMMAND body, after Qt has supplied the action ID instead of
    // LOWORD(wParam). Control-notification messages are handled by Qt signals.
    if (wmId >= kMenuCmdID_Toolbar_Start && wmId < kMenuCmdID_Toolbar_End) {
        ExecuteCommand(wmId, g_App, services);
        return true;
    }
    if (OnMenuCommand(wmId, services))
        return true;
    // Existing Qt-only dialogs/navigation entries have no original menu ID.
    // Their explicit registry entries share this owner and capability gate.
    return services.ExecuteExtensionCommand(wmId);
}

bool WndProc(QtFileManagerMessage message, CApp &app, IQtFileManagerWindowServices &services) {
    switch (message) {
    case QtFileManagerMessage::Close: {
        if (app.QtOperations().closed())
            return true;
        // A disabled Win32 modal parent cannot initiate this close workflow.
        // Qt programmatic/reentrant close must not release live caller inputs.
        if (app.QtOperations().callActive() ||
            app.QtOperations().kind() == QtAppOperationState::Kind::Open || !services.CanCloseNow())
            return false;
        services.Deactivate(); // WM_ACTIVATE / WA_INACTIVE
        services.ExitExternalWorkers(false);
        services.RevokeDropTarget(); // RevokeDragDrop / _dropTarget.Release
        // Qt asynchronous workers can outlive the original modal call stack.
        // Join them while panel folders/temporary inputs still exist.
        services.QuiesceOperations();
        if (services.WindowWasCreated())
            app.Save();
        app.ReleaseApp();
        if (services.WindowWasCreated())
            services.SaveWindowInfo();
        services.ExitExternalWorkers(true);
        return true; // Qt closeEvent supplies the subsequent window destruction.
    }
    }
    return false;
}

bool IsLargePageSupported() {
#ifdef _WIN32
#ifdef _WIN64
    return true;
#else
    DWORD v = GetVersion();
    // low byte is major version:
    // next byte is minor version:
    v = ((v & 0xff) << 8) | ((v >> 8) & 0xFF);
    return (v > 0x501);
#endif
#else
    // The source FM privilege/large-page command is Windows-only.
    return false;
#endif
}
