// Adapted from 7-Zip CPP/7zip/UI/FileManager/LangUtils.h (0766b73).
// Igor Pavlov, LGPL-2.1-or-later; see third_party/7zip/License.txt.
// Upstream resource-ID boundary; Qt supplies catalogs and locale selection.
#ifndef ZIP7_INC_LANG_UTILS_H
#define ZIP7_INC_LANG_UTILS_H

#include "localization/ResourceString.h"
#include <QString>

extern UString g_LangID;
class IQtLangDialogControls;
class IQtLangWindowText;
struct CIDLangPair {
    UInt32 ControlID;
    UInt32 LangID;
};
void LangSetDlgItemText(IQtLangDialogControls &dialog, UInt32 controlID, UInt32 langID);
void LangSetDlgItems(IQtLangDialogControls &dialog, const UInt32 *ids, unsigned numItems);
void LangSetDlgItems_Colon(IQtLangDialogControls &dialog, const UInt32 *ids, unsigned numItems);
void LangSetDlgItems_RemoveColon(IQtLangDialogControls &dialog, const UInt32 *ids,
                                 unsigned numItems);
void LangSetWindowText(IQtLangWindowText &window, UInt32 langID);
void LoadLangOneTime();
void ReloadLang(const QString &settingsPath = {}, const QString &directory = {});
UString LangString(UInt32 langID);
void LangString(UInt32 langID, UString &dest);
void AddLangString(UString &s, UInt32 langID);
void LangString_OnlyFromLangFile(UInt32 langID, UString &dest);

#endif
