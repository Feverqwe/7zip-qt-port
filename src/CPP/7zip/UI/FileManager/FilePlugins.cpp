// Adapted from 7-Zip FilePlugins.cpp (0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17).
// Igor Pavlov, LGPL-2.1-or-later; see third_party/7zip/License.txt.
// The retained native manager is injected instead of creating a Win32 plugin.
#include "FilePlugins.h"
#include "Common/MyCom.h"

int CExtDatabase::FindExt(const UString &ext) const {
    FOR_VECTOR(i, Exts)
    if (Exts[i].Ext.IsEqualTo_NoCase(ext))
        return (int)i;
    return -1;
}

void CExtDatabase::Read(IFolderManager *folderManager) {
    if (!folderManager)
        return;
    CMyComBSTR extBSTR;
    if (folderManager->GetExtensions(&extBSTR) != S_OK)
        return;
    UStringVector exts;
    SplitString((const wchar_t *)extBSTR, exts);
    FOR_VECTOR(i, exts) {
        const UString &ext = exts[i];
        Int32 iconIndex;
        CMyComBSTR iconPath;
        CPluginToIcon plugPair;
        if (folderManager->GetIconPath(ext, &iconPath, &iconIndex) == S_OK)
            if (iconPath) {
                plugPair.IconPath = (const wchar_t *)iconPath;
                plugPair.IconIndex = iconIndex;
            }
        const int index = FindExt(ext);
        if (index >= 0)
            Exts[index].Plugins.Add(plugPair);
        else {
            CExtPlugins extInfo;
            extInfo.Plugins.Add(plugPair);
            extInfo.Ext = ext;
            Exts.Add(extInfo);
        }
    }
}
