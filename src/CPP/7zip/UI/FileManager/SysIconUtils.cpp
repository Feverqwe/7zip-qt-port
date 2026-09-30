// Adapted from 7-Zip CPP/7zip/UI/FileManager/SysIconUtils.cpp (0766b73).
// Igor Pavlov, LGPL-2.1-or-later; see third_party/7zip/License.txt.
// SHGetFileInfo/Win32 image list are replaced by QFileIconProvider. The
// source attribute/extension/split-number caches and limits are retained.
#include "SysIconUtils.h"
#include <QFileIconProvider>
#include <QFileInfo>
#include <string>

CExtToIconMap g_Ext_to_Icon_Map;

void CExtToIconMap::Clear() {
    SplitIconIndex_Defined = false;
    SplitIconIndex = -1;
    _extMap_Normal.Clear();
    _extMap_Compressed.Clear();
    _attribMap.Clear();
    _icons.clear();
}

int CExtToIconMap::QueryIconIndex(DWORD attrib, const QString &fileName) {
    QFileIconProvider provider;
    QIcon icon;
    if (attrib & FILE_ATTRIBUTE_DIRECTORY)
        icon = provider.icon(QFileIconProvider::Folder);
    else
        icon = provider.icon(QFileInfo(fileName));
    if (icon.isNull())
        icon = provider.icon(QFileIconProvider::File);
    _icons.append(icon);
    return _icons.size() - 1;
}

static int FindInSorted_Attrib(const CRecordVector<CAttribIconPair> &vect, DWORD attrib,
                               unsigned &insertPos) {
    unsigned left = 0, right = vect.Size();
    while (left != right) {
        const unsigned mid = (left + right) / 2;
        const DWORD midAttrib = vect[mid].Attrib;
        if (attrib == midAttrib)
            return (int)mid;
        if (attrib < midAttrib)
            right = mid;
        else
            left = mid + 1;
    }
    insertPos = left;
    return -1;
}

static int FindInSorted_Ext(const CObjectVector<CExtIconPair> &vect, const wchar_t *ext,
                            unsigned &insertPos) {
    unsigned left = 0, right = vect.Size();
    while (left != right) {
        const unsigned mid = (left + right) / 2;
        const int compare = MyStringCompareNoCase(ext, vect[mid].Ext);
        if (compare == 0)
            return (int)mid;
        if (compare < 0)
            right = mid;
        else
            left = mid + 1;
    }
    insertPos = left;
    return -1;
}

int CExtToIconMap::GetIconIndex(DWORD attrib, const wchar_t *fileName) {
    int dotPos = -1;
    unsigned i;
    for (i = 0;; i++) {
        const wchar_t c = fileName[i];
        if (c == 0)
            break;
        if (c == '.')
            dotPos = (int)i;
    }

    if ((attrib & FILE_ATTRIBUTE_DIRECTORY) || dotPos < 0)
        for (unsigned k = 0;; k++) {
            if (k >= 2)
                return -1;
            unsigned insertPos = 0;
            const int index = FindInSorted_Attrib(_attribMap, attrib, insertPos);
            if (index >= 0)
                return _attribMap[(unsigned)index].IconIndex;
            CAttribIconPair pair;
            pair.IconIndex = QueryIconIndex(attrib, QStringLiteral("__DIR__"));
            if (_attribMap.Size() < (1u << 16) || attrib < (1u << 15)) {
                pair.Attrib = attrib;
                _attribMap.Insert(insertPos, pair);
                return pair.IconIndex;
            }
            if (pair.IconIndex >= 0)
                return pair.IconIndex;
            attrib = (attrib & FILE_ATTRIBUTE_DIRECTORY) ? FILE_ATTRIBUTE_DIRECTORY
                                                         : FILE_ATTRIBUTE_ARCHIVE;
        }

    CObjectVector<CExtIconPair> &map =
        (attrib & FILE_ATTRIBUTE_COMPRESSED) ? _extMap_Compressed : _extMap_Normal;
    const wchar_t *ext = fileName + dotPos + 1;
    unsigned insertPos = 0;
    const int index = FindInSorted_Ext(map, ext, insertPos);
    if (index >= 0)
        return map[index].IconIndex;

    for (i = 0;; i++) {
        const wchar_t c = ext[i];
        if (c == 0)
            break;
        if (c < L'0' || c > L'9')
            break;
    }
    if (i != 0 && ext[i] == 0) {
        if (!SplitIconIndex_Defined) {
            SplitIconIndex = QueryIconIndex(FILE_ATTRIBUTE_ARCHIVE, QStringLiteral("__FILE__.001"));
            SplitIconIndex_Defined = true;
        }
        return SplitIconIndex;
    }

    CExtIconPair pair;
    pair.Ext = ext;
    pair.IconIndex = QueryIconIndex(attrib & FILE_ATTRIBUTE_COMPRESSED
                                        ? FILE_ATTRIBUTE_ARCHIVE | FILE_ATTRIBUTE_COMPRESSED
                                        : FILE_ATTRIBUTE_ARCHIVE,
                                    QStringLiteral("__FILE__.") + QString::fromWCharArray(ext));
    if (map.Size() < (1u << 16))
        map.Insert(insertPos, pair);
    return pair.IconIndex;
}

QIcon CExtToIconMap::GetIcon(DWORD attrib, const QString &fileName) {
    const std::wstring name = fileName.toStdWString();
    const int index = GetIconIndex(attrib, name.c_str());
    return index >= 0 && index < _icons.size() ? _icons[index] : QIcon();
}
