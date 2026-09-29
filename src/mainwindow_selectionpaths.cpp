#include "7zip/PropID.h"
#include "Windows/PropVariant.h"
#include "mainwindow.h"

CRecordVector<UInt32> MainWindow::sourceOperatedIndices() {
    CRecordVector<UInt32> indices;
    CPanel &panel = app_.GetFocusedPanel();
    if (!panel.GetFolder())
        return indices;
    CSelectedState selection;
    // Qt selection synchronization remains until the full source OnNotify port.
    panel.SaveSelectedState(selection);
    panel.Get_ItemIndices_Operated(indices);
    return indices;
}

QStringList MainWindow::selectedArchiveItemsPaths() {
    QStringList paths;
    if (state_ != State::Viewing)
        return paths;
    const auto indices = sourceOperatedIndices();
    auto *folder = static_cast<CAgentFolder *>(app_.GetFocusedPanel().GetFolder());
    FOR_VECTOR(i, indices)
    paths << ToQString(folder->GetFullPrefix(indices[i]) + folder->GetName(indices[i]));
    return paths;
}

QStringList MainWindow::selectedArchivePaths() {
    QStringList paths;
    if (state_ != State::Viewing)
        return paths;
    CPanel &panel = app_.GetFocusedPanel();
    if (!panel.GetFolder())
        return paths;
    const auto indices = sourceOperatedIndices();
    auto *folder = static_cast<CAgentFolder *>(panel.GetFolder());
    CUIntVector realIndices;
    // Preserve the 7zz selected-directory transport policy until source Extract
    // integration (P0-12); the original engine Extract passes false here.
    folder->GetRealIndices(indices.ConstData(), indices.Size(), false, true, realIndices);
    IFolderFolder *properties = folder->GetArchiveProperties();
    if (!properties)
        return paths;
    FOR_VECTOR(i, realIndices) {
        NWindows::NCOM::CPropVariant path;
        if (properties->GetProperty(realIndices[i], kpidPath, &path) != S_OK || path.vt != VT_BSTR)
            return {};
        paths << ToQString(UString(path.bstrVal));
    }
    paths.removeDuplicates();
    return paths;
}
