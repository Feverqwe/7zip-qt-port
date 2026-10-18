#pragma once
#include "7zip/UI/FileManager/ProgressDialog2Res.h"
#include <QAbstractButton>
#include <QVariant>

namespace TestProgressDialog {
inline bool IsCloseButton(const QAbstractButton *button) {
    return button && button->property("languageID").toUInt() == IDS_CLOSE;
}
} // namespace TestProgressDialog
