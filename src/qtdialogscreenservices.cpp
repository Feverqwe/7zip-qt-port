#include "qtdialogscreenservices.h"
#include <QApplication>
#include <QFontMetrics>
#include <QScreen>
#include <QWidget>

QSize QtDialogScreenServices::GetDialogBaseUnits() const {
    const QFontMetrics font(QApplication::font());
    return {font.averageCharWidth(), font.height()};
}

QRect QtDialogScreenServices::GetWorkAreaRect() const {
    QScreen *screen = window_ ? window_->screen() : QApplication::primaryScreen();
    return screen ? screen->availableGeometry() : QRect();
}
