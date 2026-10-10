#include "qtfilemanagersplitter.h"
#include <QResizeEvent>

QtFileManagerSplitter::QtFileManagerSplitter(QWidget *parent) : QSplitter(Qt::Horizontal, parent) {
    setHandleWidth(kSplitterWidth);
    setChildrenCollapsible(false);
    connect(this, &QSplitter::splitterMoved, this, [this](int position, int) {
        if (initialized_ && HasTwoPanels()) {
            splitter_.SetPos(width(), position);
            UpdateSourceLayout();
        }
    });
}

bool QtFileManagerSplitter::HasTwoPanels() const {
    return count() == 2 && !widget(0)->isHidden() && !widget(1)->isHidden();
}

void QtFileManagerSplitter::Initialize(bool panelsDefined, unsigned splitterPos) {
    if (panelsDefined)
        splitter_.SetPos(width(), static_cast<int>(splitterPos));
    else
        splitter_.SetRatio(width(), kSplitterRateMax / 2);
    initialized_ = true;
    UpdateSourceLayout();
}

void QtFileManagerSplitter::UpdateSourceLayout() {
    if (!initialized_)
        return;
    splitter_.SetPosFromRatio(width());
    const int position = splitter_.GetPos();
    const bool twoPanels = HasTwoPanels();
    const bool changed =
        appliedWidth_ != width() || appliedPosition_ != position || appliedTwoPanels_ != twoPanels;
    appliedWidth_ = width();
    appliedPosition_ = position;
    appliedTwoPanels_ = twoPanels;
    if (twoPanels && changed)
        setSizes({position, width() - handleWidth() - position});
}

void QtFileManagerSplitter::RememberPosition() {
    if (initialized_ && HasTwoPanels() && sizes().value(0) != splitter_.GetPos())
        splitter_.SetPos(width(), sizes().value(0));
}

int QtFileManagerSplitter::SourcePosition() const {
    return HasTwoPanels() ? sizes().value(0) : splitter_.GetPos();
}

void QtFileManagerSplitter::resizeEvent(QResizeEvent *event) {
    QSplitter::resizeEvent(event);
    UpdateSourceLayout();
}
