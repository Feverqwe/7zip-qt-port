#include "qtappoperationstate.h"

bool QtAppOperationState::Begin(Kind kind) {
    if (closed_)
        return false;
    if (kind == Kind::Idle) {
        End();
        return true;
    }
    if (!active()) {
        // Build transactionally: allocation failure restores every already
        // acquired source guard and cannot leave a half-disabled app owner.
        std::array<std::unique_ptr<PanelGuards>, 2> guards;
        for (unsigned i = 0; i < guards.size(); ++i)
            guards[i] = std::make_unique<PanelGuards>(panels_[i]);
        guards_ = std::move(guards);
    }
    kind_ = kind;
    return true;
}

void QtAppOperationState::End() {
    for (auto &guard : guards_)
        guard.reset();
    kind_ = Kind::Idle;
}

void QtAppOperationState::Close() {
    // Restore retained operation guards BEFORE ReleaseApp disables all panels.
    // No guard may later re-enable notifications on an already released owner.
    End();
    closed_ = true;
}

void QtAppOperationState::ResetForCreate() {
    End();
    closed_ = false;
}
