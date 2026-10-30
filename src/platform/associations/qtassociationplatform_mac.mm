#include "platform/associations/qtmacassociationnative.h"
#import <AppKit/AppKit.h>
#include <QEventLoop>
#import <UniformTypeIdentifiers/UniformTypeIdentifiers.h>
#include <atomic>
#include <memory>

namespace {
NSString *native(const QString &value) {
    return [NSString stringWithUTF8String:value.toUtf8().constData()];
}
QString qt(NSString *value) { return value ? QString::fromUtf8(value.UTF8String) : QString(); }
} // namespace
QString QtMacOwnBundlePath() { return qt([NSBundle mainBundle].bundlePath); }
QString QtMacOwnBundleIdentifier() { return qt([NSBundle mainBundle].bundleIdentifier); }
QString QtMacCurrentUserName() { return qt(NSUserName()); }
bool QtMacCanSetAssociation() {
    if (@available(macOS 12.0, *))
        return [NSBundle mainBundle].bundleIdentifier != nil &&
               [[[NSBundle mainBundle].bundleURL pathExtension] isEqualToString:@"app"] &&
               [[NSBundle mainBundle] objectForInfoDictionaryKey:@"CFBundleDocumentTypes"] != nil;
    return false;
}
long QtMacReadAssociation(const QString &extension, QtMacAssociationState &state) {
    if (@available(macOS 12.0, *)) {
        UTType *type = [UTType typeWithFilenameExtension:native(extension)];
        if (!type || type.dynamic)
            return 50;
        NSURL *url = [[NSWorkspace sharedWorkspace] URLForApplicationToOpenContentType:type];
        state.type = qt(type.identifier);
        state.bundle = qt(url ? [NSBundle bundleWithURL:url].bundleIdentifier : nil);
        state.path = qt(url.path);
        return 0;
    }
    return 50;
}
long QtMacSetAssociation(const QString &extension, const QString &applicationPath) {
    if (@available(macOS 12.0, *)) {
        UTType *type = [UTType typeWithFilenameExtension:native(extension)];
        if (!type || type.dynamic)
            return 50;
        NSURL *url = [NSURL fileURLWithPath:native(applicationPath)];
        // AppKit can release the captured owner after the GUI caller has returned.
        // Always destroy the QObject in its owning thread.
        auto loop = std::shared_ptr<QEventLoop>(new QEventLoop,
                                                [](QEventLoop *value) { value->deleteLater(); });
        auto result = std::make_shared<std::atomic<long>>(31);
        auto completed = std::make_shared<std::atomic<bool>>(false);
        [[NSWorkspace sharedWorkspace]
            setDefaultApplicationAtURL:url
                     toOpenContentType:type
                     completionHandler:^(NSError *error) {
                       result->store(error ? 5 : 0);
                       completed->store(true);
                       QMetaObject::invokeMethod(loop.get(), &QEventLoop::quit,
                                                 Qt::QueuedConnection);
                     }];
        if (!completed->load())
            loop->exec();
        return result->load();
    }
    return 50;
}
