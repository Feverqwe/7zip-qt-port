#pragma once

#include <QLabel>

class QVBoxLayout;
class QWidget;
class QPushButton;
class QDialog;

// Shared presentation for redesigned dialogs; callers own fields and actions.
QVBoxLayout *QtCreateDialogLayout(QWidget &window, QWidget &footer);
void QtSizeIconButton(QPushButton &button);
// Opt in only for contextual forms, never modeless reports or operation progress.
void QtUseContextDialogModality(QDialog &dialog);

// Keep wrapped captions in the layout minimum at their current width.
class QtWrappingLabel final : public QLabel {
  public:
    using QLabel::QLabel;
    QSize minimumSizeHint() const override;

  protected:
    void resizeEvent(QResizeEvent *event) override;
};
