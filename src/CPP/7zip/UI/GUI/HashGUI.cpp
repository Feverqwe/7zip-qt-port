// HashGUI.cpp
// Adapted from 7-Zip 0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17.
// Igor Pavlov, LGPL-2.1-or-later; see third_party/7zip/License.txt.
// Native bundle result formatting/presentation dependencies. The separate full
// HashCalcGUI/hash-file workflow remains mandatory with its hash owner.
#include "HashGUI.h"
#include "../FileManager/LangUtils.h"
#include "../FileManager/ListViewDialog.h"
#include "7zip/UI/FileManager/PropertyNameRes.h"
#include "7zip/UI/FileManager/resourceGui.h"
#include "Common/IntToString.h"
void AddValuePair(CPropNameValPairs &pairs, UINT resourceID, UInt64 value) {
    CProperty &pair = pairs.AddNew();
    AddLangString(pair.Name, resourceID);
    char sz[32];
    ConvertUInt64ToString(value, sz);
    pair.Value = sz;
}

void AddSizeValuePair(CPropNameValPairs &pairs, UINT resourceID, UInt64 value) {
    CProperty &pair = pairs.AddNew();
    LangString(resourceID, pair.Name);
    AddSizeValue(pair.Value, value);
}

static const unsigned k_DigestStringSize =
    k_HashCalc_DigestSize_Max * 2 + k_HashCalc_ExtraSize * 2 + 16;

static void AddHashString(CProperty &s, const CHasherState &h, unsigned digestIndex) {
    char temp[k_DigestStringSize];
    h.WriteToString(digestIndex, temp);
    s.Value = temp;
}

static void AddHashResString(CPropNameValPairs &s, const CHasherState &h, unsigned digestIndex,
                             UInt32 resID) {
    CProperty &pair = s.AddNew();
    UString &s2 = pair.Name;
    LangString(resID, s2);
    UString name(h.Name);
    s2.Replace(L"CRC", name);
    s2.Replace(L":", L"");
    AddHashString(pair, h, digestIndex);
}

void AddHashBundleRes(CPropNameValPairs &s, const CHashBundle &hb) {
    if (hb.NumErrors != 0)
        AddValuePair(s, IDS_PROP_NUM_ERRORS, hb.NumErrors);

    if (hb.NumFiles == 1 && hb.NumDirs == 0 && !hb.FirstFileName.IsEmpty()) {
        CProperty &pair = s.AddNew();
        LangString(IDS_PROP_NAME, pair.Name);
        pair.Value = hb.FirstFileName;
    } else {
        if (!hb.MainName.IsEmpty()) {
            CProperty &pair = s.AddNew();
            LangString(IDS_PROP_NAME, pair.Name);
            pair.Value = hb.MainName;
        }
        if (hb.NumDirs != 0)
            AddValuePair(s, IDS_PROP_FOLDERS, hb.NumDirs);
        AddValuePair(s, IDS_PROP_FILES, hb.NumFiles);
    }

    AddSizeValuePair(s, IDS_PROP_SIZE, hb.FilesSize);

    if (hb.NumAltStreams != 0) {
        AddValuePair(s, IDS_PROP_NUM_ALT_STREAMS, hb.NumAltStreams);
        AddSizeValuePair(s, IDS_PROP_ALT_STREAMS_SIZE, hb.AltStreamsSize);
    }

    FOR_VECTOR(i, hb.Hashers) {
        const CHasherState &h = hb.Hashers[i];
        if (hb.NumFiles == 1 && hb.NumDirs == 0) {
            CProperty &pair = s.AddNew();
            pair.Name += h.Name;
            AddHashString(pair, h, k_HashCalc_Index_DataSum);
        } else {
            AddHashResString(s, h, k_HashCalc_Index_DataSum, IDS_CHECKSUM_CRC_DATA);
            AddHashResString(s, h, k_HashCalc_Index_NamesSum, IDS_CHECKSUM_CRC_DATA_NAMES);
        }
        if (hb.NumAltStreams != 0) {
            AddHashResString(s, h, k_HashCalc_Index_StreamsSum, IDS_CHECKSUM_CRC_STREAMS_NAMES);
        }
    }
}

void AddHashBundleRes(UString &s, const CHashBundle &hb) {
    CPropNameValPairs pairs;
    AddHashBundleRes(pairs, hb);

    FOR_VECTOR(i, pairs) {
        const CProperty &pair = pairs[i];
        s += pair.Name;
        s += ": ";
        s += pair.Value;
        s.Add_LF();
    }

    if (hb.NumErrors == 0 && hb.Hashers.IsEmpty()) {
        s.Add_LF();
        AddLangString(s, IDS_MESSAGE_NO_ERRORS);
        s.Add_LF();
    }
}

void ShowHashResults(const CPropNameValPairs &propPairs, QWidget *hwnd) {
    CListViewDialog lv(hwnd);

    FOR_VECTOR(i, propPairs) {
        const CProperty &pair = propPairs[i];
        lv.Strings.Add(pair.Name);
        lv.Values.Add(pair.Value);
    }

    lv.Title = LangString(IDS_CHECKSUM_INFORMATION);
    lv.DeleteIsAllowed = true;
    lv.SelectFirst = false;
    lv.NumColumns = 2;

    lv.OnInit();
    lv.exec();
}

void ShowHashResults(const CHashBundle &hb, QWidget *hwnd) {
    CPropNameValPairs propPairs;
    AddHashBundleRes(propPairs, hb);
    ShowHashResults(propPairs, hwnd);
}
