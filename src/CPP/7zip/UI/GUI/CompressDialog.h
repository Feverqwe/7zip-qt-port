// Adapted from 7-Zip UI/GUI/CompressDialog.cpp (0766b73).
// Igor Pavlov, LGPL-2.1-or-later; see third_party/7zip/License.txt.
#pragma once

#include "Common/MyTypes.h"
#include <QStringList>
#include <QStringView>

namespace NCompressDialog {
enum EMethodID {
    kCopy,
    kLZMA,
    kLZMA2,
    kPPMd,
    kBZip2,
    kDeflate,
    kDeflate64,
    kPPMdZip,
    // kZSTD,
    kSha256,
    kSha1,
    kCrc32,
    kCrc64,
    kGnu,
    kPosix
};

inline constexpr UInt32 kFF_Filter = 1 << 0;
inline constexpr UInt32 kFF_Solid = 1 << 1;
inline constexpr UInt32 kFF_MultiThread = 1 << 2;
inline constexpr UInt32 kFF_Encrypt = 1 << 3;
inline constexpr UInt32 kFF_EncryptFileNames = 1 << 4;
inline constexpr UInt32 kFF_MemUse = 1 << 5;
inline constexpr UInt32 kFF_SFX = 1 << 6;

/*
inline constexpr UInt32 kFF_Time_Win  = 1 << 10;
inline constexpr UInt32 kFF_Time_Unix = 1 << 11;
inline constexpr UInt32 kFF_Time_DOS  = 1 << 12;
inline constexpr UInt32 kFF_Time_1ns  = 1 << 13;
*/

struct CFormatInfo {
    const char *Name;
    UInt32 LevelsMask;
    unsigned NumMethods;
    const EMethodID *MethodIDs;

    UInt32 Flags;

    bool Filter_() const { return (Flags & kFF_Filter) != 0; }
    bool Solid_() const { return (Flags & kFF_Solid) != 0; }
    bool MultiThread_() const { return (Flags & kFF_MultiThread) != 0; }
    bool Encrypt_() const { return (Flags & kFF_Encrypt) != 0; }
    bool EncryptFileNames_() const { return (Flags & kFF_EncryptFileNames) != 0; }
    bool MemUse_() const { return (Flags & kFF_MemUse) != 0; }
    bool SFX_() const { return (Flags & kFF_SFX) != 0; }
};

struct CDictionaryOptions {
    UInt32 AutoDict = 0;
    QList<quint64> Values;
};
CDictionaryOptions SetDictionary2(EMethodID methodID, UInt32 level);
QString Combo_AddDict2(quint64 sizeShow);

struct COrderOptions {
    UInt32 AutoOrder = 1;
    QList<int> Values;
};
EMethodID GetMethodID(QStringView name, QStringView format);
bool GetOrderMode(EMethodID methodID);
COrderOptions SetOrder2(EMethodID methodID, UInt32 level);
QList<int> SetSolidBlockSize2(QStringView format, UInt32 level);

const CFormatInfo &GetFormatInfo(QStringView name);
QStringList GetMethodNames(QStringView name);
} // namespace NCompressDialog
