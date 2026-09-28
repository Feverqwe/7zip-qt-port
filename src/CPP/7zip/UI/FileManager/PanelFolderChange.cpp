// Adapted from 7-Zip CPP/7zip/UI/FileManager/PanelFolderChange.cpp (0766b73).
// Igor Pavlov, LGPL-2.1-or-later; see third_party/7zip/License.txt.
// History is supplied explicitly instead of _appState. Return the selected
// path to the Qt navigation adapter instead of BindToPathAndRefresh.
#include "AppState.h"
#include "ListViewDialog.h"
#include "Panel.h"

void CPanel::SetBookmark(CFastFolders &fastFolders, unsigned index,
                         const UString &currentFolderPrefix) {
    fastFolders.SetString(index, currentFolderPrefix);
}

UString CPanel::OpenBookmark(CFastFolders &fastFolders, unsigned index) {
    return fastFolders.GetString(index);
}

QString CPanel::FoldersHistory(CFolderHistory &history, const QString &settingsPath,
                               QWidget *parent) {
    CListViewDialog listViewDialog(parent);
    listViewDialog.DeleteIsAllowed = true;
    listViewDialog.SelectFirst = true;
    listViewDialog.Title = ToUString(QStringLiteral("История папок"));
    history.GetList(listViewDialog.Strings);
    listViewDialog.OnInit();
    if (listViewDialog.exec() != QDialog::Accepted)
        return QString();
    UString selectString;
    if (listViewDialog.StringsWereChanged) {
        history.RemoveAll();
        for (int i = (int)listViewDialog.Strings.Size() - 1; i >= 0; i--)
            history.AddString(listViewDialog.Strings[(unsigned)i]);
        history.Save(settingsPath);
        if (listViewDialog.FocusedItemIndex >= 0)
            selectString = listViewDialog.Strings[(unsigned)listViewDialog.FocusedItemIndex];
    } else {
        if (listViewDialog.FocusedItemIndex >= 0)
            selectString = listViewDialog.Strings[(unsigned)listViewDialog.FocusedItemIndex];
    }
    return listViewDialog.FocusedItemIndex >= 0 ? ToQString(selectString) : QString();
}
