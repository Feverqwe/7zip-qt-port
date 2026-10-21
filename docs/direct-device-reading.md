# Read files directly from an external device

File → Open device provides experimental read-only access to
ext2/ext3/ext4 on an external disk. A filesystem driver and a full disk image
are not required. Ordinary mounted volumes and archive files remain available
through the existing file manager.

1. Connect the device. Its volumes must be unmounted for direct access.
2. Choose File → Open device and select the device by name and size.
3. Complete the system authorization request if one appears, then select a
   partition. A device with a single partition opens directly.
4. Browse folders in the current panel. Use Copy or Extract to copy files to
   another disk. The status bar shows Read only; commands that change the
   source are unavailable.

Refresh updates the device list; Back returns from partitions to devices.
Cancel leaves the current panel unchanged. After a denied authorization request
you can retry. If a device is busy, close files on it and unmount its volumes;
Sevenzip does not force an unmount. If it is disconnected, reconnect it and open
it again.

On macOS and Windows, retain the complete application package: its bundled
helper obtains read access when required. The macOS helper can be ad-hoc signed
without Developer ID and installs no permanent service. Linux requires UDisks2
and a working system authorization agent.

If macOS reports “Operation not permitted” after you enter the administrator
password, enable Sevenzip.app in System Settings → Privacy & Security → Full
Disk Access, then quit and reopen Sevenzip. Administrator authorization and
macOS privacy permissions are separate; entering the password alone may not
permit device access.

The first version accepts GPT, MBR and filesystems stored directly on a device.
Some ext features and layouts are unsupported. fscrypt files require
[explicit raw master keys](ext-encryption.md); the no-key view remains available.
When the reader can open a filesystem with read-only-compatible limitations or
unfinished recovery, Sevenzip lists those conditions and offers limited read-only
access. Continue only if you accept that some files may be missing, outdated or
unreadable. The status bar keeps a limited-access warning while you browse and
copy; the source device remains unchanged. Cancel leaves the current panel
unchanged. This choice applies to the current opening only and cannot provide
missing decoding support. Sevenzip does not replay the journal or clean up
orphaned files.
Sevenzip does not unlock LUKS containers or unwrap hardware fscrypt keys,
assemble LVM/RAID, repair filesystems or provide damaged-disk recovery. Direct
reading is not a consistent snapshot if another program changes the medium.
An unsupported-feature error lists the rejected features. `RECOVER` means the
filesystem needs journal recovery. Opening it on its usual Linux system and
unmounting it cleanly is an alternative to reading without journal replay;
that system may write journal changes to the disk. Sevenzip does not replay the journal.

Current automated checks use private filesystem samples on macOS, Linux and
Windows. Helper packaging and loading are checked separately. A user has reported
successful device authorization on macOS after enabling Full Disk Access;
successful browsing and copying from real external media remain unverified.
Detailed scope and platform evidence are in the [audit](upstream-port-audit/direct-devices.md).
