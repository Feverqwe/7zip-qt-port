#pragma once

#include <QWidget>

class QLabel;
class QToolButton;

// Presentation-only disclosure: hiding content never changes its values/enabled state.
class QtDisclosure final : public QWidget {
    Q_OBJECT
  public:
    explicit QtDisclosure(QWidget *content, QWidget *parent = nullptr);
    QToolButton *button() const { return button_; }
    void setExpanded(bool expanded);
    bool isExpanded() const;
    void setSummary(const QString &text);

  private:
    QWidget *content_;
    QToolButton *button_;
    QLabel *summary_;
};
