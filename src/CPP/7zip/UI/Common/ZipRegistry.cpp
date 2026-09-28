// Adapted from 7-Zip CPP/7zip/UI/Common/ZipRegistry (0766b73).
// Igor Pavlov, LGPL-2.1-or-later; see third_party/7zip/License.txt.
#include "ZipRegistry.h"
#include "Common/StringToInt.h"
#include <QSettings>

inline wchar_t MyCharLower_Ascii(wchar_t c) {
    if (c >= 'A' && c <= 'Z')
        return (wchar_t)(c + 0x20);
    return c;
}

namespace NCompression {
static bool ParseMemUse(const wchar_t *s, CMemUse &mu) {
    mu.Clear();

    bool percentMode = false;
    {
        const wchar_t c = *s;
        if (MyCharLower_Ascii(c) == 'p') {
            percentMode = true;
            s++;
        }
    }
    const wchar_t *end;
    UInt64 number = ConvertStringToUInt64(s, &end);
    if (end == s)
        return false;

    wchar_t c = *end;

    if (percentMode) {
        if (c != 0)
            return false;
        mu.IsPercent = true;
        mu.Val = number;
        return true;
    }

    if (c == 0) {
        mu.Val = number;
        return true;
    }

    c = MyCharLower_Ascii(c);

    const wchar_t c1 = end[1];

    if (c == '%') {
        if (c1 != 0)
            return false;
        mu.IsPercent = true;
        mu.Val = number;
        return true;
    }

    if (c == 'b') {
        if (c1 != 0)
            return false;
        mu.Val = number;
        return true;
    }

    if (c1 != 0)
        if (MyCharLower_Ascii(c1) != 'b' || end[2] != 0)
            return false;

    unsigned numBits;
    switch (c) {
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
    default:
        return false;
    }
    if (number >= ((UInt64)1 << (64 - numBits)))
        return false;
    mu.Val = number << numBits;
    return true;
}

void CMemUse::Parse(const QString &s) { IsDefined = ParseMemUse(s.toStdWString().c_str(), *this); }

static const char *const kOptionsKeyName = "Compression/Options";
static const char *const kMemUse = sizeof(size_t) == 4 ? "MemUse32" : "MemUse64";

// Registry DWORD adapters preserve upstream's absent-value sentinel.
static void Key_Set_UInt32(QSettings &key, const char *name, UInt32 value) {
    if (value == (UInt32)(Int32)-1)
        key.remove(QLatin1String(name));
    else
        key.setValue(QLatin1String(name), value);
}
static void Key_Get_UInt32(QSettings &key, const char *name, UInt32 &value) {
    value = (UInt32)(Int32)-1;
    bool ok;
    const UInt32 v = key.value(QLatin1String(name)).toUInt(&ok);
    if (ok)
        value = v;
}

void CInfo::Save(const QString &path) const {
    QSettings optionsKey(path, QSettings::IniFormat);
    optionsKey.setValue("Compression/Archiver", ArcType);
    QStringList arcPaths;
    FOR_VECTOR(i, ArcPaths)
    arcPaths.append(ArcPaths[i]);
    optionsKey.setValue("Compression/ArcHistory", arcPaths);
    optionsKey.beginGroup(QLatin1String(kOptionsKeyName));
    // Only this ported options group is replaced. Other application settings survive.
    optionsKey.remove(QString());
    FOR_VECTOR(i, Formats) {
        const CFormatOptions &fo = Formats[i];
        optionsKey.beginGroup(fo.FormatID);
        optionsKey.setValue("Method", fo.Method);
        optionsKey.setValue(QLatin1String(kMemUse), fo.MemUse);
        Key_Set_UInt32(optionsKey, "Level", fo.Level);
        Key_Set_UInt32(optionsKey, "Dictionary", fo.Dictionary);
        Key_Set_UInt32(optionsKey, "Order", fo.Order);
        Key_Set_UInt32(optionsKey, "BlockSize", fo.BlockLogSize);
        Key_Set_UInt32(optionsKey, "NumThreads", fo.NumThreads);
        optionsKey.endGroup();
    }
}

void CInfo::Load(const QString &path) {
    ArcPaths.Clear();
    Formats.Clear();
    ArcType = QStringLiteral("7z");
    QSettings optionsKey(path, QSettings::IniFormat);
    if (optionsKey.contains("Compression/Archiver"))
        ArcType = optionsKey.value("Compression/Archiver").toString();
    for (const QString &s : optionsKey.value("Compression/ArcHistory").toStringList())
        ArcPaths.Add(s);
    optionsKey.beginGroup(QLatin1String(kOptionsKeyName));
    const QStringList formatIDs = optionsKey.childGroups();
    for (int i = 0; i < formatIDs.size(); i++) {
        CFormatOptions fo;
        fo.FormatID = formatIDs[i];
        optionsKey.beginGroup(fo.FormatID);
        fo.Method = optionsKey.value("Method").toString();
        fo.MemUse = optionsKey.value(QLatin1String(kMemUse)).toString();
        Key_Get_UInt32(optionsKey, "Level", fo.Level);
        Key_Get_UInt32(optionsKey, "Dictionary", fo.Dictionary);
        Key_Get_UInt32(optionsKey, "Order", fo.Order);
        Key_Get_UInt32(optionsKey, "BlockSize", fo.BlockLogSize);
        Key_Get_UInt32(optionsKey, "NumThreads", fo.NumThreads);
        Formats.Add(fo);
        optionsKey.endGroup();
    }
}
} // namespace NCompression

