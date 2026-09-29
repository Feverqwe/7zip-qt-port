// GUI-thread presentation boundary for upstream worker password callbacks.
#pragma once
#include "Common/MyString.h"
class CProgressDialog;
bool AskCallbackPassword(CProgressDialog &parent, UString &password);
bool AskCallbackPassword(CProgressDialog &parent, UString &password, bool &showPassword);
