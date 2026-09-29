#pragma once
#include <QRect>
#include <QSize>

class QWidget;
class IQtDialogScreenServices {
  public:
    virtual ~IQtDialogScreenServices() = default;
    virtual QSize GetDialogBaseUnits() const = 0;
    virtual QRect GetWorkAreaRect() const = 0;
};

class QtDialogScreenServices final : public IQtDialogScreenServices {
  public:
    explicit QtDialogScreenServices(QWidget *window) : window_(window) {}
    QSize GetDialogBaseUnits() const override;
    QRect GetWorkAreaRect() const override;

  private:
    QWidget *window_;
};
