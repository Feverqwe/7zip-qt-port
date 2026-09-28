// Adapted from 7-Zip CPP/7zip/UI/GUI/CompressDialog.cpp (0766b73).
// Igor Pavlov, LGPL-2.1-or-later; see third_party/7zip/License.txt.
// Qt names replace CArcInfoEx / UString. SFX and external codecs are not exposed.
#include "CompressDialog.h"

namespace NCompressDialog {
static const char *const kMethodsNames[] = {"Copy",
                                            "LZMA",
                                            "LZMA2",
                                            "PPMd",
                                            "BZip2",
                                            "Deflate",
                                            "Deflate64",
                                            "PPMd"
                                            // , "ZSTD"
                                            ,
                                            "SHA256",
                                            "SHA1",
                                            "CRC32",
                                            "CRC64",
                                            "GNU",
                                            "POSIX"};

static const EMethodID g_7zMethods[] = {kLZMA2,
                                        kLZMA,
                                        kPPMd,
                                        kBZip2,
                                        kDeflate,
                                        kDeflate64
                                        // , kZSTD
                                        ,
                                        kCopy};

static const EMethodID g_7zSfxMethods[] = {kCopy, kLZMA, kLZMA2, kPPMd};

static const EMethodID g_ZipMethods[] = {
    kDeflate, kDeflate64, kBZip2, kLZMA, kPPMdZip
    // , kZSTD
};

static const EMethodID g_GZipMethods[] = {kDeflate};

static const EMethodID g_BZip2Methods[] = {kBZip2};

static const EMethodID g_XzMethods[] = {kLZMA2};

/*
static const EMethodID g_ZstdMethods[] =
{
  kZSTD
};
*/

/*
static const EMethodID g_SwfcMethods[] =
{
  kDeflate
  // kLZMA
};
*/

static const EMethodID g_TarMethods[] = {kGnu, kPosix};

static const EMethodID g_HashMethods[] = {
    kSha256, kSha1
    // , kCrc32
    // , kCrc64
};

#define METHODS_PAIR(x) Z7_ARRAY_SIZE(x), x

static const CFormatInfo g_Formats[] = {
    {"",
     // (1 << 0) | (1 << 1) | (1 << 3) | (1 << 5) | (1 << 7) | (1 << 9),
     ((UInt32)1 << 10) - 1,
     // (UInt32)(Int32)-1,
     0, NULL, kFF_MultiThread | kFF_MemUse},
    {
        "7z",
        // (1 << 0) | (1 << 1) | (1 << 3) | (1 << 5) | (1 << 7) | (1 << 9),
        (1 << 10) - 1, METHODS_PAIR(g_7zMethods),
        kFF_Filter | kFF_Solid | kFF_MultiThread | kFF_Encrypt | kFF_EncryptFileNames | kFF_MemUse |
            kFF_SFX
        // | kFF_Time_Win
    },
    {
        "Zip", (1 << 0) | (1 << 1) | (1 << 3) | (1 << 5) | (1 << 7) | (1 << 9),
        METHODS_PAIR(g_ZipMethods), kFF_MultiThread | kFF_Encrypt | kFF_MemUse
        // | kFF_Time_Win | kFF_Time_Unix | kFF_Time_DOS
    },
    {
        "GZip", (1 << 1) | (1 << 5) | (1 << 7) | (1 << 9), METHODS_PAIR(g_GZipMethods), kFF_MemUse
        // | kFF_Time_Unix
    },
    {"BZip2", (1 << 1) | (1 << 3) | (1 << 5) | (1 << 7) | (1 << 9), METHODS_PAIR(g_BZip2Methods),
     kFF_MultiThread | kFF_MemUse},
    {"xz",
     // (1 << 1) | (1 << 3) | (1 << 5) | (1 << 7) | (1 << 9),
     (1 << 10) - 1 - (1 << 0), // store (1 << 0) is not supported
     METHODS_PAIR(g_XzMethods), kFF_Solid | kFF_MultiThread | kFF_MemUse},
    /*
    {
      "zstd",
      // (1 << (MY_ZSTD_LEVEL_MAX + 1)) - 1,
      (1 << (9 + 1)) - 1,
      METHODS_PAIR(g_ZstdMethods),
      // kFF_Solid |
      kFF_MultiThread
      | kFF_MemUse
    },
    */
    /*
      {
        "Swfc",
        (1 << 1) | (1 << 3) | (1 << 5) | (1 << 7) | (1 << 9),
        METHODS_PAIR(g_SwfcMethods),
        0
      },
    */
    {
        "Tar", (1 << 0), METHODS_PAIR(g_TarMethods), 0
        // kFF_Time_Unix | kFF_Time_Win // | kFF_Time_1ns
    },
    {
        "wim", (1 << 0), 0, NULL, 0
        // | kFF_Time_Win
    },
    {"Hash", (0 << 0), METHODS_PAIR(g_HashMethods), 0}};

static unsigned GetStaticFormatIndex(QStringView name) {
    for (unsigned i = 0; i < Z7_ARRAY_SIZE(g_Formats); i++)
        if (name.compare(QLatin1StringView(g_Formats[i].Name), Qt::CaseInsensitive) == 0)
            return i;
    return 0;
}

const CFormatInfo &GetFormatInfo(QStringView name) { return g_Formats[GetStaticFormatIndex(name)]; }

QStringList GetMethodNames(QStringView name) {
    const CFormatInfo &fi = GetFormatInfo(name);
    const bool is7z = name.compare(QLatin1StringView("7z"), Qt::CaseInsensitive) == 0;
    QStringList methods;
    for (unsigned m = 0; m < fi.NumMethods; m++) {
        const int methodID = fi.MethodIDs[m];
        const char *method = kMethodsNames[methodID];
        if (is7z)
            if (methodID == kCopy || methodID == kDeflate || methodID == kDeflate64)
                continue;
        methods.append(QString::fromLatin1(method));
    }
    return methods;
}
EMethodID GetMethodID(QStringView name, QStringView format) {
    const auto &fi = GetFormatInfo(format);
    for (unsigned i = 0; i < fi.NumMethods; i++)
        if (name == QLatin1StringView(kMethodsNames[fi.MethodIDs[i]]))
            return fi.MethodIDs[i];
    return kCopy;
}
// LZMA/LZMA2 numeric branch from CCompressDialog::SetDictionary2 (0766b73).
// Registry and Win32 insertion are handled by the Qt adapter.
CDictionaryOptions SetDictionary2(EMethodID methodID, UInt32 level) {
    static const UInt32 kLzmaMaxDictSize = (UInt32)15 << 28;
    CDictionaryOptions options;
    switch (methodID) {
    case kLZMA:
    case kLZMA2: {
        {
            options.AutoDict = level <= 4 ? (UInt32)1 << (level * 2 + 16)
                               : level <= sizeof(size_t) / 2 + 4
                                   ? (UInt32)1 << (level + 20)
                                   : (UInt32)1 << (sizeof(size_t) / 2 + 24);
        }

        const size_t kLzmaMaxDictSize_Up = (size_t)1 << (20 + sizeof(size_t) / 4 * 6);

        for (unsigned i = (16 - 1) * 2; i <= (32 - 1) * 2; i++) {
            if (i < (20 - 1) * 2 && i != (16 - 1) * 2 && i != (18 - 1) * 2)
                continue;
            if (i == (20 - 1) * 2 + 1)
                continue;
            const size_t dict_up = (size_t)(2 + (i & 1)) << (i / 2);
            size_t dict = dict_up;
            if (dict_up >= kLzmaMaxDictSize)
                dict = kLzmaMaxDictSize; // we reduce dictionary

            options.Values.append(quint64(dict));
            if (dict_up >= kLzmaMaxDictSize_Up)
                break;
        }

        break;
    }

    case kPPMd: {
        options.AutoDict = (UInt32)1 << (level + 19);

        const UInt32 kPpmd_Default_4g = (UInt32)0 - ((UInt32)1 << 10);
        const size_t kPpmd_MaxDictSize_Up = (size_t)1 << (29 + sizeof(size_t) / 8);

        for (unsigned i = (20 - 1) * 2; i <= (32 - 1) * 2; i++) {
            if (i == (20 - 1) * 2 + 1)
                continue;

            const size_t dict_up = (size_t)(2 + (i & 1)) << (i / 2);
            size_t dict = dict_up;
            if (dict_up >= kPpmd_Default_4g)
                dict = kPpmd_Default_4g;

            options.Values.append(quint64(dict));
            if (dict_up >= kPpmd_MaxDictSize_Up)
                break;
        }
        break;
    }

    case kPPMdZip: {
        options.AutoDict = (UInt32)1 << (level + 19);

        for (unsigned i = 20; i <= 28; i++) {
            const UInt32 dict = (UInt32)1 << i;
            options.Values.append(quint64(dict));
        }
        break;
    }

    case kBZip2: {
        {
            if (level >= 5)
                options.AutoDict = (900 << 10);
            else if (level >= 3)
                options.AutoDict = (500 << 10);
            else
                options.AutoDict = (100 << 10);
        }

        for (unsigned i = 1; i <= 9; i++) {
            const UInt32 dict = ((UInt32)i * 100) << 10;
            options.Values.append(quint64(dict));
        }
        break;
    }

    case kDeflate:
    case kDeflate64: {
        const UInt32 dict = (methodID == kDeflate ? (UInt32)(1 << 15) : (UInt32)(1 << 16));
        options.AutoDict = dict;
        // EnableItem(IDC_COMPRESS_DICTIONARY, false);
        break;
    }

    default:
        break;
    }
    return options;
}
// Unit selection from Combo_AddDict2; Qt formats the visible text.
QString Combo_AddDict2(quint64 sizeShow) {
    unsigned moveBits = 0;
    QString unit = QStringLiteral("Б");
    if ((sizeShow & 0xFFFFF) == 0) {
        moveBits = 20;
        unit = QStringLiteral("МиБ");
    } else if ((sizeShow & 0x3FF) == 0) {
        moveBits = 10;
        unit = QStringLiteral("КиБ");
    }
    return QString::number(sizeShow >> moveBits) + QLatin1Char(' ') + unit;
}

// LZMA, Deflate and PPMd branches copied from CCompressDialog::SetOrder2 (0766b73).
// The caller replaces registry selection and ComboBox insertion with Qt data.
COrderOptions SetOrder2(EMethodID methodID, UInt32 level) {
    COrderOptions options;
    switch (methodID) {
    case kLZMA:
    case kLZMA2: {
        options.AutoOrder = (level < 7 ? 32 : 64);
        for (unsigned i = 2 * 2; i < 8 * 2; i++) {
            UInt32 order = ((UInt32)(2 + (i & 1)) << (i / 2));
            if (order > 256)
                order = 273;
            options.Values.append(int(order));
        }
        break;
    }
    case kDeflate:
    case kDeflate64: {
        {
            if (level >= 9)
                options.AutoOrder = 128;
            else if (level >= 7)
                options.AutoOrder = 64;
            else
                options.AutoOrder = 32;
        }
        for (unsigned i = 2 * 2; i < 8 * 2; i++) {
            UInt32 order = ((UInt32)(2 + (i & 1)) << (i / 2));
            if (order > 256)
                order = (methodID == kDeflate64 ? 257 : 258);
            options.Values.append(int(order));
        }

        break;
    }

    case kPPMd: {
        {
            if (level >= 9)
                options.AutoOrder = 32;
            else if (level >= 7)
                options.AutoOrder = 16;
            else if (level >= 5)
                options.AutoOrder = 6;
            else
                options.AutoOrder = 4;
        }

        for (unsigned i = 0;; i++) {
            UInt32 order = i + 2;
            if (i >= 2)
                order = (4 + ((i - 2) & 3)) << ((i - 2) / 4);
            options.Values.append(int(order));
            if (order >= 32)
                break;
        }
        break;
    }

    case kPPMdZip: {
        options.AutoOrder = level + 3;
        for (unsigned i = 2; i <= 16; i++) {
            options.Values.append(int(i));
        }
        break;
    }

    // case kBZip2:
    default:
        break;
    }
    return options;
}
bool GetOrderMode(EMethodID methodID) {
    switch (methodID) {
    case kPPMd:
    case kPPMdZip:
        return true;
    default:
        break;
    }
    return false;
}

static UInt64 Get_Lzma2_ChunkSize(UInt64 dict) {
    // we use same default chunk sizes as defined in 7z encoder and lzma2 encoder
    UInt64 cs = (UInt64)dict << 2;
    const UInt32 kMinSize = (UInt32)1 << 20;
    const UInt32 kMaxSize = (UInt32)1 << 28;
    if (cs < kMinSize)
        cs = kMinSize;
    if (cs > kMaxSize)
        cs = kMaxSize;
    if (cs < dict)
        cs = dict;
    cs += (kMinSize - 1);
    cs &= ~(UInt64)(kMinSize - 1);
    return cs;
}

QString Add_Size(UInt64 val) {
    unsigned moveBits = 0;
    QString unit = QStringLiteral("Б");
    if ((val & 0x3FFFFFFF) == 0) {
        moveBits = 30;
        unit = QStringLiteral("ГиБ");
    } else if ((val & 0xFFFFF) == 0) {
        moveBits = 20;
        unit = QStringLiteral("МиБ");
    } else if ((val & 0x3FF) == 0) {
        moveBits = 10;
        unit = QStringLiteral("КиБ");
    }
    return QString::number(val >> moveBits) + QLatin1Char(' ') + unit;
}

CSolidOptions SetSolidBlockSize2(QStringView format, UInt32 level, EMethodID methodId,
                                 UInt64 dict) {
    static const UInt32 kSolidLog_NoSolid = 0;
    static const UInt32 kSolidLog_FullSolid = 64;
    CSolidOptions options;
    const CFormatInfo &fi = GetFormatInfo(format);
    if (!fi.Solid_())
        return options;
    if (level == 0)
        return options;
    if (dict == (UInt64)(Int64)-1) {
        dict = 1 << 25; // default dict for unknown methods
                        // return;
    }

    const bool is7z = format.compare(QLatin1StringView("7z"), Qt::CaseInsensitive) == 0;
    const UInt64 cs = Get_Lzma2_ChunkSize(dict);

    // Solid Block Size
    UInt64 blockSize = cs; // for xz

    if (is7z) {
        // we use same default block sizes as defined in 7z encoder
        UInt64 kMaxSize = (UInt64)1 << 32;
        if (methodId == kLZMA2) {
            blockSize = cs << 6;
            kMaxSize = (UInt64)1 << 34;
        } else {
            UInt64 dict2 = dict;
            if (methodId == kBZip2) {
                dict2 /= 100000;
                if (dict2 < 1)
                    dict2 = 1;
                dict2 *= 100000;
            }
            blockSize = dict2 << 7;
        }

        const UInt32 kMinSize = (UInt32)1 << 24;
        if (blockSize < kMinSize)
            blockSize = kMinSize;
        if (blockSize > kMaxSize)
            blockSize = kMaxSize;
    }

    options.AutoSolid = blockSize;

    if (is7z)
        options.Values.append(kSolidLog_NoSolid);
    for (unsigned i = 20; i <= 36; i++)
        options.Values.append(i);
    options.Values.append(kSolidLog_FullSolid);
    return options;
}

// Solid conversion block copied from CCompressDialog::OnOK.
void OnOK(UInt32 solidLogSize, CInfo &Info) {
    Info.SolidBlockSize = 0;
    if (solidLogSize == (UInt32)(Int32)-1)
        Info.SolidIsSpecified = false;
    else if (solidLogSize > 0)
        Info.SolidBlockSize =
            (solidLogSize >= 64) ? (UInt64)(Int64)-1 : ((UInt64)1 << solidLogSize);
}
} // namespace NCompressDialog
