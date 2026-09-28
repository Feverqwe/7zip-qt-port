// Adapted from 7-Zip CPP/7zip/UI/FileManager/ListViewDialog.h (0766b73).
// Igor Pavlov, LGPL-2.1-or-later; see third_party/7zip/License.txt.
#pragma once

#include "Common/MyString.h"
#include <QDialog>

class QListWidget;
class CListViewDialog : public QDialog {
    Q_OBJECT
    QListWidget *_listView = nullptr;
    void OnOK();
    void CopyToClipboard();
    void DeleteItems();
    void ShowItemInfo();
    void OnEnter();
    bool eventFilter(QObject *watched, QEvent *event) override;

  public:
    explicit CListViewDialog(QWidget *parent = nullptr);
    bool OnInit();
    void accept() override;
    UString Title;
    bool SelectFirst = false;
    bool DeleteIsAllowed = false;
    bool StringsWereChanged = false;
    UStringVector Strings;
    int FocusedItemIndex = -1;
};
