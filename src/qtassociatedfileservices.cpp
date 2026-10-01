#include "qtassociatedfileservices.h"
#include "CPP/7zip/UI/FileManager/ViewSettings.h"
#include <QDesktopServices>
#include <QMessageBox>
#include <QProcess>

namespace {
bool OpenAssociation(const QUrl &url) { return QDesktopServices::openUrl(url); }
} // namespace

QtAssociatedFileServices::QtAssociatedFileServices(std::function<bool(const QUrl &)> opener)
    : _opener(opener ? std::move(opener) : OpenAssociation) {}

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

HRESULT QtAssociatedFileServices::StartApplication(const UString &, const UString &path,
                                                   QWidget *window) {
    // The platform association API has no waitable process handle. The native
    // caller retains its temp through the no-handle exit-event branch.
    bool opened = false;
    try {
        opened = _opener(QUrl::fromLocalFile(ToQString(path)));
    } catch (...) {
        opened = false;
    }
    if (opened)
        return S_OK;
    QMessageBox box(QMessageBox::Warning, QStringLiteral("Открыть файл"),
                    QStringLiteral("Не удалось открыть файл: %1").arg(ToQString(path)),
                    QMessageBox::Ok, window);
    box.setObjectName("externalFileOpenError");
    box.setTextFormat(Qt::PlainText);
    box.exec();
    return E_FAIL;
}
