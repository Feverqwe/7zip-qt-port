#pragma once
#include "Common/MyString.h"
#include "Common/MyTypes.h"
#include <QString>
class CAgentFolder;
class IFolderFolder;
class QWidget;

HRESULT ReOpenArchiveFolder(CAgentFolder &folder, const QString &settingsPath, QWidget *parent,
                            IFolderFolder **resultFolder, bool usePassword,
                            const UString &password);
