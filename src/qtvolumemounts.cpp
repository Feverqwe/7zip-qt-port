#include "qtvolumemounts.h"
#include <QStorageInfo>

QList<QtVolumeMount> QtSystemVolumeMounts() {
    QList<QtVolumeMount> result;
    QList<QStorageInfo> volumes = QStorageInfo::mountedVolumes();
    volumes.prepend(QStorageInfo::root());
    for (const QStorageInfo &volume : volumes) {
        if (!volume.isValid() || !volume.isReady())
            continue;
        const QString path = volume.rootPath();
        if (path.isEmpty())
            continue;
        bool duplicate = false;
        for (const QtVolumeMount &existing : result)
            duplicate |= existing.path == path;
        if (duplicate)
            continue;
        QtVolumeMount mount;
        mount.path = path;
        mount.name = volume.displayName().isEmpty() ? path : volume.displayName();
        mount.fileSystem = QString::fromUtf8(volume.fileSystemType());
        const QString type = mount.fileSystem.toLower();
        mount.network = type == "smbfs" || type == "cifs" || type == "nfs" || type == "nfs4" ||
                        type == "afpfs" || type == "sshfs" || type == "davfs" ||
                        type == "fuse.sshfs" || type == "fuse.smb";
        mount.totalBytes = volume.bytesTotal();
        mount.availableBytes = volume.bytesAvailable();
        result.append(mount);
    }
    bool hasRoot = false;
    for (const QtVolumeMount &mount : result)
        hasRoot |= mount.path == QStringLiteral("/");
    if (!hasRoot)
        result.prepend({QStringLiteral("/"), QStringLiteral("/"), {}});
    return result;
}
