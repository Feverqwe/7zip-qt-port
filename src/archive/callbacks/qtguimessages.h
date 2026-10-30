#pragma once
#include "Common/MyTypes.h"
class QWidget;

// Qt presentation of MyMessages' source error/question policy.
void ShowErrorMessageQt(QWidget *parent, const wchar_t *message);
void ShowErrorMessageHwndResQt(QWidget *parent, UInt32 resID);
