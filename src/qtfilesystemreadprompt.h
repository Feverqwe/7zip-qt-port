#pragma once
#include "Common/MyWindows.h"
#include <QString>
class QWidget;
class COpenArchiveCallback;
struct CArchiveLink;
class CCodecs;
class CProgressDialog;
struct IOpenCallbackUI;
struct IQtArchiveReadConsent {
    virtual ~IQtArchiveReadConsent() = default;
    virtual CProgressDialog *ReadConsentDialog() const = 0;
    virtual IOpenCallbackUI *ReadConsentCallback() const = 0;
};
bool ConfirmLimitedFilesystemRead(QWidget *parent, const QString &limitations);
HRESULT PrepareLimitedFilesystemRead(const CArchiveLink &link, COpenArchiveCallback *callback,
                                     const QString &accepted, QString &limitations);
HRESULT PrepareLimitedArchiveRead(const CArchiveLink &link, const CCodecs &codecs,
                                  CProgressDialog *dialog, IOpenCallbackUI *callback,
                                  const QString &accepted, QString &limitations,
                                  bool &archiveWarning);
