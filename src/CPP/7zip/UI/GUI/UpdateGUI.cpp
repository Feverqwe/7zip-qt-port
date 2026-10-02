// Adapted from 7-Zip CPP/7zip/UI/GUI/UpdateGUI.cpp at 0766b73.
// Igor Pavlov, LGPL-2.1-or-later; see third_party/7zip/License.txt.
#include "UpdateGUI.h"
#include "../Common/WorkDir.h"

#include "Common/StringConvert.h"
#include "Common/StringToInt.h"
#include "Windows/FileFind.h"

HRESULT
NCompressDialog::PrepareFormatSelection(CCompressDialog &dialog, const CCodecs &codecs,
                                        const CObjectVector<NWildcard::CCensorPath> &censor) {
    bool oneFile = false;
    NWindows::NFile::NFind::CFileInfo fileInfo;
    UString name;
    if (censor.Size() > 0) {
        const NWildcard::CCensorPath &cp = censor[0];
        if (cp.Include) {
            {
                if (fileInfo.Find(us2fs(cp.Path))) {
                    if (censor.Size() == 1)
                        oneFile = !fileInfo.IsDir();
                }
            }
        }
    }
    NCompressDialog::CInfo &di = dialog.Info;
    dialog.ArcFormats = &codecs.Formats;
    // Qt's source picker can change selection without reconstructing the modal.
    dialog.ArcIndices.Clear();
    FOR_VECTOR(i, codecs.Formats) {
        const CArcInfoEx &ai = codecs.Formats[i];
        if (!ai.UpdateEnabled)
            continue;
        if (!oneFile && ai.Flags_KeepName())
            continue;
        if ((int)i != di.FormatIndex) {
            if (ai.Flags_HashHandler())
                continue;
            if (ai.Name.IsEqualTo_Ascii_NoCase("swfc"))
                if (!oneFile || name.Len() < 4 ||
                    !StringsAreEqualNoCase_Ascii(name.RightPtr(4), ".swf"))
                    continue;
        }
        dialog.ArcIndices.Add(i);
    }
    if (dialog.ArcIndices.IsEmpty())
        return E_FAIL; // Qt presents the original No Update Engines result.
    dialog.OriginalFileName = fs2us(fileInfo.Name);
    di.KeepName = !oneFile;
    return S_OK;
}

// Source ShowDialog working-directory block; return the field instead of the
// unavailable in-process CUpdateOptions owner. The engine consumes it via -w.
FString NCompressDialog::GetCompressionWorkingDir(const UString &arcPath,
                                                  const QString &settingsPath) {
    NWorkDir::CInfo workDirInfo;
    workDirInfo.Load(settingsPath);
    FString workingDir;
    if (workDirInfo.Mode != NWorkDir::NMode::kCurrent) {
        FString fullPath;
        NWindows::NFile::NDir::MyGetFullPathName(us2fs(arcPath), fullPath);
        FString namePart;
        workingDir = GetWorkDir(workDirInfo, fullPath, namePart);
        NWindows::NFile::NDir::CreateComplexDir(workingDir);
    }
    return workingDir;
}

static void AddProp_UString(CObjectVector<CProperty> &properties, const char *name,
                            const UString &value) {
    CProperty prop;
    prop.Name = name;
    prop.Value = value;
    properties.Add(prop);
}

// Existing Qt encryption controls supply QString; all source properties use UString.
static void AddProp_UString(CObjectVector<CProperty> &properties, const char *name,
                            const QString &value) {
    AddProp_UString(properties, name, UString(value.toStdWString().c_str()));
}

static void AddProp_UInt32(CObjectVector<CProperty> &properties, const char *name, UInt32 value) {
    UString s;
    s.Add_UInt32(value);
    AddProp_UString(properties, name, s);
}

static void AddProp_bool(CObjectVector<CProperty> &properties, const char *name, bool value) {
    AddProp_UString(properties, name, UString(value ? "on" : "off"));
}

static void AddProp_BoolPair(CObjectVector<CProperty> &properties, const char *name,
                             const CBoolPair &bp) {
    if (bp.Def)
        AddProp_bool(properties, name, bp.Val);
}

static void SplitOptionsToStrings(const UString &src, UStringVector &strings) {
    SplitString(src, strings);
    FOR_VECTOR(i, strings) {
        UString &s = strings[i];
        if (s.Len() > 2 && s[0] == '-' && MyCharLower_Ascii(s[1]) == 'm')
            s.DeleteFrontal(2);
    }
}

static bool IsThereMethodOverride(bool is7z, const UStringVector &strings) {
    FOR_VECTOR(i, strings) {
        const UString &s = strings[i];
        if (is7z) {
            const wchar_t *end;
            UInt64 n = ConvertStringToUInt64(s, &end);
            if (n == 0 && *end == L'=')
                return true;
        } else {
            if (s.Len() > 0)
                if (s[0] == L'm' && s[1] == L'=')
                    return true;
        }
    }
    return false;
}

