#pragma once
#include "platform/shell/qtcontextmenuservices.h"
#include <memory>

// Owned by each native COM object, including enumerated child commands.
std::unique_ptr<IQtContextMenuServices> CreateNativeContextMenuServices();
