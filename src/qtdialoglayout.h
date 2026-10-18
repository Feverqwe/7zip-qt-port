#pragma once

class QDialogButtonBox;
class QVBoxLayout;
class QWidget;

// Shared presentation for redesigned dialogs; callers own fields and actions.
QVBoxLayout *QtCreateDialogLayout(QWidget &window, QDialogButtonBox &buttons);
