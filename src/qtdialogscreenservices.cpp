#include "qtdialogscreenservices.h"
#include "CPP/Windows/Control/Dialog.h"
#include <QApplication>
#include <QEvent>
#include <QFontMetrics>
#include <QScreen>
#include <QWidget>

namespace {
enum class EGeometry { Size, Position };

void ApplyGeometry(QWidget &window, EGeometry geometry, bool fullNormalize) {
    const QtDialogScreenServices screen(&window);
    if (geometry == EGeometry::Size)
        NWindows::NControl::CDialog::NormalizeSize(window, screen, fullNormalize);
    else {
        const QtDialogScreenServices parentScreen(window.parentWidget());
        NWindows::NControl::CDialog::NormalizePosition(window, screen, parentScreen);
    }
}

class CNormalizeOnShow final : public QObject {
    QWidget &window_;
    const EGeometry geometry_;
    const bool fullNormalize_;

  public:
    CNormalizeOnShow(QWidget &window, EGeometry geometry, bool fullNormalize)
        : QObject(&window), window_(window), geometry_(geometry), fullNormalize_(fullNormalize) {
        window_.installEventFilter(this);
    }
    bool eventFilter(QObject *, QEvent *event) override {
        if (event->type() == QEvent::Show) {
            window_.removeEventFilter(this);
            ApplyGeometry(window_, geometry_, fullNormalize_);
            deleteLater();
        }
        return false;
    }
};
} // namespace

void QtNormalizeDialogSize(QWidget &window, bool fullNormalize) {
    if (window.isVisible())
        ApplyGeometry(window, EGeometry::Size, fullNormalize);
    else
        new CNormalizeOnShow(window, EGeometry::Size, fullNormalize);
}

void QtNormalizeDialogPosition(QWidget &window) {
    if (window.isVisible())
        ApplyGeometry(window, EGeometry::Position, false);
    else
        new CNormalizeOnShow(window, EGeometry::Position, false);
}

QSize QtDialogScreenServices::GetDialogBaseUnits() const {
    const QFontMetrics font(QApplication::font());
    return {font.averageCharWidth(), font.height()};
}

QRect QtDialogScreenServices::GetWorkAreaRect() const {
    QScreen *screen = window_ ? window_->screen() : QApplication::primaryScreen();
    return screen ? screen->availableGeometry() : QRect();
}