namespace NExtract {
void CInfo::Save(const QString &path) const {
    QSettings key(path, QSettings::IniFormat);
    key.beginGroup("Extraction");
    if (PathMode_Force)
        key.setValue("ExtractMode", UInt32(PathMode));
    if (OverwriteMode_Force)
        key.setValue("OverwriteMode", UInt32(OverwriteMode));
    if (SplitDest.Def)
        key.setValue("SplitDest", SplitDest.Val);
    if (ShowPassword.Def)
        key.setValue("ShowPassword", ShowPassword.Val);
    QStringList paths;
    FOR_VECTOR(i, Paths)
    paths.append(Paths[i]);
    key.setValue("PathHistory", paths);
}

void CInfo::Load(const QString &path) {
    PathMode = NPathMode::kCurPaths;
    PathMode_Force = false;
    OverwriteMode = NOverwriteMode::kAsk;
    OverwriteMode_Force = false;
    SplitDest.Val = true;
    SplitDest.Def = false;
    ShowPassword.Init();
    Paths.Clear();
    QSettings key(path, QSettings::IniFormat);
    // Migration of the former Qt history; all further writes use the upstream group.
    const QStringList legacyPaths = key.value("history/extract").toStringList();
    key.beginGroup("Extraction");
    const QStringList paths = key.value("PathHistory", legacyPaths).toStringList();
    for (const QString &s : paths)
        Paths.Add(s);
    bool ok;
    UInt32 v = key.value("ExtractMode").toUInt(&ok);
    if (ok && v <= NPathMode::kAbsPaths) {
        PathMode = (NPathMode::EEnum)v;
        PathMode_Force = true;
    }
    v = key.value("OverwriteMode").toUInt(&ok);
    if (ok && v <= NOverwriteMode::kRenameExisting) {
        OverwriteMode = (NOverwriteMode::EEnum)v;
        OverwriteMode_Force = true;
    }
    SplitDest.Def = key.contains("SplitDest");
    if (SplitDest.Def)
        SplitDest.Val = key.value("SplitDest").toBool();
    if (key.contains("ShowPassword")) {
        ShowPassword.Def = true;
        ShowPassword.Val = key.value("ShowPassword").toBool();
    }
}
} // namespace NExtract
