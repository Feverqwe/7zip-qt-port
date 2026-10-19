#include "deviceaccess.h"
#include "mainwindow.h"
#include "qtfilemanagercommands.h"
#include "qtlanguageservices.h"

namespace {
bool ConfirmJournalRead(QWidget *parent) {
    QMessageBox warning(
        QMessageBox::Warning,
        QCoreApplication::translate("qtdevicecommands", "Open without journal recovery?"),
        QCoreApplication::translate(
            "qtdevicecommands",
            "This filesystem has unfinished journal changes. You can copy files without "
            "replaying the journal, but some files may be missing, outdated or unreadable. "
            "Nothing will be written to the device."),
        QMessageBox::Cancel, parent);
    warning.setObjectName("deviceJournalRecoveryWarning");
    warning.setTextFormat(Qt::PlainText);
    auto *open = warning.addButton(
        QCoreApplication::translate("qtdevicecommands", "Open read-only"), QMessageBox::AcceptRole);
    open->setObjectName("deviceOpenWithoutRecoveryButton");
    warning.setDefaultButton(QMessageBox::Cancel);
    warning.setEscapeButton(QMessageBox::Cancel);
    warning.exec();
    return warning.clickedButton() == open;
}
} // namespace

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
        HRESULT result = panel.OpenAsArc(input, temporary, name, UString(L"Ext"), opened);
        if (result == kDeviceJournalRecoveryRequired) {
            if (!ConfirmJournalRead(&w))
                return;
            CMyComPtr<IInStream> withoutReplay;
            result = CreateDeviceInputIgnoringJournal(input, &withoutReplay);
            if (result == S_OK) {
                input = withoutReplay;
                result = panel.OpenAsArc(input, temporary, name, UString(L"Ext"), opened);
            }
        }
        if (result != S_OK && result != E_ABORT) {
            const QString error =
                !opened.ErrorMessage.IsEmpty() ? ToQString(opened.ErrorMessage)
                : result == E_NOTIMPL
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
