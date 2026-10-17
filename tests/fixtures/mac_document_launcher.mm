#import <AppKit/AppKit.h>
#include <cstdio>

int main(int argc, const char **argv) {
    @autoreleasepool {
        if (argc != 5)
            return 2;
        [NSApplication sharedApplication];
        NSURL *application = [NSURL fileURLWithPath:[NSString stringWithUTF8String:argv[1]]];
        NSString *identifier = [NSBundle bundleWithURL:application].bundleIdentifier;
        if (![identifier hasPrefix:@"org.sevenzip-desktop.fixture."])
            return 2;
        NSString *request = [NSString stringWithUTF8String:argv[2]];
        NSString *report = [NSString stringWithUTF8String:argv[3]];
        NSWorkspaceOpenConfiguration *configuration = [NSWorkspaceOpenConfiguration configuration];
        configuration.createsNewApplicationInstance = YES;
        NSMutableDictionary *environment = [NSProcessInfo.processInfo.environment mutableCopy];
        environment[@"QT_QPA_PLATFORM"] = @"cocoa";
        environment[@"SEVENZIP_NATIVE_STARTUP_REPORT"] = report;
        environment[@"SEVENZIP_NATIVE_STARTUP_MEMBER"] = [NSString stringWithUTF8String:argv[4]];
        environment[@"SEVENZIP_SETTINGS_PATH"] = [report stringByAppendingString:@".ini"];
        configuration.environment = environment;
        __block NSRunningApplication *child = nil;
        void (^completed)(NSRunningApplication *, NSError *) = ^(NSRunningApplication *running,
                                                                 NSError *error) {
          dispatch_async(dispatch_get_main_queue(), ^{
            if (error || !running) {
                std::fprintf(stderr, "Native launch failed: %s\n", error.description.UTF8String);
                exit(3);
            }
            child = [running retain];
          });
        };
        if ([request isEqualToString:@"--normal"])
            [[NSWorkspace sharedWorkspace] openApplicationAtURL:application
                                                  configuration:configuration
                                              completionHandler:completed];
        else
            [[NSWorkspace sharedWorkspace] openURLs:@[ [NSURL fileURLWithPath:request] ]
                               withApplicationAtURL:application
                                      configuration:configuration
                                  completionHandler:completed];
        dispatch_source_t poll =
            dispatch_source_create(DISPATCH_SOURCE_TYPE_TIMER, 0, 0, dispatch_get_main_queue());
        dispatch_source_set_timer(poll, DISPATCH_TIME_NOW, 50 * NSEC_PER_MSEC, 0);
        dispatch_source_set_event_handler(poll, ^{
          if (child.terminated)
              exit([NSFileManager.defaultManager fileExistsAtPath:report] ? 0 : 4);
        });
        dispatch_resume(poll);
        dispatch_after(
            dispatch_time(DISPATCH_TIME_NOW, 20 * NSEC_PER_SEC), dispatch_get_main_queue(), ^{
              // Only the unique fixture application created by this launcher.
              for (NSRunningApplication *running in NSWorkspace.sharedWorkspace.runningApplications)
                  if ([running.bundleIdentifier isEqualToString:identifier])
                      [running forceTerminate];
              exit(5);
            });
        [NSApp run];
        return 6;
    }
}
