#include "qtnativepanelkeys.h"

#include <QKeyEvent>
#include <QtGui/private/qkeymapper_p.h>
#include <type_traits>

namespace {
template <typename T> QKeyCombination keyCombination(const T &candidate) {
    if constexpr (std::is_integral_v<T>)
        return QKeyCombination::fromCombined(candidate);
    else
        return candidate;
}
} // namespace

int QtPanelVirtualKey(const QKeyEvent &key) {
    const int logical = key.key();
    if (key.modifiers().testFlag(Qt::KeypadModifier))
        return logical >= Qt::Key_0 && logical <= Qt::Key_9 ? Qt::Key_unknown : logical;
#ifdef Q_OS_WIN
    // NMLVKEYDOWN retains virtual digits when Shift translates them to symbols.
    const auto native = key.nativeVirtualKey();
    if (native >= '0' && native <= '9')
        return static_cast<int>(native);
#endif
    if (!(key.modifiers() & (Qt::ControlModifier | Qt::AltModifier)) ||
        (!key.nativeScanCode() && !key.nativeVirtualKey()))
        return logical;
    // QPA already resolves native keys using the current keyboard layout.
    for (const auto &candidate : QKeyMapper::possibleKeys(&key)) {
        const int native = keyCombination(candidate).key();
        if (native >= Qt::Key_0 && native <= Qt::Key_9)
            return native;
    }
    return logical;
}
