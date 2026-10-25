#include "native_media_consumer.h"
#include <AVFoundation/AVFoundation.h>
#include <PDFKit/PDFKit.h>
#include <QDir>
#include <QElapsedTimer>
#include <cmath>

namespace {
template <typename Predicate> bool Wait(Predicate ready) {
    QElapsedTimer timer;
    timer.start();
    while (!ready() && timer.elapsed() < 15000)
        [[NSRunLoop currentRunLoop] runMode:NSDefaultRunLoopMode
                                 beforeDate:[NSDate dateWithTimeIntervalSinceNow:0.02]];
    return ready();
}
NSURL *File(const QString &directory, const QString &name) {
    const auto path = QDir(directory).filePath(name).toUtf8();
    return [NSURL fileURLWithPath:[NSString stringWithUTF8String:path.constData()]];
}
} // namespace

bool ReadNativeMedia(const QString &directory, QString &error) {
    @autoreleasepool {
        PDFDocument *pdf = [[PDFDocument alloc] initWithURL:File(directory, "sample.pdf")];
        if (pdf.pageCount != 1 || ![pdf.string containsString:@"PDF through native TAR reader"]) {
            error = QStringLiteral("PDFKit could not read the mounted PDF page and text.");
            return false;
        }
        AVPlayer *player = [AVPlayer playerWithURL:File(directory, "video with spaces.mp4")];
        player.automaticallyWaitsToMinimizeStalling = NO;
        AVPlayerItemVideoOutput *output =
            [[AVPlayerItemVideoOutput alloc] initWithPixelBufferAttributes:@{
                (id)kCVPixelBufferPixelFormatTypeKey : @(kCVPixelFormatType_32BGRA)
            }];
        [player.currentItem addOutput:output];
        if (!Wait([&] { return player.currentItem.status != AVPlayerItemStatusUnknown; }) ||
            player.currentItem.status != AVPlayerItemStatusReadyToPlay) {
            error = QStringLiteral("AVPlayer could not open the mounted video: ") +
                    QString::fromUtf8(player.currentItem.error.localizedDescription.UTF8String);
            return false;
        }
        QByteArray previous;
        bool success = true;
        for (const int seconds : {0, 7, 2}) {
            [player seekToTime:CMTimeMake(seconds, 1)
                toleranceBefore:kCMTimeZero
                 toleranceAfter:kCMTimeZero];
            if (!Wait([&] {
                    return std::abs(CMTimeGetSeconds(player.currentTime) - seconds) < 0.2;
                })) {
                error = QStringLiteral("AVPlayer did not complete forward/backward seek.");
                success = false;
                break;
            }
            [player play];
            if (!Wait([&] { return CMTimeGetSeconds(player.currentTime) > seconds + 0.3; })) {
                error = QStringLiteral("AVPlayer playback did not advance after seeking.");
                success = false;
                break;
            }
            CVPixelBufferRef frame = nullptr;
            const bool decoded = Wait([&] {
                if (!frame) {
                    CMTime displayed = kCMTimeInvalid;
                    CVPixelBufferRef candidate =
                        [output copyPixelBufferForItemTime:player.currentTime
                                        itemTimeForDisplay:&displayed];
                    if (candidate && CMTIME_IS_NUMERIC(displayed) &&
                        std::abs(CMTimeGetSeconds(displayed) -
                                 CMTimeGetSeconds(player.currentTime)) < 0.3)
                        frame = candidate;
                    else if (candidate)
                        CVPixelBufferRelease(candidate);
                }
                return frame != nullptr;
            });
            [player pause];
            if (!decoded || CVPixelBufferGetWidth(frame) != 320 ||
                CVPixelBufferGetHeight(frame) != 180) {
                if (frame)
                    CVPixelBufferRelease(frame);
                error = QStringLiteral("AVPlayer did not decode the expected video frame.");
                success = false;
                break;
            }
            CVPixelBufferLockBaseAddress(frame, kCVPixelBufferLock_ReadOnly);
            const QByteArray pixels(
                static_cast<const char *>(CVPixelBufferGetBaseAddress(frame)),
                qsizetype(CVPixelBufferGetBytesPerRow(frame) * CVPixelBufferGetHeight(frame)));
            CVPixelBufferUnlockBaseAddress(frame, kCVPixelBufferLock_ReadOnly);
            CVPixelBufferRelease(frame);
            if (!previous.isEmpty() && previous == pixels) {
                error = QStringLiteral("Video frame did not change after seeking.");
                success = false;
                break;
            }
            previous = pixels;
        }
        [player pause];
        [player.currentItem removeOutput:output];
        [player replaceCurrentItemWithPlayerItem:nil];
        return success;
    }
}
