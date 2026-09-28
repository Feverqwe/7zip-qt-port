// Adapted from 7-Zip CPP/7zip/UI/Common/ZipRegistry (0766b73).
// Igor Pavlov, LGPL-2.1-or-later; see third_party/7zip/License.txt.
#pragma once
#include "../../Common/MethodProps.h"
#include "7zip/UI/Common/ExtractMode.h"
#include "Common/MyVector.h"
#include <QString>
namespace NCompression {
struct CMemUse {
    // UString Str;
    bool IsDefined;
    bool IsPercent;
    UInt64 Val;

    CMemUse() : IsDefined(false), IsPercent(false), Val(0) {}

    void Clear() {
        // Str.Empty();
        IsDefined = false;
        IsPercent = false;
        Val = 0;
    }

    UInt64 GetBytes(UInt64 ramSize) const {
        if (!IsPercent)
            return Val;
        return Calc_From_Val_Percents(ramSize, Val);
    }
    void Parse(const QString &s);
};

// Compression controls ported from upstream; time/options/encryption remain pending.
struct CFormatOptions {
    UInt32 Level;
    UInt32 Dictionary;
    UInt32 Order;
    UInt32 BlockLogSize;
    UInt32 NumThreads;
    QString FormatID;
    QString Method;
    QString MemUse;

    void Reset_BlockLogSize() { BlockLogSize = (UInt32)(Int32)-1; }
    void ResetForLevelChange() {
        BlockLogSize = NumThreads = Level = Dictionary = Order = (UInt32)(Int32)-1;
        Method.clear();
    }
    CFormatOptions() { ResetForLevelChange(); }
};
struct CInfo {
    CObjectVector<CFormatOptions> Formats;
    void Save(const QString &path) const;
    void Load(const QString &path);
};
} // namespace NCompression

namespace NExtract {
struct CInfo {
    NPathMode::EEnum PathMode;
    NOverwriteMode::EEnum OverwriteMode;
    bool PathMode_Force;
    bool OverwriteMode_Force;
    CBoolPair SplitDest;
    CBoolPair ShowPassword;
    CObjectVector<QString> Paths;
    void Save(const QString &path) const;
    void Load(const QString &path);
};
} // namespace NExtract
