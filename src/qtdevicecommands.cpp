#include "mainwindow.h"
#include "qtfilemanagercommands.h"
#include "qtlanguageservices.h"

void QtFileManagerCommands::OpenDevice() {
    auto &w = window_;
    DeviceSelectionDialog dialog(w.backend_->codecCatalog(), &w, w.deviceServices_);
    if (dialog.exec() != QDialog::Accepted)
        return;
    CMyComPtr<IInStream> input = dialog.selectedStream();
    const UString name = ToUString(dialog.selectedName());
    RunNativeOpen([&](CPanel &panel) {
        CPanel::CDisableTimerProcessing timer(panel);
        CPanel::CDisableNotify notify(panel);
        CTempFileInfo temporary;
        COpenResult opened;
        const HRESULT result = panel.OpenAsArc(input, temporary, name, UString(L"Ext"), opened);
        if (result != S_OK && result != E_ABORT) {
            const QString error =
                result == E_NOTIMPL
                    ? QCoreApplication::translate("qtdevicecommands",
                                                  "This filesystem uses features that are not "
                                                  "supported for direct reading.")
                    : QCoreApplication::translate(
                          "qtdevicecommands",
                          "Could not open this partition. It may be unsupported or the device may "
                          "have been disconnected.");
            QMessageBox::warning(&w, QCoreApplication::translate("qtdevicecommands", "Open device"),
                                 error);
        }
    });
}
