// Adapted from 7-Zip CPP/7zip/UI/FileManager/SysIconUtils.cpp (0766b73).
// Igor Pavlov, LGPL-2.1-or-later; see third_party/7zip/License.txt.
// QFileIconProvider presents portable icons; the native Windows shell image
// list is bridged to QIcon. Source cache policy and limits are retained.
#include "SysIconUtils.h"
#include <QFileIconProvider>
#include <QFileInfo>
#include <QImage>
#include <QPixmap>
#include <string>

#ifdef _WIN32
#include "Common/StringConvert.h"
#include "Windows/FileIO.h"
#include <ShlObj.h>
#include <commctrl.h>
#include <shellapi.h>

int Shell_GetFileInfo_SysIconIndex_for_CSIDL(int csidl) {
    LPITEMIDLIST pidl = NULL;
    if (SHGetSpecialFolderLocation(NULL, csidl, &pidl) == S_OK)
        if (pidl) {
            SHFILEINFO shFileInfo;
            shFileInfo.iIcon = -1;
            const DWORD_PTR res =
                SHGetFileInfo((LPCTSTR)(const void *)(pidl), FILE_ATTRIBUTE_DIRECTORY, &shFileInfo,
                              sizeof(shFileInfo), SHGFI_PIDL | SHGFI_SYSICONINDEX);
            // we use OLE2.dll function here
            CoTaskMemFree(pidl);
            if (res)
                return shFileInfo.iIcon;
        }
    return -1;
}

HIMAGELIST Shell_Get_SysImageList_smallIcons(bool smallIcons) {
    SHFILEINFO shFileInfo;
    Z7_memset_0_VAR(shFileInfo);
    const DWORD_PTR res =
        SHGetFileInfo(TEXT(""), FILE_ATTRIBUTE_DIRECTORY, &shFileInfo, sizeof(shFileInfo),
                      SHGFI_USEFILEATTRIBUTES | SHGFI_SYSICONINDEX |
                          (smallIcons ? SHGFI_SMALLICON : SHGFI_LARGEICON));
    return (HIMAGELIST)res;
}

// Original Unicode branch and HRESULT wrapper; obsolete ANSI dispatch omitted.
static DWORD_PTR Shell_GetFileInfo_SysIconIndex_for_Path_attrib_iconIndexRef(CFSTR path,
                                                                             DWORD attrib,
                                                                             int &iconIndex) {
    SHFILEINFOW shFileInfo;
    shFileInfo.iIcon = -1;
    const DWORD_PTR res =
        ::SHGetFileInfoW(fs2us(path), attrib ? attrib : FILE_ATTRIBUTE_ARCHIVE, &shFileInfo,
                         sizeof(shFileInfo), SHGFI_USEFILEATTRIBUTES | SHGFI_SYSICONINDEX);
    iconIndex = shFileInfo.iIcon;
    return res;
}

HRESULT Shell_GetFileInfo_SysIconIndex_for_Path_return_HRESULT(CFSTR path, DWORD attrib,
                                                               Int32 *iconIndex) {
    *iconIndex = -1;
    int iconIndexTemp;
    if (Shell_GetFileInfo_SysIconIndex_for_Path_attrib_iconIndexRef(path, attrib, iconIndexTemp)) {
        *iconIndex = iconIndexTemp;
        return S_OK;
    }
    return GetLastError_noZero_HRESULT();
}
#endif

CExtToIconMap g_Ext_to_Icon_Map;

void CExtToIconMap::Clear() {
    SplitIconIndex_Defined = false;
    SplitIconIndex = -1;
    _extMap_Normal.Clear();
    _extMap_Compressed.Clear();
    _attribMap.Clear();
    _icons.clear();
    _pathIcons.clear();
    _platformIcons.clear();
#ifdef _WIN32
    _systemIcons.clear();
#endif
}

void CExtToIconMap::EnsureDefaultIcon() {
    // The original shell image list reserves index zero for its default image.
    if (_icons.isEmpty()) {
        QFileIconProvider provider;
        _icons.append(provider.icon(QFileIconProvider::File));
    }
}

int CExtToIconMap::GetPathIconIndex(const UString &path, DWORD attrib) {
    EnsureDefaultIcon();
    const auto key = qMakePair(QString::fromWCharArray(path), attrib);
    const auto found = _pathIcons.constFind(key);
    if (found != _pathIcons.cend())
        return found.value();
    if (_pathIcons.size() >= (1u << 16))
        return GetIconIndex(attrib, path);
#ifdef _WIN32
    Int32 systemIndex;
    QIcon icon;
    if (Shell_GetFileInfo_SysIconIndex_for_Path_return_HRESULT(us2fs(path), attrib, &systemIndex) ==
        S_OK)
        icon = GetSystemIcon(systemIndex);
#else
    QFileIconProvider provider;
    const QFileInfo info(key.first);
    QIcon icon = info.exists() ? provider.icon(info) : QIcon();
#endif
    if (icon.isNull())
        return GetIconIndex(attrib, path);
    const int index = _icons.size();
    _icons.append(icon);
    _pathIcons.insert(key, index);
    return index;
}

int CExtToIconMap::GetPlatformIconIndex(const QString &key, const QIcon &icon) {
    EnsureDefaultIcon();
    const auto found = _platformIcons.constFind(key);
    if (found != _platformIcons.cend())
        return found.value();
    const int index = _icons.size();
    _icons.append(icon);
    _platformIcons.insert(key, index);
    return index;
}

int CExtToIconMap::QueryIconIndex(DWORD attrib, const QString &fileName) {
    EnsureDefaultIcon();
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
    return GetIcon(index);
}

QIcon CExtToIconMap::GetIcon(int index) const {
    return index >= 0 && index < _icons.size() ? _icons[index] : QIcon();
}

#ifdef _WIN32
// Source Shell_Get_SysImageList_smallIcons retrieval, with Qt image transport.
QIcon CExtToIconMap::GetSystemIcon(int index) {
    if (index < 0)
        return {};
    const auto cached = _systemIcons.constFind(index);
    if (cached != _systemIcons.cend())
        return cached.value();
    QIcon icon;
    for (bool smallIcons : {true, false}) {
        const HIMAGELIST images = Shell_Get_SysImageList_smallIcons(smallIcons);
        if (!images)
            continue;
        HICON native = ImageList_GetIcon(images, index, ILD_NORMAL);
        if (!native)
            continue;
        icon.addPixmap(QPixmap::fromImage(QImage::fromHICON(native)));
        ::DestroyIcon(native);
    }
    if (icon.isNull())
        return {};
    _systemIcons.insert(index, icon);
    return icon;
}
#endif
