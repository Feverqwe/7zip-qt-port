// GUI-thread boundary for copied extraction callback modal bodies.
#pragma once
#include "Common/MyWindows.h"
#include <functional>
class CProgressDialog;
HRESULT RunCallbackDialog(CProgressDialog &parent, const std::function<HRESULT()> &body);
