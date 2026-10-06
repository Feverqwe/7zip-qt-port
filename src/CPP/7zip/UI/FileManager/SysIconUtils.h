// Adapted from 7-Zip CPP/7zip/UI/FileManager/SysIconUtils.h (0766b73).
// Igor Pavlov, LGPL-2.1-or-later; see third_party/7zip/License.txt.
// The source cache/index policy remains; Qt provides platform icons.
#pragma once

#include "Common/MyString.h"
#include "Common/MyVector.h"
#include "Common/MyWindows.h"
#include <QIcon>
#include <QMap>
#include <QString>
#include <QVector>

struct CExtIconPair {
    UString Ext;
    int IconIndex;
};

struct CAttribIconPair {
    DWORD Attrib;
    int IconIndex;
};

struct CExtToIconMap {
    CRecordVector<CAttribIconPair> _attribMap;
    CObjectVector<CExtIconPair> _extMap_Normal;
    CObjectVector<CExtIconPair> _extMap_Compressed;
    int SplitIconIndex = -1;
    int SplitIconIndex_Defined = false;

    void Clear();
    int GetIconIndex(DWORD attrib, const wchar_t *fileName);
    int GetIconIndex_DIR(DWORD attrib = FILE_ATTRIBUTE_DIRECTORY) {
        return GetIconIndex(attrib, L"__DIR__");
    }
    int GetPathIconIndex(const UString &path, DWORD attrib);
    int GetPlatformIconIndex(const QString &key, const QIcon &icon);
    QIcon GetIcon(DWORD attrib, const QString &fileName);
    QIcon GetIcon(int index) const;

  private:
    int QueryIconIndex(DWORD attrib, const QString &fileName);
    QVector<QIcon> _icons;
    QMap<QPair<QString, DWORD>, int> _pathIcons;
    QMap<QString, int> _platformIcons;
};

extern CExtToIconMap g_Ext_to_Icon_Map;

#ifdef _WIN32
HRESULT Shell_GetFileInfo_SysIconIndex_for_Path_return_HRESULT(CFSTR path, DWORD attrib,
                                                               Int32 *iconIndex);
#endif
