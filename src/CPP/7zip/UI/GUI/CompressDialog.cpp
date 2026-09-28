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
// LZMA/LZMA2 branch copied from CCompressDialog::SetOrder2 (0766b73).
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
    default:
        break;
    }
    return options;
}
// Numeric choices from CCompressDialog::SetSolidBlockSize2 (0766b73).
// UI data uses MiB instead of upstream logarithms; Auto/non-solid stay in Qt.
QList<int> SetSolidBlockSize2(QStringView format, UInt32 level) {
    QList<int> values;
    const CFormatInfo &fi = GetFormatInfo(format);
    if (!fi.Solid_())
        return values;
    if (level == 0)
        return values;
    for (unsigned i = 20; i <= 36; i++) {
        const UInt64 size = (UInt64)1 << i;
        values.append(int(size >> 20));
    }
    return values;
}
} // namespace NCompressDialog
