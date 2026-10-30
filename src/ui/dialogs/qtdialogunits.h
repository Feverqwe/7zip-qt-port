// Qt font service for the Win32 dialog-unit presentation boundary.
#pragma once
#include <QSize>

class QFont;

class QtDialogUnits {
    double _horizontal;
    double _vertical;

  public:
    explicit QtDialogUnits(const QFont &font);
    double horizontalScale() const { return _horizontal; }
    double verticalScale() const { return _vertical; }
    QSize pixels(const QSize &units) const;
};
