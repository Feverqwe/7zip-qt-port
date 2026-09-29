// Qt widget/platform services for the source ProgressDialog2 presentation.
#pragma once

#include "Common/MyString.h"
#include <QDialog>
#include <QElapsedTimer>
#include <QHash>

class CListView;
class QProgressBar;

struct CQtProgressRect {
    int left;
    int top;
    int right;
    int bottom;
};

class CQtProgressBar {
    QProgressBar *_view = nullptr;

  public:
    void Attach(QWidget *widget);
    void SetRange32(int minimum, int maximum);
    void SetPos(int position);
};

class CQtProgressListView {
    CListView *_view = nullptr;

  public:
    void Attach(QWidget *widget);
    void InsertColumn(unsigned column, const wchar_t *text, int width);
    int InsertItem(unsigned index, const UString &text);
    void SetSubItem(unsigned index, int column, const UString &text);
    void SetColumnWidthAuto(int column);
    void Move(int x, int y, int width, int height);
    int GetNextSelectedItem(int index) const;
    int GetItemCount() const;
};

class CQtProgressDialog : public QDialog {
    QHash<unsigned, QWidget *> _controls;
    QElapsedTimer _clock;
    double _unitX;
    double _unitY;
    void AddControl(unsigned id, const char *name, QWidget *widget, int x, int y, int width,
                    int height);

  protected:
    QWidget *GetItem(unsigned id) const;
    void SetItemText(unsigned id, const UString &text);
    void GetItemText(unsigned id, UString &text) const;
    void SetText(const UString &text);
    void ShowItem_Bool(unsigned id, bool visible);
    void HideItem(unsigned id);
    void GetItemSizes(unsigned id, int &width, int &height) const;
    void GetClientRectOfItem(unsigned id, CQtProgressRect &rect) const;
    void MoveItem(unsigned id, int x, int y, int width, int height);
    int Units_To_Pixels_X(int units) const;
    UInt32 GetTickCount() const;
    void NormalizeSize(bool fullNormalize);
    static void ChangeSubWindowSizeX(QWidget *widget, int width);
    static void ClipboardSetText(const UString &text);

  public:
    explicit CQtProgressDialog(QWidget *parent = nullptr);
};
