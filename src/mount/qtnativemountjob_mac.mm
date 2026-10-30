#include "mount/qtnativemountjob.h"
#include <CoreFoundation/CoreFoundation.h>
#include <NetFS/NetFS.h>
#include <QCoreApplication>
#include <QFile>
#include <QFileInfo>
#include <QPointer>
#include <cerrno>
#include <cstring>
#include <dispatch/dispatch.h>
#include <sys/mount.h>
#include <thread>

namespace {
CFStringRef String(const QString &value) {
    return CFStringCreateWithCharacters(
        kCFAllocatorDefault, reinterpret_cast<const UniChar *>(value.utf16()), value.size());
}
QString Text(CFStringRef value) {
    if (!value)
        return {};
    const CFIndex length = CFStringGetLength(value);
    QString result(length, Qt::Uninitialized);
    CFStringGetCharacters(value, CFRangeMake(0, length),
                          reinterpret_cast<UniChar *>(result.data()));
    return result;
}
QString Error(int status) {
    return status > 0 ? QString::fromLocal8Bit(std::strerror(status))
                      : QStringLiteral("macOS network filesystem error %1").arg(status);
}
} // namespace

struct QtNativeMountJob::State {
    bool Busy = false, Mounted = false;
    QString Path;
    std::thread Worker;
    ~State() {
        if (Worker.joinable())
            Worker.join();
    }
};
QtNativeMountJob::QtNativeMountJob(QObject *parent)
    : QObject(parent), state_(std::make_unique<State>()) {}
QtNativeMountJob::~QtNativeMountJob() = default;
bool QtNativeMountJob::mounted() const { return state_->Mounted; }
bool QtNativeMountJob::busy() const { return state_->Busy; }

void QtNativeMountJob::mount(const QtNativeMountParameters &parameters, const QUrl &) {
    if (state_->Busy) {
        emit mountFinished({}, QStringLiteral("An OS mount operation is already running."));
        return;
    }
    const auto &directory = parameters.Directory;
    const QFileInfo targetInfo(directory);
    if (parameters.Owner != QtNativeClientOwnerIdentity() || !targetInfo.isDir() ||
        targetInfo.isSymLink() || targetInfo.canonicalFilePath() != directory) {
        emit mountFinished({}, tr("The private mount directory is unavailable."));
        return;
    }
    state_->Busy = true;
    const auto address =
        QStringLiteral("smb://127.0.0.1:%1/%2").arg(parameters.Port).arg(parameters.Share).toUtf8();
    const auto path = QFile::encodeName(directory);
    CFURLRef url = CFURLCreateWithBytes(kCFAllocatorDefault,
                                        reinterpret_cast<const UInt8 *>(address.constData()),
                                        address.size(), kCFStringEncodingUTF8, nullptr);
    CFURLRef target = CFURLCreateFromFileSystemRepresentation(
        kCFAllocatorDefault, reinterpret_cast<const UInt8 *>(path.constData()), path.size(), true);
    CFStringRef username = String(parameters.User), secret = String(parameters.Password);
    CFMutableDictionaryRef open = CFDictionaryCreateMutable(
        kCFAllocatorDefault, 0, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
    CFMutableDictionaryRef options = CFDictionaryCreateMutable(
        kCFAllocatorDefault, 0, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
    CFDictionarySetValue(open, kNetFSAllowLoopbackKey, kCFBooleanTrue);
    CFDictionarySetValue(open, kNetFSForceNewSessionKey, kCFBooleanTrue);
    CFDictionarySetValue(open, kNAUIOptionKey, kNAUIOptionNoUI);
    CFDictionarySetValue(options, kNetFSMountAtMountDirKey, kCFBooleanTrue);
    const int flags = MNT_RDONLY | MNT_DONTBROWSE | MNT_NOSUID | MNT_NODEV;
    CFNumberRef number = CFNumberCreate(kCFAllocatorDefault, kCFNumberIntType, &flags);
    CFDictionarySetValue(options, kNetFSMountFlagsKey, number);
    const QPointer<QtNativeMountJob> receiver(this);
    AsyncRequestID request = nullptr;
    const int status = NetFSMountURLAsync(
        url, target, username, secret, open, options, &request,
        dispatch_get_global_queue(QOS_CLASS_USER_INITIATED, 0),
        ^(int result, AsyncRequestID, CFArrayRef paths) {
          QString mounted;
          if (!result && paths && CFArrayGetCount(paths) == 1)
              mounted = Text(static_cast<CFStringRef>(CFArrayGetValueAtIndex(paths, 0)));
          QMetaObject::invokeMethod(QCoreApplication::instance(), [receiver, mounted, result] {
              if (!receiver)
                  return;
              receiver->state_->Busy = false;
              receiver->state_->Mounted = !result && !mounted.isEmpty();
              receiver->state_->Path = mounted;
              emit receiver->mountFinished(mounted, result ? Error(result) : QString());
          });
        });
    CFRelease(number);
    CFRelease(options);
    CFRelease(open);
    CFRelease(secret);
    CFRelease(username);
    CFRelease(target);
    CFRelease(url);
    if (status) {
        state_->Busy = false;
        emit mountFinished({}, Error(status));
    }
}

void QtNativeMountJob::unmount(bool force) {
    if (state_->Busy) {
        emit unmountFinished(QStringLiteral("An OS mount operation is already running."));
        return;
    }
    if (!state_->Mounted) {
        emit unmountFinished({});
        return;
    }
    if (state_->Worker.joinable())
        state_->Worker.join();
    state_->Busy = true;
    const auto path = QFile::encodeName(state_->Path);
    state_->Worker = std::thread([this, path, force] {
        const int status = ::unmount(path.constData(), force ? MNT_FORCE : 0) == 0 ? 0 : errno;
        QMetaObject::invokeMethod(this, [this, status] {
            state_->Busy = false;
            if (!status) {
                state_->Mounted = false;
                state_->Path.clear();
            }
            emit unmountFinished(status ? Error(status) : QString());
        });
    });
}
void QtNativeMountJob::recover(const QtNativeMountParameters &parameters, const QUrl &,
                               bool force) {
    const QString path = parameters.Directory;
    if (parameters.Owner != QtNativeClientOwnerIdentity() || path.isEmpty()) {
        emit unmountFinished(tr("The private mount directory is unavailable."));
        return;
    }
    struct statfs volume{};
    if (::statfs(QFile::encodeName(path).constData(), &volume) != 0) {
        emit unmountFinished(errno == ENOENT ? QString() : Error(errno));
        return;
    }
    if (QFile::decodeName(volume.f_mntonname) != path) {
        emit unmountFinished({}); // Already unmounted; leave its parent filesystem alone.
        return;
    }
    if (std::strcmp(volume.f_fstypename, "smbfs") ||
        !QString::fromUtf8(volume.f_mntfromname).endsWith(u'/' + parameters.Share)) {
        emit unmountFinished(tr("The mount belongs to another session and has been preserved."));
        return;
    }
    state_->Mounted = true;
    state_->Path = path;
    unmount(force);
}
