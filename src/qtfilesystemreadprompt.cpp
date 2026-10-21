#include "qtfilesystemreadprompt.h"
#include "CPP/7zip/UI/Common/OpenArchive.h"
#include "CPP/7zip/UI/FileManager/OpenCallback.h"
#include "Common/ComTry.h"
#include "Windows/PropVariant.h"
#include "qtcallbackdialogs.h"
#include "qtdiagnostictext.h"
#include <QCoreApplication>
#include <QMessageBox>
#include <QPushButton>

bool ConfirmLimitedFilesystemRead(QWidget *parent, const QString &limitations) {
    QMessageBox warning(
        QMessageBox::Warning,
        QCoreApplication::translate("qtdevicecommands", "Open read-only with limitations?"),
        QCoreApplication::translate(
            "qtdevicecommands",
            "You can copy readable files, but unsupported features or unfinished recovery may "
            "leave some files missing, outdated or unreadable. Nothing will be written to the "
            "source. Sevenzip will not replay the journal, clean up orphaned files or repair "
            "the filesystem."),
        QMessageBox::Cancel, parent);
    warning.setObjectName("filesystemReadLimitationsWarning");
    warning.setTextFormat(Qt::PlainText);
    QStringList translated;
    for (const QString &line : limitations.split('\n'))
        translated.append(
            QString::fromWCharArray(QtArchiveDiagnosticText(UString(line.toStdWString().c_str()))));
    warning.setInformativeText(translated.join('\n'));
    auto *open = warning.addButton(
        QCoreApplication::translate("qtdevicecommands", "Open read-only"), QMessageBox::AcceptRole);
    open->setObjectName("deviceOpenWithoutRecoveryButton");
    warning.setDefaultButton(QMessageBox::Cancel);
    warning.setEscapeButton(QMessageBox::Cancel);
    warning.exec();
    return warning.clickedButton() == open;
}

HRESULT PrepareLimitedFilesystemRead(const CArchiveLink &link, COpenArchiveCallback *callback,
                                     const QString &accepted, QString &limitations) {
    limitations.clear();
    FOR_VECTOR(a, link.Arcs) {
        const CArc &arc = link.Arcs[a];
        if (!arc.IsReadOnly)
            continue;
        UInt32 count = 0;
        RINOK(arc.Archive->GetNumberOfArchiveProperties(&count))
        for (UInt32 i = 0; i < count; ++i) {
            CMyComBSTR name;
            PROPID id;
            VARTYPE type;
            RINOK(arc.Archive->GetArchivePropertyInfo(i, &name, &id, &type))
            if (!name || QString::fromWCharArray(name) != QStringLiteral("Read-only limitations"))
                continue;
            NWindows::NCOM::CPropVariant value;
            RINOK(arc.Archive->GetArchiveProperty(id, &value))
            if (value.vt == VT_EMPTY)
                continue;
            if (!arc.IsReadOnly || value.vt != VT_BSTR)
                return E_NOTIMPL;
            if (!limitations.isEmpty())
                limitations += '\n';
            limitations += QString::fromWCharArray(value.bstrVal);
        }
    }
    if (!callback || limitations.isEmpty() || limitations == accepted)
        return S_OK;
    RINOK(callback->Open_CheckBreak())
    RINOK(RunCallbackDialog(callback->ProgressDialog, [&] {
        return ConfirmLimitedFilesystemRead(&callback->ProgressDialog, limitations) ? S_OK
                                                                                    : E_ABORT;
    }))
    return callback->Open_CheckBreak();
}
