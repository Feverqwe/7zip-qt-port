#include "dock_observer.h"
#import <AppKit/AppKit.h>

bool EnsureCocoaDock() { return [NSApplication sharedApplication].dockTile != nil; }
QString NativeDockBadge() { return QString::fromUtf8(NSApp.dockTile.badgeLabel.UTF8String); }
void SetNativeDockBadge(const QString &text) {
    const QByteArray utf8 = text.toUtf8();
    NSString *label = [[NSString alloc] initWithBytes:utf8.constData()
                                               length:std::size_t(utf8.size())
                                             encoding:NSUTF8StringEncoding];
    NSApp.dockTile.badgeLabel = label;
    [label release];
}
