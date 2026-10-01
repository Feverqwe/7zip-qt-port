// Adapted from 7-Zip CPP/7zip/UI/FileManager/ListViewDialog.h (0766b73).
// Igor Pavlov, LGPL-2.1-or-later; see third_party/7zip/License.txt.
#pragma once

#include "Common/MyString.h"
#include <QDialog>
#include <QHash>

class CListView;
class CListViewDialog : public QDialog {
    Q_OBJECT
    CListView *_listView = nullptr;
    void OnOK();
    void CopyToClipboard();
    void DeleteItems();
    void ShowItemInfo();
    void OnEnter();
    bool eventFilter(QObject *watched, QEvent *event) override;
    struct CPropertyNameBinding {
        UInt32 ID;
        UString Fallback;
        bool HasFallback;
    };
    QHash<unsigned, CPropertyNameBinding> _propertyNameBindings;
    void changeEvent(QEvent *event) override;

  public:
    explicit CListViewDialog(QWidget *parent = nullptr);
    bool OnInit();
    void QtBindPropertyName(unsigned row, UInt32 id, const wchar_t *fallback);
    void accept() override;
    UString Title;
    QString SettingsPath;
    bool SelectFirst = false;
    bool DeleteIsAllowed = false;
    bool StringsWereChanged = false;
    UStringVector Strings;
    UStringVector Values;
    unsigned NumColumns = 1;
    int FocusedItemIndex = -1;
};
