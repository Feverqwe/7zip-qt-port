#include "7zip/UI/FileManager/resource.h"
#include "7zip/UI/FileManager/resourceGui.h"
#include "CPP/7zip/UI/FileManager/Panel.h"
#include "CPP/7zip/UI/FileManager/ProgressDialog2.h"
#include "mainwindow.h"
#include "qtapppresentation.h"
#include "qtarchivetransferbackup.h"
#include "qtlangcontrols.h"
#include "qttrashservice.h"
#include <QScopedValueRollback>

bool MainWindow::transferBetweenArchivePanels(const QString &destination, bool move,
                                              const CRecordVector<UInt32> &indices) {
    if (app_.NumPanels != 2 || state_ != State::Viewing)
        return false;
    const unsigned sourceIndex = app_.GetFocusedPanelIndex();
    const unsigned destinationIndex = 1 - sourceIndex;
    CPanel &source = app_.Panels[sourceIndex];
    CPanel &target = app_.Panels[destinationIndex];
    if (panelUi_[destinationIndex].state != State::Viewing ||
        QDir::cleanPath(destination) != QDir::cleanPath(ToQString(target.GetFsPath())))
        return false;
    if (source.GetParentFolders().Size() != 1 || target.GetParentFolders().Size() != 1 ||
        QDir::cleanPath(archive_) == QDir::cleanPath(panelUi_[destinationIndex].archive) ||
        target.IsThereReadOnlyFolder()) {
        QMessageBox::warning(this, QtLangText(IDS_COPYING),
                             QtLocalText("Transfer between these archives is unavailable.",
                                         "Перенос между этими архивами недоступен."));
        return true;
    }
    if (indices.IsEmpty())
        return true;
    UStringVector messages;
    HRESULT result;
    std::unique_ptr<QtArchiveTransferBackup> rollback;
    const QString destinationArchive = panelUi_[destinationIndex].archive;
    if (move)
        rollback = backend_->saveArchiveTransferDestination(destinationArchive);
    if (move && !rollback) {
        QMessageBox::warning(this, QtLangText(IDS_MOVING),
                             QtLocalText("Could not save destination archive for rollback.",
                                         "Не удалось сохранить целевой архив для отката."));
        return true;
    }
    {
        QScopedValueRollback<bool> progressOwner(sourceArchiveProgress_, true);
        result = app_.CopyArchiveToArchive(sourceIndex, indices, &messages);
    }
    CSelectedState selected;
    if (result == S_OK && messages.IsEmpty() && move) {
        result = source.DeleteItems(false, *trashService_, selected, &indices);
        if (result == S_OK) {
            app_.QtOperations().End();
            renderBoundFolder();
            source.RestoreLoadedSelection(selected);
        }
        if (result != S_OK) {
            const bool restored = rollback->restore();
            app_.QtOperations().End();
            activatePanelUi(destinationIndex);
            refreshCurrent();
            appPresentation_->CopyDestinationRefreshed(destinationIndex);
            activatePanelUi(sourceIndex);
            if (!restored) {
                QMessageBox::warning(this, QtLangText(IDS_MOVING),
                                     QtLocalText("Could not roll back destination archive. "
                                                 "Check both archives.",
                                                 "Не удалось откатить целевой архив. "
                                                 "Проверьте оба архива."));
                return true;
            }
        }
    }
    // All native work (including Move rollback) is settled before publication.
    // Keep a scan already started by the source refresh above.
    if (app_.QtOperations().kind() == QtAppOperationState::Kind::Operation)
        app_.QtOperations().End();
    if (result == S_OK && messages.IsEmpty()) {
        activatePanelUi(destinationIndex);
        refreshCurrent();
        appPresentation_->CopyDestinationRefreshed(destinationIndex);
        activatePanelUi(sourceIndex);
        statusBar()->showMessage(
            move
                ? QtLocalText("Items moved between archives.", "Объекты перемещены между архивами.")
                : QtLocalText("Items copied into the archive.", "Объекты скопированы в архив."));
    } else if (result != E_ABORT) {
        QMessageBox::warning(this, QtLangText(IDS_COPYING),
                             QString::fromWCharArray(HResultToMessage(result).Ptr()));
    }
    return true;
}
