#pragma once
#include <memory>
class IQtMenuPageServices;
std::shared_ptr<IQtMenuPageServices> CreateNativeMenuPageServices(bool commandsAvailable = false);
