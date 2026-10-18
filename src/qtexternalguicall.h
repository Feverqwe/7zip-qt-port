#pragma once
#include <functional>
class QWidget;
struct CExitEventLauncher;

// Worker-side SendMessage/MessageBox boundary, interruptible before delivery
// when the GUI synchronously joins its source external workers during close.
// A delivered callback exception returns to the waiting worker.
int QtExternalGuiCall(QWidget *window, CExitEventLauncher &launcher,
                      const std::function<int()> &body, const std::function<bool()> &ready = {});
