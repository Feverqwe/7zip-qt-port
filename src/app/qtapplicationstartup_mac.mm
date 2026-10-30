#include "app/qtapplicationstartup.h"
#import <AppKit/AppKit.h>
#include <QEventLoop>
#include <QGuiApplication>

bool QtWaitForMacApplicationLaunch() {
    if (QGuiApplication::platformName() != QStringLiteral("cocoa") ||
        NSRunningApplication.currentApplication.finishedLaunching)
        return true;

    QEventLoop launch;
    QEventLoop *waiting = &launch;
    __block bool finished = false;
    NSNotificationCenter *center = NSNotificationCenter.defaultCenter;
    id observer = [center addObserverForName:NSApplicationDidFinishLaunchingNotification
                                      object:NSApp
                                       queue:nil
                                  usingBlock:^(NSNotification *) {
                                    finished = true;
                                    waiting->quit();
                                  }];
    // Initial openFiles precedes didFinishLaunching. A manual event flush may
    // return between willFinishLaunching and that document delivery.
    if (!NSRunningApplication.currentApplication.finishedLaunching)
        launch.exec();
    else
        finished = true;
    [center removeObserver:observer];
    return finished;
}
