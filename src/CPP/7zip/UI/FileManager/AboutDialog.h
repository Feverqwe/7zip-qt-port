// Adapted from 7-Zip AboutDialog.h (0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17).
// Igor Pavlov, LGPL-2.1-or-later; see third_party/7zip/License.txt.
#ifndef ZIP7_INC_ABOUT_DIALOG_H
#define ZIP7_INC_ABOUT_DIALOG_H

#include "7zip/UI/FileManager/AboutDialogRes.h"
#include "Common/MyString.h"
#include <QDialog>

class CAboutDialog : public QDialog {
  public:
    explicit CAboutDialog(QWidget *parent = nullptr);
    bool OnInit();
    void OnHelp();
    bool OnButtonClicked(unsigned buttonID, QWidget *buttonHWND);
    int Create() { return exec(); }

  private:
    void SetItemText(unsigned id, const UString &text);
};

#endif
