#pragma once

#include <QLabel>

class QDialogButtonBox;
class QVBoxLayout;
class QWidget;

// Shared presentation for redesigned dialogs; callers own fields and actions.
QVBoxLayout *QtCreateDialogLayout(QWidget &window, QDialogButtonBox &buttons);

// Keep wrapped captions in the layout minimum at their current width.
class QtWrappingLabel final : public QLabel {
  public:
    using QLabel::QLabel;
    QSize minimumSizeHint() const override;

  protected:
    void resizeEvent(QResizeEvent *event) override;
};
