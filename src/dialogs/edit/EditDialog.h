// Adapted from 7-Zip CPP/7zip/UI/FileManager/EditDialog.h (0766b73).
// Igor Pavlov, LGPL-2.1-or-later; see third_party/7zip/License.txt.
#pragma once
#include "Common/MyString.h"
#include <QDialog>

class QPlainTextEdit;
class CEditDialog : public QDialog {
    QPlainTextEdit *_edit = nullptr;
    bool eventFilter(QObject *watched, QEvent *event) override;

  public:
    explicit CEditDialog(QWidget *parent = nullptr);
    bool OnInit();
    UString Title;
    UString Text;
};
