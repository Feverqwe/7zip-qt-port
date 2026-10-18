#include "qtdialoglayout.h"
#include <QPushButton>
#include <QResizeEvent>
#include <QVBoxLayout>
#include <QWidget>

QVBoxLayout *QtCreateDialogLayout(QWidget &window, QWidget &footer) {
    auto *frame = new QVBoxLayout(&window);
    frame->setContentsMargins(16, 16, 16, 16);
    frame->setSpacing(16);
    auto *content = new QVBoxLayout;
    content->setSpacing(8);
    frame->addLayout(content, 1);
    frame->addWidget(&footer);
    return content;
}

void QtSizeIconButton(QPushButton &button) {
    const QSize native = button.sizeHint();
    button.setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    button.setFixedWidth(qMax(native.width(), native.height()));
}

QSize QtWrappingLabel::minimumSizeHint() const {
    QSize hint = QLabel::minimumSizeHint();
    if (wordWrap() && testAttribute(Qt::WA_Resized))
        hint.setHeight(qMax(hint.height(), heightForWidth(width())));
    return hint;
}

void QtWrappingLabel::resizeEvent(QResizeEvent *event) {
    QLabel::resizeEvent(event);
    if (event->oldSize().width() != event->size().width())
        updateGeometry();
}
