# Read files directly from an external device

File → Open device provides experimental read-only access to unencrypted
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

The first version accepts GPT, MBR and filesystems stored directly on a device.
Some ext features and layouts are unsupported. Encrypted filesystems and those
requiring journal recovery are refused. Sevenzip does not unlock LUKS/fscrypt,
assemble LVM/RAID, repair filesystems or provide damaged-disk recovery. Direct
reading is not a consistent snapshot if another program changes the medium.

Current automated checks use private filesystem samples on macOS, Linux and
Windows. Helper packaging and loading are checked separately; successful access
to real external media and actual system authorization prompts remain unverified.
Detailed scope and platform evidence are in the [audit](upstream-port-audit/direct-devices.md).
