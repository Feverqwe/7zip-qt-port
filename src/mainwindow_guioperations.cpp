#include "mainwindow.h"
#include "qtoperationlauncher.h"
#include <QDir>
#include <QFileInfo>
#include <QTimer>

namespace {
QStringList ArchivePaths(const UStringVector &paths) {
    QStringList result;
    FOR_VECTOR(i, paths) { result.append(QString::fromWCharArray(paths[i])); }
    return result;
}
QString NormalPath(const QString &path) {
    const QFileInfo info(path);
    const QString canonical = info.canonicalFilePath();
    return canonical.isEmpty() ? info.absoluteFilePath() : canonical;
}
bool ContainsDirectory(const QString &parent, const QString &child) {
    const QString relative = QDir(parent).relativeFilePath(child);
    return QDir::isRelativePath(relative) && relative != ".." && !relative.startsWith("../");
}
} // namespace

HRESULT MainWindow::launchGuiArchiveTest(const UStringVector &paths, bool hashMode) {
    if (hashMode)
        return E_NOTIMPL;
    QString error;
    auto *operation = backend_->startGuiTest(ArchivePaths(paths), error);
    return observeGuiArchiveOperation(operation, error);
}

HRESULT MainWindow::launchGuiArchiveExtract(const UStringVector &paths, const UString &folder,
                                            bool showDialog, bool elimDup, UInt32 writeZone) {
    QString error;
    auto *operation =
        backend_->startGuiExtract(ArchivePaths(paths), QString::fromWCharArray(folder), showDialog,
                                  elimDup, writeZone, error);
    return observeGuiArchiveOperation(operation, error);
}

HRESULT MainWindow::observeGuiArchiveOperation(QtOperationLauncher *operation,
                                               const QString &error) {
    const unsigned source = app_.GetFocusedPanelIndex();
    if (!operation) {
        app_.Panels[source].MessageBox_Error(ToUString(error));
        return E_FAIL;
    }
    connect(operation, &QtOperationLauncher::finished, this,
            [this, source](const QtOperationResult &result) {
                if (app_.QtOperations().closed())
                    return;
                for (unsigned i = 0; i < kNumPanelsMax; ++i)
                    refreshGuiOutputPanel(i, result.ChangedDirectories);
                using Outcome = QtOperationResult::Outcome;
                if (result.Status == Outcome::LaunchFailed ||
                    result.Status == Outcome::ProtocolFailed || result.Status == Outcome::Crashed)
                    app_.Panels[source].MessageBox_Error(ToUString(result.Error));
                // Source dialogs in 7zG own ordinary errors and cancellation.
            });
    return S_OK;
}

void MainWindow::refreshGuiOutputPanel(unsigned index, const QStringList &directories) {
    if (app_.QtOperations().closed() || directories.isEmpty())
        return;
    if (app_.QtOperations().active() || app_.QtOperations().callActive()) {
        // Another source operation/scan owns the panels. Re-evaluate the current
        // provider after it releases its guards; never restore an old location.
        QTimer::singleShot(
            50, this, [this, index, directories] { refreshGuiOutputPanel(index, directories); });
        return;
    }
    auto &panel = app_.Panels[index];
    if (!panel.PanelCreated || !panel.Is_IO_FS_Folder() || panel.IsArcFolder())
        return;
    const QString current = NormalPath(QString::fromWCharArray(panel.GetFsPath()));
    for (const auto &directory : directories) {
        const QString output = NormalPath(directory);
        if (ContainsDirectory(current, output) || ContainsDirectory(output, current)) {
            panel.OnReload();
            return;
        }
    }
}