static void ParseAndAddPropertires(CObjectVector<CProperty> &properties,
                                   const UStringVector &strings) {
    FOR_VECTOR(i, strings) {
        const UString &s = strings[i];
        CProperty property;
        const int index = s.Find(L'=');
        if (index < 0)
            property.Name = s;
        else {
            property.Name.SetFrom(s, (unsigned)index);
            property.Value = s.Ptr(index + 1);
        }
        properties.Add(property);
    }
}

static void AddProp_Size(CObjectVector<CProperty> &properties, const char *name,
                         const UInt64 size) {
    UString s;
    s.Add_UInt64(size);
    s.Add_Char('b');
    AddProp_UString(properties, name, s);
}

static void SetOutProperties(CObjectVector<CProperty> &properties, const NCompressDialog::CInfo &di,
                             bool is7z, bool setMethod) {
    if (di.Level != (UInt32)(Int32)-1)
        AddProp_UInt32(properties, "x", (UInt32)di.Level);
    if (setMethod) {
        if (!di.Method.IsEmpty())
            AddProp_UString(properties, is7z ? "0" : "m", di.Method);
        if (di.Dict64 != (UInt64)(Int64)-1) {
            AString name;
            if (is7z)
                name = "0";
            name += (di.OrderMode ? "mem" : "d");
            AddProp_Size(properties, name, di.Dict64);
        }
        /*
        if (di.Dict64_Chain != (UInt64)(Int64)-1)
        {
          AString name;
          if (is7z)
            name = "0";
          name += "dc";
          AddProp_Size(properties, name, di.Dict64_Chain);
        }
        */
        if (di.Order != (UInt32)(Int32)-1) {
            AString name;
            if (is7z)
                name = "0";
            name += (di.OrderMode ? "o" : "fb");
            AddProp_UInt32(properties, name, (UInt32)di.Order);
        }
    }

    if (!di.EncryptionMethod.isEmpty())
        AddProp_UString(properties, "em", di.EncryptionMethod);

    if (di.EncryptHeadersIsAllowed)
        AddProp_bool(properties, "he", di.EncryptHeaders);

    if (di.SolidIsSpecified)
        AddProp_Size(properties, "s", di.SolidBlockSize);

    if (
        // di.MultiThreadIsAllowed &&
        di.NumThreads != (UInt32)(Int32)-1)
        AddProp_UInt32(properties, "mt", di.NumThreads);

    const NCompression::CMemUse &memUse = di.MemUsage;
    if (memUse.IsDefined) {
        const char *kMemUse = "memuse";
        if (memUse.IsPercent) {
            UString s;
            // s += 'p'; // for debug: alternate percent method
            s.Add_UInt64(memUse.Val);
            s.Add_Char('%');
            AddProp_UString(properties, kMemUse, s);
        } else
            AddProp_Size(properties, kMemUse, memUse.Val);
    }

    AddProp_BoolPair(properties, "tm", di.MTime);
    AddProp_BoolPair(properties, "tc", di.CTime);
    AddProp_BoolPair(properties, "ta", di.ATime);

    if (di.TimePrec != (UInt32)(Int32)-1)
        AddProp_UInt32(properties, "tp", di.TimePrec);
}

// Native ShowDialog property orchestration, with a returned property vector
// instead of the not-yet-ported CUpdateOptions encoder owner.
CObjectVector<CProperty> NCompressDialog::GetCompressionProperties(const CInfo &info, bool is7z) {
    CObjectVector<CProperty> properties;
    UStringVector optionStrings;
    SplitOptionsToStrings(info.Options, optionStrings);
    const bool methodOverride = IsThereMethodOverride(is7z, optionStrings);
    SetOutProperties(properties, info, is7z, !methodOverride);
    ParseAndAddPropertires(properties, optionStrings);
    return properties;
}

// UpdateGUI.cpp::g_UpdateMode_Pairs. The Win32 dialog's transport is replaced
// by CreateArchiveDialog; the original action-set mapping remains here.
struct C_UpdateMode_ToAction_Pair {
    NCompressDialog::NUpdateMode::EEnum UpdateMode;
    const NUpdateArchive::CActionSet *ActionSet;
};

static const C_UpdateMode_ToAction_Pair g_UpdateMode_Pairs[] = {
    {NCompressDialog::NUpdateMode::kAdd, &NUpdateArchive::k_ActionSet_Add},
    {NCompressDialog::NUpdateMode::kUpdate, &NUpdateArchive::k_ActionSet_Update},
    {NCompressDialog::NUpdateMode::kFresh, &NUpdateArchive::k_ActionSet_Fresh},
    {NCompressDialog::NUpdateMode::kSync, &NUpdateArchive::k_ActionSet_Sync}};

static int FindUpdateMode(NCompressDialog::NUpdateMode::EEnum mode) {
    for (unsigned i = 0; i < sizeof(g_UpdateMode_Pairs) / sizeof(g_UpdateMode_Pairs[0]); i++)
        if (mode == g_UpdateMode_Pairs[i].UpdateMode)
            return int(i);
    return -1;
}

const NUpdateArchive::CActionSet *NCompressDialog::FindUpdateActionSet(NUpdateMode::EEnum mode) {
    const int index = FindUpdateMode(mode);
    return index < 0 ? nullptr : g_UpdateMode_Pairs[index].ActionSet;
}
