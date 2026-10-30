#include "platform/associations/qtfileassociationprobe.h"
#import <AppKit/AppKit.h>

bool QtFileHasOpenAssociation(const QString &path) {
    NSURL *file = [NSURL fileURLWithPath:[NSString stringWithUTF8String:path.toUtf8().constData()]];
    return [[NSWorkspace sharedWorkspace] URLForApplicationToOpenURL:file] != nil;
}
