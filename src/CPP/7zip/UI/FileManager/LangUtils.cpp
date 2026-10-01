// Adapted from 7-Zip LangUtils.cpp (0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17).
// Igor Pavlov, LGPL-2.1-or-later; see third_party/7zip/License.txt.
// Source load/reload/fallback bodies. Qt supplies module directory/default locale.
#include "LangUtils.h"
#include "RegistryUtils.h"
#include "qtlanguageservices.h"
#include <QDir>
#include <QLocale>
#include <QMutexLocker>
#include <QRecursiveMutex>

UString g_LangID;
CLang g_Lang;
static bool g_Loaded = false;
static QRecursiveMutex g_CriticalSection;

bool LangOpen(CLang &lang, CFSTR fileName) { return lang.Open(fileName, "7-Zip"); }

FString GetLangDirPrefix(const QString &directory) {
    // NDLL::GetModuleDirPrefix + Lang; a bundled resource is staged only when
    // no installed Lang directory exists, because CLang consumes native files.
    const QString path = directory.isEmpty() ? QtLanguageDirectory() : directory;
    const std::wstring native =
        QDir::toNativeSeparators(QDir(path).absolutePath() + "/").toStdWString();
    return us2fs(UString(native.c_str()));
}

void LoadLangOneTime() {
    QMutexLocker lock(&g_CriticalSection);
    if (g_Loaded)
        return;
    g_Loaded = true;
    ReloadLang();
}

UString LangString(UInt32 langID) {
    QMutexLocker lock(&g_CriticalSection);
    const wchar_t *s = g_Lang.Get(langID);
    if (s)
        return s;
    return NWindows::MyLoadString(langID);
}

void AddLangString(UString &s, UInt32 langID) { s += LangString(langID); }

void LangString(UInt32 langID, UString &dest) {
    QMutexLocker lock(&g_CriticalSection);
    const wchar_t *s = g_Lang.Get(langID);
    if (s) {
        dest = s;
        return;
    }
    NWindows::MyLoadString(langID, dest);
}

void LangString_OnlyFromLangFile(UInt32 langID, UString &dest) {
    QMutexLocker lock(&g_CriticalSection);
    dest.Empty();
    const wchar_t *s = g_Lang.Get(langID);
    if (s)
        dest = s;
}

#ifdef _WIN32
static const char *const kLangs =
    "ar.bg.ca.zh.-tw.-cn.cs.da.de.el.en.es.fi.fr.he.hu.is."
    "it.ja.ko.nl.no.=nb.=nn.pl.pt.-br.rm.ro.ru.sr.=hr.-spl.-spc.=hr.=bs.sk.sq.sv.th.tr."
    "ur.id.uk.be.sl.et.lv.lt.tg.fa.vi.hy.az.eu.hsb.mk."
    "st.ts.tn.ve.xh.zu.af.ka.fo.hi.mt.se.ga.yi.ms.kk."
    "ky.sw.tk.uz.-latn.-cyrl.tt.bn.pa.-in.gu.or.ta.te.kn.ml.as.mr.sa."
    "mn.=mn.=mng.bo.cy.kh.lo.my.gl.kok..sd.syr.si..iu.am.tzm."
    "ks.ne.fy.ps.tl.dv..ff.ha..yo.qu.st.ba.lb.kl."
    "ig.kr.om.ti.gn..la.so.ii..arn..moh..br.."
    "ug.mi.oc.co.";

static void FindShortNames(UInt32 primeLang, AStringVector &names) {
    UInt32 index = 0;
    for (const char *p = kLangs; *p != 0;) {
        const char *p2 = p;
        for (; *p2 != '.'; p2++)
            ;
        bool isSub = (p[0] == '-' || p[0] == '=');
        if (!isSub)
            index++;
        if (index >= primeLang) {
            if (index > primeLang)
                break;
            AString s;
            if (isSub) {
                if (p[0] == '-')
                    s = names[0];
                else
                    p++;
            }
            while (p != p2)
                s.Add_Char((char)(Byte)*p++);
            names.Add(s);
        }
        p = p2 + 1;
    }
}
#endif

void Lang_GetShortNames_for_DefaultLang(AStringVector &names, unsigned &subLang) {
    names.Clear();
    subLang = 0;
#ifdef _WIN32
    const LANGID sysLang = GetSystemDefaultLangID();
    const LANGID userLang = GetUserDefaultLangID();
    if (PRIMARYLANGID(sysLang) != PRIMARYLANGID(userLang))
        return;
    const LANGID langID = userLang;
    const WORD primLang = (WORD)(PRIMARYLANGID(langID));
    subLang = SUBLANGID(langID);
    FindShortNames(primLang, names);
#else
    // POSIX has no Windows LANGID service. Qt supplies locale names; the
    // source caller still tries the regional candidate, then the primary.
    const QString shortName = QLocale::system().bcp47Name().toLower();
    const QString primary = shortName.section('-', 0, 0);
    names.Add(AString(primary.toLatin1().constData()));
    if (shortName != primary) {
        names.Add(AString(shortName.toLatin1().constData()));
        subLang = 1;
    }
#endif
}

static void OpenDefaultLang(const QString &directory) {
    AStringVector names;
    unsigned subLang;
    Lang_GetShortNames_for_DefaultLang(names, subLang);
    {
        const FString dirPrefix(GetLangDirPrefix(directory));
        for (unsigned i = 0; i < 2; i++) {
            const unsigned index = (i == 0 ? subLang : 0);
            if (index < names.Size()) {
                const AString &name = names[index];
                if (!name.IsEmpty()) {
                    FString path(dirPrefix);
                    path += name;
                    path += ".txt";
                    if (LangOpen(g_Lang, path)) {
                        g_LangID = name;
                        return;
                    }
                }
            }
        }
    }
}

void ReloadLang(const QString &settingsPath, const QString &directory) {
    QMutexLocker lock(&g_CriticalSection);
    g_Lang.Clear();
    ReadRegLang(g_LangID, settingsPath.isEmpty() ? FmSettingsPath() : settingsPath);
    if (g_LangID.IsEmpty()) {
        OpenDefaultLang(directory);
        return;
    }
    if (g_LangID.Len() > 1 || g_LangID[0] != L'-') {
        FString s = us2fs(g_LangID);
        if (s.ReverseFind_PathSepar() < 0) {
            if (s.ReverseFind_Dot() < 0)
                s += ".txt";
            s.Insert(0, GetLangDirPrefix(directory));
            LangOpen(g_Lang, s);
        }
    }
}
