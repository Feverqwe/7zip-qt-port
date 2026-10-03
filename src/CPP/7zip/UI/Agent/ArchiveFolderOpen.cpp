// Agent/ArchiveFolderOpen.cpp
// Adapted from 7-Zip 0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17.
// Igor Pavlov, LGPL-2.1-or-later; see third_party/7zip/License.txt.
// Partial port: original icon load/lookup/extension bodies. The immutable
// retained catalog replaces global loading; archive opening remains elsewhere.
#include "../../../Windows/ResourceString.h"
#include "Agent.h"
#include "Common/StringToInt.h"

static const UINT kIconTypesResId = 100;

void CCodecIcons::LoadIcons(HMODULE m) {
    IconPairs.Clear();
    UString iconTypes;
#ifdef _WIN32
    NWindows::MyLoadString(m, kIconTypesResId, iconTypes);
#else
    // ELF/Mach-O modules have no Win32 STRINGTABLE; compile the exact official
    // Format7zF resource as the internal Qt format presentation baseline.
    (void)m;
    NWindows::MyLoadString(kIconTypesResId, iconTypes);
#endif
    UStringVector pairs;
    SplitString(iconTypes, pairs);
    FOR_VECTOR(i, pairs) {
        const UString &s = pairs[i];
        int pos = s.Find(L':');
        CIconPair iconPair;
        iconPair.IconIndex = -1;
        if (pos < 0)
            pos = (int)s.Len();
        else {
            const UString num = s.Ptr((unsigned)pos + 1);
            if (!num.IsEmpty()) {
                const wchar_t *end;
                iconPair.IconIndex = (int)ConvertStringToUInt32(num, &end);
                if (*end != 0)
                    continue;
            }
        }
        iconPair.Ext = s.Left((unsigned)pos);
        IconPairs.Add(iconPair);
    }
}

bool CCodecIcons::FindIconIndex(const UString &ext, int &iconIndex) const {
    iconIndex = -1;
    FOR_VECTOR(i, IconPairs) {
        const CIconPair &pair = IconPairs[i];
        if (ext.IsEqualTo_NoCase(pair.Ext)) {
            iconIndex = pair.IconIndex;
            return true;
        }
    }
    return false;
}

void CArchiveFolderManager::LoadFormats() {
    if (WasLoaded)
        return;
    // LoadGlobalCodecs is replaced by the already validated retained catalog.
    CodecIconsVector.Clear();
    FOR_VECTOR(i, _codecs.Libs) {
        CCodecIcons &ci = CodecIconsVector.AddNew();
#ifdef _WIN32
        ci.LoadIcons(_codecs.Libs[i].Lib.Get_HMODULE());
#endif
    }
#ifdef _WIN32
    InternalIcons.LoadIcons(GetModuleHandleW(NULL));
#else
    InternalIcons.LoadIcons(NULL);
#endif
    WasLoaded = true;
}

static void AddIconExt(const CCodecIcons &lib, UString &dest) {
    FOR_VECTOR(i, lib.IconPairs) {
        dest.Add_Space_if_NotEmpty();
        dest += lib.IconPairs[i].Ext;
    }
}

HRESULT CArchiveFolderManager::GetExtensions(BSTR *extensions) {
    *extensions = NULL;
    LoadFormats();
    UString res;
    FOR_VECTOR(i, CodecIconsVector)
    AddIconExt(CodecIconsVector[i], res);
    AddIconExt(InternalIcons, res);
    return StringToBstr(res, extensions);
}

HRESULT CArchiveFolderManager::GetIconPath(const wchar_t *ext, BSTR *iconPath, Int32 *iconIndex) {
    *iconPath = NULL;
    *iconIndex = 0;
    LoadFormats();
    FOR_VECTOR(i, CodecIconsVector) {
        int ii;
        if (CodecIconsVector[i].FindIconIndex(ext, ii)) {
            const CCodecLib &lib = _codecs.Libs[i];
            *iconIndex = ii;
            return StringToBstr(fs2us(lib.Path), iconPath);
        }
    }
    int ii;
    if (InternalIcons.FindIconIndex(ext, ii)) {
        FString path;
        if (NWindows::NDLL::MyGetModuleFileName(path)) {
            *iconIndex = ii;
            return StringToBstr(fs2us(path), iconPath);
        }
    }
    return S_OK;
}
