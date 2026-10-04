#include "7zip/UI/FileManager/resource.h"
#include "CPP/7zip/UI/FileManager/ProgressDialog2.h"
#include "Common/StringConvert.h"
#include "archiveprogresstransport.h"
#include "mainwindow.h"
#include "qtcallbackprogresstransport.h"
#include "qtlangcontrols.h"

void MainWindow::showSelectedProperties() {
    if (!properties_->isEnabled())
        return;
    CPanel &panel = app_.GetFocusedPanel();
    CSelectedState selection;
    // Synchronize the Qt selection until the full source OnNotify port is connected.
    panel.SaveSelectedState(selection);
    CRecordVector<UInt32> indices;
    panel.Get_ItemIndices_Operated(indices);
    panel.LoadFullPath();
    PropertiesDialog dialog(panel, indices, this);
    dialog.exec();
}

void MainWindow::begin(const QString &label, const QString &detail) {
    operationName_ = label;
    currentDetail_ = detail;
    startProgressDialog(label, detail);
    progressTransport_ = std::make_unique<ArchiveProgressTransport>(*progress_, *backend_);
    progress_->OnInit();
    progress_->show();
}

void MainWindow::beginWorkerProgress(const QString &label, const QString &detail,
                                     std::function<void()> cancel) {
    startProgressDialog(label, detail);
    progress_->CompressingMode = false;
    progress_->ShowCompressionInfo = false;
    progressTransport_ = std::make_unique<QtCallbackProgressTransport>(std::move(cancel));
    progress_->AttachTransport(progressTransport_.get());
    progress_->OnInit();
    progress_->show();
}

void MainWindow::startProgressDialog(const QString &label, const QString &detail) {
    closeProgressDialog();
    auto *dialog = new CProgressDialog(this, ToUString(label));
    dialog->MainWindow = this;
    dialog->setWindowModality(Qt::ApplicationModal);
    dialog->Sync.Set_Status(ToUString(label));
    dialog->Sync.Set_FilePath(detail.toStdWString().c_str());
    progress_ = dialog;
}
