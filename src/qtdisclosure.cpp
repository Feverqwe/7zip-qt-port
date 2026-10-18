#include "qtdisclosure.h"
#include <QFrame>
#include <QLabel>
#include <QStyleOptionToolButton>
#include <QStylePainter>
#include <QToolButton>
#include <QVBoxLayout>

namespace {
class DisclosureButton final : public QToolButton {
  public:
    using QToolButton::QToolButton;

  protected:
    void paintEvent(QPaintEvent *) override {
        QStyleOptionToolButton option;
        initStyleOption(&option);
        QStylePainter painter(this);
        // Keep the native glyph, but constrain its rect independently of the
        // toolbar arrow size. The full button remains the interaction target.
        QStyleOption arrow;
        arrow.initFrom(this);
        arrow.rect =
            QStyle::visualRect(layoutDirection(), rect(), QRect(4, (height() - 8) / 2, 8, 8));
        painter.drawPrimitive(isChecked() ? QStyle::PE_IndicatorArrowDown
                                          : (layoutDirection() == Qt::RightToLeft
                                                 ? QStyle::PE_IndicatorArrowLeft
                                                 : QStyle::PE_IndicatorArrowRight),
                              arrow);
        option.features &= ~QStyleOptionToolButton::Arrow;
        option.arrowType = Qt::NoArrow;
        option.rect = QStyle::visualRect(layoutDirection(), rect(), rect().adjusted(16, 0, 0, 0));
        painter.drawControl(QStyle::CE_ToolButtonLabel, option);
        if (hasFocus()) {
            QStyleOptionFocusRect focus;
            focus.initFrom(this);
            focus.rect = rect().adjusted(1, 1, -1, -1);
            painter.drawPrimitive(QStyle::PE_FrameFocusRect, focus);
        }
    }
};
} // namespace

QtDisclosure::QtDisclosure(QWidget *content, QWidget *parent)
    : QWidget(parent), content_(content), button_(new DisclosureButton(this)),
      summary_(new QLabel(this)) {
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(8);
    auto *separator = new QFrame(this);
    separator->setFrameShape(QFrame::HLine);
    separator->setFrameShadow(QFrame::Plain);
    separator->setForegroundRole(QPalette::Mid);
    layout->addWidget(separator);
    button_->setCheckable(true);
    button_->setAutoRaise(true);
    button_->setIconSize(QSize(10, 10));
    button_->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    button_->setArrowType(Qt::RightArrow);
    button_->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    button_->setFocusPolicy(Qt::StrongFocus);
    layout->addWidget(button_, 0, Qt::AlignLeading);
    summary_->setTextFormat(Qt::PlainText);
    summary_->setWordWrap(true);
    summary_->setObjectName("disclosureSummary");
    layout->addWidget(summary_);
    layout->addWidget(content_);
    content_->hide();
    summary_->hide();
    connect(button_, &QToolButton::toggled, this, [this](bool expanded) {
        button_->setArrowType(expanded ? Qt::DownArrow : Qt::RightArrow);
        content_->setVisible(expanded);
    });
}

void QtDisclosure::setExpanded(bool expanded) { button_->setChecked(expanded); }
bool QtDisclosure::isExpanded() const { return button_->isChecked(); }
void QtDisclosure::setSummary(const QString &text) {
    summary_->setText(text);
    summary_->setVisible(!text.isEmpty());
}
