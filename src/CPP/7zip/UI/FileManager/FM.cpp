// Adapted from 7-Zip CPP/7zip/UI/FileManager/FM.cpp (0766b73).
// Igor Pavlov, LGPL-2.1-or-later; see third_party/7zip/License.txt.
// Partial port: WM_CLOSE lifecycle. Startup/command dispatch remain mandatory.
#include "App.h"
#include "qtfilemanagerwindowservices.h"

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
