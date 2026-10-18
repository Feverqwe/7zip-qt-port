#include "qtassociatedfileservices.h"
#include "CPP/7zip/UI/FileManager/BrowseDialog2.h"
#include "CPP/7zip/UI/FileManager/ViewSettings.h"
#include "qtlanguageservices.h"
#include <QDesktopServices>
#include <QMessageBox>
#include <QProcess>
#ifdef Q_OS_WIN
#include "Windows/ProcessUtils.h"
#endif

namespace {
bool OpenAssociation(const QUrl &url) { return QDesktopServices::openUrl(url); }
} // namespace

QtAssociatedFileServices::QtAssociatedFileServices(
    std::function<bool(const QUrl &)> opener,
    std::function<bool(CPanel &, const QtArchiveUpdateBody &)> updateRunner,
    std::function<bool()> updateReady)
    : _opener(std::move(opener)), _updateRunner(std::move(updateRunner)),
      _updateReady(std::move(updateReady)) {}

bool QtAssociatedFileServices::CanHandleExternalUpdate() const {
    return !_updateReady || _updateReady();
}
bool QtAssociatedFileServices::RunArchiveUpdate(CPanel &panel, const QtArchiveUpdateBody &body) {
    return _updateRunner ? _updateRunner(panel, body)
                         : IQtExternalFileServices::RunArchiveUpdate(panel, body);
}

HRESULT QtAssociatedFileServices::CreateKnownProcess(const UString &program,
                                                     const UString &parameters,
                                                     const UStringVector &paths, QWidget *,
                                                     IQtProcessHandle *&process) {
    return _processServices.Create(program, parameters, paths, process);
}

HRESULT QtAssociatedFileServices::CreateProcess(const UString &program, const UString &parameters,
                                                const UStringVector &paths, QWidget *) {
    // Disk View/Edit/Diff do not wait in the source implementation. Archive
    // editing requires the separate known-process owner before it can use this.
    QProcess process;
    process.setProgram(ToQString(program));
    QStringList arguments = QProcess::splitCommand(ToQString(parameters));
    FOR_VECTOR(i, paths) arguments.append(ToQString(paths[i]));
#ifdef Q_OS_WIN
    // Preserve original CreateProcess parameter-string interpretation on Windows.
    UString nativeParameters = parameters;
    FOR_VECTOR(i, paths) {
        if (!nativeParameters.IsEmpty() && nativeParameters.Back() != ' ')
            nativeParameters.Add_Space();
        nativeParameters.Add_Char('"');
        nativeParameters += paths[i];
        nativeParameters.Add_Char('"');
    }
    process.setNativeArguments(ToQString(nativeParameters));
#else
    process.setArguments(arguments);
#endif
    return process.startDetached() ? S_OK : E_FAIL;
}

HRESULT QtAssociatedFileServices::StartApplicationKnown(const UString &dir, const UString &path,
                                                        QWidget *window,
                                                        IQtProcessHandle *&process) {
    process = nullptr;
#ifdef Q_OS_WIN
    if (!_opener) {
        return _processServices.StartNative(
            [&](NWindows::CProcess &native) {
                return ::StartApplication(dir, path, window, native);
            },
            process);
    }
#endif
    return StartApplication(dir, path, window);
}

HRESULT QtAssociatedFileServices::StartApplication(const UString &dir, const UString &path,
                                                   QWidget *window) {
#ifdef Q_OS_WIN
    if (!_opener) {
        NWindows::CProcess native;
        return ::StartApplication(dir, path, window, native);
    }
#else
    (void)dir;
#endif
    // POSIX associations and explicitly injected openers have no waitable handle.
    bool opened = false;
    try {
        opened = (_opener ? _opener : OpenAssociation)(QUrl::fromLocalFile(ToQString(path)));
    } catch (...) {
        opened = false;
    }
    if (opened)
        return S_OK;
    QMessageBox box(
        QMessageBox::Warning, QtLocalText("Open file", "Открыть файл"),
        QtLocalText("Could not open file: %1", "Не удалось открыть файл: %1").arg(ToQString(path)),
        QMessageBox::Ok, window);
    box.setObjectName("externalFileOpenError");
    box.setTextFormat(Qt::PlainText);
    box.exec();
    return E_FAIL;
}
