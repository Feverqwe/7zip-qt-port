# Read encrypted ext files

The bundled dynamic engine can read fscrypt v1/v2 files in ext filesystem images
and through direct device access. This requires the filesystem's raw master key.
A login password, fscrypt protector password or LUKS password is not a raw key.
Sevenzip does not unwrap hardware keys or unlock LUKS containers.

When opening an encrypted ext image or device, Sevenzip shows **Open encrypted
filesystem**. Select the key version and paste the master key as hexadecimal
bytes. For v1, also enter its key descriptor. Spaces in pasted hexadecimal text
are accepted. **Add another key** supports filesystems using several keys;
**Show keys** makes the entered values visible. Invalid lengths or characters
are explained before opening.

Choose **Browse without keys** to view the accessible contents without
decrypting encrypted files, or Cancel to leave the current panel unchanged.
Keys are kept for the current opening and refresh; they are not saved in
settings. Reopen the filesystem to use different keys.

The bundled command-line engine accepts explicit key descriptions:

- `fscrypt:v2:<master-key-hex>` for a v2 key.
- `fscrypt:v1:<descriptor-hex>:<master-key-hex>` for a v1 key. The descriptor is
  the filesystem's eight-byte key descriptor, written as sixteen hex digits.
- `fscrypt:none` to browse the Linux no-key view without providing keys.

Replace the placeholders with hexadecimal bytes. Raw keys are 16–64 bytes;
the policy can require a larger minimum. For several keys, separate entries
with semicolons after one `fscrypt:` prefix, for example
`fscrypt:v2:<key-hex>;v1:<descriptor-hex>:<other-key-hex>`.

Without a matching key, encrypted names and symlink targets use Linux's opaque
no-key representation; encrypted regular files cannot be read. With a matching
key, Copy and Extract use the decrypted contents. fscrypt v1 does not authenticate
master keys; a readable result alone does not verify that a supplied v1 key is
correct.

On Windows, copying names containing `:` from filesystem images is currently
unavailable. Copying these files directly from a device remains available.

For the bundled command-line engine, the same text is the value of `-p`.
For example, listing without keys is:

```sh
7z l -tExt -pfscrypt:none -- volume.ext4
```

Keep the supplied format modules with the native codec runtime; they provide
the GPL fscrypt reader. The GUI does not require the console sidecar.
Direct device access retains its [read-only and authorization rules](direct-device-reading.md).
Validation uses synthetic Linux volumes; real encrypted external media have not
been checked.
