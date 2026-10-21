#pragma once
#include "Common/MyWindows.h"
#include <QString>
class QWidget;
class COpenArchiveCallback;
struct CArchiveLink;
bool ConfirmLimitedFilesystemRead(QWidget *parent, const QString &limitations);
HRESULT PrepareLimitedFilesystemRead(const CArchiveLink &link, COpenArchiveCallback *callback,
                                     const QString &accepted, QString &limitations);
