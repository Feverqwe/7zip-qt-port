// Adapted from 7-Zip LangUtils.cpp (0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17).
// Igor Pavlov, LGPL-2.1-or-later; see third_party/7zip/License.txt.
// Source resource helpers/fallback bodies; Qt catalogs replace the native file loader.
#include "LangUtils.h"
#include "RegistryUtils.h"
#include "qtlangdialogcontrols.h"
#include "qtlanguageservices.h"
#include "qtmodalresult.h"
#include <QMutexLocker>
#include <QRecursiveMutex>
#ifdef _WIN32
#include "Common/StringConvert.h"
#include "Windows/DLL.h"
#endif

UString g_LangID;
static bool g_Loaded = false;
static QRecursiveMutex g_CriticalSection;

static UString SelectedLangString(UInt32 id) {
    return UString(QtTranslatedResourceString(id).toStdWString().c_str());
}

void LoadLangOneTime() {
    QMutexLocker lock(&g_CriticalSection);
    if (g_Loaded)
        return;
    g_Loaded = true;
    ReloadLang();
}

void LangSetDlgItemText(IQtLangDialogControls &dialog, UInt32 controlID, UInt32 langID) {
    QMutexLocker lock(&g_CriticalSection);
    const UString s = SelectedLangString(langID);
    if (!s.IsEmpty())
        dialog.SetItemText(controlID, s);
}

static const CIDLangPair kLangPairs[] = {{IDOK, 401},      {IDCANCEL, 402}, {IDYES, 406},
                                         {IDNO, 407},      {IDCLOSE, 408},  {IDHELP, 409},
                                         {IDCONTINUE, 411}};

void LangSetDlgItems(IQtLangDialogControls &dialog, const UInt32 *ids, unsigned numItems) {
    QMutexLocker lock(&g_CriticalSection);
    unsigned i;
    for (i = 0; i < Z7_ARRAY_SIZE(kLangPairs); i++) {
        const CIDLangPair &pair = kLangPairs[i];
        if (dialog.HasItem(pair.ControlID)) {
            const UString s = SelectedLangString(pair.LangID);
            if (!s.IsEmpty())
                dialog.SetItemText(pair.ControlID, s);
        }
    }
    for (i = 0; i < numItems; i++) {
        const UInt32 id = ids[i];
        LangSetDlgItemText(dialog, id, id);
    }
}

void LangSetDlgItems_Colon(IQtLangDialogControls &dialog, const UInt32 *ids, unsigned numItems) {
    QMutexLocker lock(&g_CriticalSection);
    for (unsigned i = 0; i < numItems; i++) {
        const UInt32 id = ids[i];
        const UString s = SelectedLangString(id);
        if (!s.IsEmpty()) {
            UString s2 = s;
            s2.Add_Colon();
            dialog.SetItemText(id, s2);
        }
    }
}

void LangSetDlgItems_RemoveColon(IQtLangDialogControls &dialog, const UInt32 *ids,
                                 unsigned numItems) {
    QMutexLocker lock(&g_CriticalSection);
    for (unsigned i = 0; i < numItems; i++) {
        const UInt32 id = ids[i];
        const UString s = SelectedLangString(id);
        if (!s.IsEmpty()) {
            UString s2 = s;
            if (!s2.IsEmpty() && s2.Back() == ':')
                s2.DeleteBack();
            dialog.SetItemText(id, s2);
        }
    }
}

void LangSetWindowText(IQtLangWindowText &window, UInt32 langID) {
    QMutexLocker lock(&g_CriticalSection);
    const UString s = SelectedLangString(langID);
    if (!s.IsEmpty())
        window.SetText(s);
}

UString LangString(UInt32 langID) {
    QMutexLocker lock(&g_CriticalSection);
    const UString s = SelectedLangString(langID);
    if (!s.IsEmpty())
        return s;
    return NWindows::MyLoadString(langID);
}

void AddLangString(UString &s, UInt32 langID) { s += LangString(langID); }

void LangString(UInt32 langID, UString &dest) {
    QMutexLocker lock(&g_CriticalSection);
    const UString s = SelectedLangString(langID);
    if (!s.IsEmpty()) {
        dest = s;
        return;
    }
    NWindows::MyLoadString(langID, dest);
}

void LangString_OnlyFromLangFile(UInt32 langID, UString &dest) {
    QMutexLocker lock(&g_CriticalSection);
    dest.Empty();
    const UString s = SelectedLangString(langID);
    if (!s.IsEmpty())
        dest = s;
}

void ReloadLang(const QString &settingsPath, const QString &directory) {
    QMutexLocker lock(&g_CriticalSection);
    UString requested;
    ReadRegLang(requested, settingsPath.isEmpty() ? FmSettingsPath() : settingsPath);
    QString catalogDirectory = directory;
#ifdef _WIN32
    if (catalogDirectory.isEmpty() && !QCoreApplication::instance())
        catalogDirectory =
            QString::fromWCharArray(fs2us(NWindows::NDLL::GetModuleDirPrefix())) + "translations";
#endif
    const QString selected =
        QtLoadTranslation(QString::fromWCharArray(requested), catalogDirectory);
    g_LangID = selected.toStdWString().c_str();
}
