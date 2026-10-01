#pragma once
#include <QLabel>

// Win32 status parts have fixed bounds. Qt must allow the same compact panel
// widths; elide the presentation while retaining the full source text.
class QtStatusLabel final : public QLabel {
  public:
    explicit QtStatusLabel(QWidget *parent);
    QSize minimumSizeHint() const override;

  protected:
    void paintEvent(QPaintEvent *event) override;
    bool event(QEvent *event) override;
};
