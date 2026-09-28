// Adapted from 7-Zip CPP/7zip/UI/FileManager/SplitUtils.cpp (0766b73).
// Igor Pavlov, LGPL-2.1-or-later; see third_party/7zip/License.txt.
// UString / CRecordVector use standard containers; the combo box uses Qt.
// SplitUtils.cpp

#include "SplitUtils.h"

#include <QComboBox>

#include "Common/StringToInt.h"

bool ParseVolumeSizes(const std::wstring &s, std::vector<UInt64> &values) {
    values.clear();
    bool prevIsNumber = false;
    for (unsigned i = 0; i < s.size();) {
        wchar_t c = s[i++];
        if (c == L' ')
            continue;
        if (c == L'-')
            return true;
        if (prevIsNumber) {
            prevIsNumber = false;
            unsigned numBits = 0;
            switch ((c >= L'A' && c <= L'Z' ? c + (L'a' - L'A') : c)) {
            case 'b':
                continue;
            case 'k':
                numBits = 10;
                break;
            case 'm':
                numBits = 20;
                break;
            case 'g':
                numBits = 30;
                break;
            case 't':
                numBits = 40;
                break;
            }
            if (numBits != 0) {
                UInt64 &val = values.back();
                if (val >= ((UInt64)1 << (64 - numBits)))
                    return false;
                val <<= numBits;

                for (; i < s.size(); i++)
                    if (s[i] == L' ')
                        break;
                continue;
            }
        }
        i--;
        const wchar_t *start = s.c_str() + i;
        const wchar_t *end;
        UInt64 val = ConvertStringToUInt64(start, &end);
        if (start == end)
            return false;
        if (val == 0)
            return false;
        values.push_back(val);
        prevIsNumber = true;
        i += (unsigned)(end - start);
    }
    return true;
}

static const char *const k_Sizes[] = {
    "10M",
    "100M",
    "1000M",
    "650M - CD",
    "700M - CD",
    "4092M - FAT",
    "4480M - DVD" //  4489 MiB limit
    ,
    "8128M - DVD DL" //  8147 MiB limit
    ,
    "23040M - BD" // 23866 MiB limit
                  // , "1457664 - 3.5\" floppy"
};

void AddVolumeItems(QComboBox &combo) {
    for (unsigned i = 0; i < Z7_ARRAY_SIZE(k_Sizes); i++)
        combo.addItem(QString::fromLatin1(k_Sizes[i]));
}

UInt64 GetNumberOfVolumes(UInt64 size, const std::vector<UInt64> &volSizes) {
    if (size == 0 || volSizes.size() == 0)
        return 1;
    for (size_t i = 0; i < volSizes.size(); ++i) {
        UInt64 volSize = volSizes[i];
        if (volSize >= size)
            return i + 1;
        size -= volSize;
    }
    UInt64 volSize = volSizes.back();
    if (volSize == 0)
        return (UInt64)(Int64)-1;
    return volSizes.size() + (size - 1) / volSize + 1;
}
