#include "filemanager/qtfilemenuprojection.h"
#include "fileops/native/Dialog.h"

bool QtFileMenuProjection::IsEnabled(unsigned id) const {
    for (const Item &item : items)
        if (!item.source.IsSeparator() && item.source.wID == id)
            return item.enabled;
    return false;
}
void QtFileMenuProjection::ReadRegDiff(UString &path) const { path = diffPath; }
void QtFileMenuProjection::ReadReg_VerCtrlPath(UString &path) const { path = versionControlPath; }
bool QtFileMenuProjection::IsDialogSizeOK(unsigned width, unsigned height) const {
    return screenServices ? NWindows::NControl::IsDialogSizeOK(width, height, *screenServices)
                          : normalScreen;
}
bool QtFileMenuProjection::GetItem(unsigned index, QtFileMenuItem &item) const {
    if (index >= static_cast<unsigned>(sourceItems.size()))
        return false;
    item = sourceItems[index];
    return true;
}
void QtFileMenuProjection::CopyPopMenu_IfRequired(QtFileMenuItem &) {
    // Source IDs already identify the CRC leaves; Qt owns no HMENU handles.
}
bool QtFileMenuProjection::InsertItem(unsigned position, const QtFileMenuItem &item) {
    if (position > static_cast<unsigned>(items.size()))
        return false;
    items.insert(position, {item, true});
    return true;
}
void QtFileMenuProjection::EnableItem(unsigned position, bool enabled) {
    if (position < static_cast<unsigned>(items.size()))
        items[position].enabled = enabled;
}
bool QtFileMenuProjection::AppendItem(unsigned id, const UString &text) {
    items.append({{id, text, false}, true});
    return true;
}
void QtFileMenuProjection::RemoveAllItemsFrom(unsigned position) {
    if (position < static_cast<unsigned>(items.size()))
        items.erase(items.begin() + position, items.end());
}
