// Adapted from 7-Zip CPP/7zip/UI/FileManager/LangUtils.h (0766b73).
// Igor Pavlov, LGPL-2.1-or-later; see third_party/7zip/License.txt.
// Source language-file owner; Qt supplies directory and default-locale services.
#ifndef ZIP7_INC_LANG_UTILS_H
#define ZIP7_INC_LANG_UTILS_H

#include "CPP/Windows/ResourceString.h"
#include "Common/Lang.h"
#include <QString>

extern UString g_LangID;
extern CLang g_Lang;
class IQtLangDialogControls;
struct CIDLangPair {
    UInt32 ControlID;
    UInt32 LangID;
};
void LangSetDlgItemText(IQtLangDialogControls &dialog, UInt32 controlID, UInt32 langID);
void LangSetDlgItems(IQtLangDialogControls &dialog, const UInt32 *ids, unsigned numItems);
bool LangOpen(CLang &lang, CFSTR fileName);
FString GetLangDirPrefix(const QString &directory = {});
void Lang_GetShortNames_for_DefaultLang(AStringVector &names, unsigned &subLang);
void LoadLangOneTime();
void ReloadLang(const QString &settingsPath = {}, const QString &directory = {});
UString LangString(UInt32 langID);
void LangString(UInt32 langID, UString &dest);
void AddLangString(UString &s, UInt32 langID);
void LangString_OnlyFromLangFile(UInt32 langID, UString &dest);

#endif
