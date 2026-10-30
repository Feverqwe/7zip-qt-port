#include "support/native_sheet.h"
#import <AppKit/AppKit.h>
#include <QWidget>

bool NativeSheetAttachedTo(QWidget &sheet, QWidget &parent) {
    NSView *view = reinterpret_cast<NSView *>(sheet.winId());
    NSView *parentView = reinterpret_cast<NSView *>(parent.winId());
    return view.window.sheetParent == parentView.window;
}
