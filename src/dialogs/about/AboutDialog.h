// Adapted from 7-Zip AboutDialog.h (0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17).
// Igor Pavlov, LGPL-2.1-or-later; see third_party/7zip/License.txt.
// Modified 2026-10-18: remove the obsolete presentation-only text setter.
#ifndef ZIP7_INC_ABOUT_DIALOG_H
#define ZIP7_INC_ABOUT_DIALOG_H

#include "7zip/UI/FileManager/AboutDialogRes.h"
#include "Common/MyString.h"
#include <QDialog>
class CCodecs;

class CAboutDialog : public QDialog {
  public:
    explicit CAboutDialog(QWidget *parent = nullptr, const CCodecs *codecs = nullptr);
    bool OnInit();
    void OnHelp();
    bool OnButtonClicked(unsigned buttonID, QWidget *buttonHWND);
    int Create() { return exec(); }

  protected:
    void changeEvent(QEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

  private:
    void UpdateMinimumHeight();
    const CCodecs *codecs_ = nullptr; // Caller retains the immutable catalog lease.
};

#endif
