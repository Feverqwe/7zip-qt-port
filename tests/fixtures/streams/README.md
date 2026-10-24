# Portable WIM ADS fixture

`unicode-ads.wim` is synthetic test data generated on Windows NTFS with the
official 7-Zip 26.03 runtime from upstream commit
`0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17` (https://github.com/ip7z/7zip.git).
It contains `файл δ with space.bin` (UTF-8 bytes `main bytes`) and its alternate
stream `поток δ` (UTF-8 bytes `archive stream bytes`). It contains no user data.
SHA-256: `6d8b8571b5503251a044c5e29e134d582357e99e144e106f92da8d9b9e982b69`.

Generate in a disposable NTFS directory using Windows PowerShell:

```powershell
$name = 'файл δ with space.bin'
[IO.File]::WriteAllBytes($name, [Text.Encoding]::UTF8.GetBytes('main bytes'))
Set-Content -LiteralPath $name -Stream 'поток δ' -Encoding Byte `
    -Value ([Text.Encoding]::UTF8.GetBytes('archive stream bytes'))
7z.exe a -twim -sns -- unicode-ads.wim $name
```

Use a new output file: timestamps make regenerated archive hashes differ.
NT security storage is off by default; do not pass `-sni`. The source command
parser accepts `sni` only as a positive switch, and `l -slt` confirms empty
`NT Security` for this fixture. `-sni-` is not a supported disabling switch.
Tests copy this fixture into temporary directories on macOS/Linux to exercise
real archive streams without pretending those filesystems provide Windows ADS.
Windows additionally creates a fresh fixture on the actual NTFS filesystem.
