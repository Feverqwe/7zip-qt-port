#include "qtdialoglayout.h"
#include <QDialogButtonBox>
#include <QVBoxLayout>
#include <QWidget>

QVBoxLayout *QtCreateDialogLayout(QWidget &window, QDialogButtonBox &buttons) {
    auto *frame = new QVBoxLayout(&window);
    frame->setContentsMargins(16, 16, 16, 16);
    frame->setSpacing(16);
    auto *content = new QVBoxLayout;
    content->setSpacing(8);
    frame->addLayout(content, 1);
    frame->addWidget(&buttons);
    return content;
}
