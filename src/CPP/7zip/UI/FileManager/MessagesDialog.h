// Adapted from 7-Zip CPP/7zip/UI/FileManager/MessagesDialog.h (0766b73).
// Igor Pavlov, LGPL-2.1-or-later; see third_party/7zip/License.txt.
#ifndef ZIP7_INC_MESSAGES_DIALOG_H
#define ZIP7_INC_MESSAGES_DIALOG_H

#include "Common/MyString.h"
#include "qtmodalresult.h"
#include "qtprogresscontrols.h"
#include <QDialog>

class CListView;
class QPushButton;

class CMessagesDialog : public QDialog {
    CQtProgressListView _messageList;
    CListView *_list;
    QPushButton *_close;
    bool _initialized = false;

    void AddMessageDirect(const wchar_t *message);
    void AddMessage(const wchar_t *message);
    bool OnInit();
    bool OnSize(int xSize, int ySize);
    void resizeEvent(QResizeEvent *event) override;
    void changeEvent(QEvent *event) override;

  public:
    const UStringVector *Messages = nullptr;
    explicit CMessagesDialog(QWidget *parent = nullptr);
    INT_PTR Create(QWidget *parent = nullptr);
    void reject() override;
};

#endif
