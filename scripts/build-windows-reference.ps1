# Build the pinned native 7zFM for rendered UI comparisons in the disposable guest.
[CmdletBinding()]
param([string]$UpstreamCheckout = 'upstream')
$ErrorActionPreference = 'Stop'
$compilerDirectory = [IO.Path]::GetDirectoryName((Get-Command cl.exe -ErrorAction Stop).Source)
$env:PATH = "$compilerDirectory;$env:PATH"
if ($env:OS -ne 'Windows_NT' -or $env:COMPUTERNAME -ne 'RUNNER-WINDOWS' -or
    $env:VSCMD_ARG_TGT_ARCH -ne 'x64') {
    throw 'Use the authorized x64 Windows runner developer environment.'
}
$project = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$baseline = Get-Content -LiteralPath (Join-Path $project 'docs/upstream-baseline.json') -Raw |
    ConvertFrom-Json
$revision = [string]$baseline.revision
if ($revision -notmatch '^[0-9a-f]{40}$') { throw 'Invalid pinned revision.' }
$upstream = [IO.Path]::GetFullPath($UpstreamCheckout)
$output = Join-Path $project 'build/windows-reference'
if (Test-Path -LiteralPath $output) {
    throw 'Reference output already exists; use a fresh runner snapshot.'
}
$null = New-Item -ItemType Directory -Path $output
$snapshot = Join-Path $output 'source'
$null = New-Item -ItemType Directory -Path $snapshot
& git -C $upstream archive "--output=$output/source.tar" $revision
if ($LASTEXITCODE -ne 0) { throw 'Cannot archive pinned source.' }
& (Join-Path $env:WINDIR 'System32/tar.exe') -xf "$output/source.tar" -C $snapshot
if ($LASTEXITCODE -ne 0) { throw 'Cannot extract pinned source.' }
$module = Join-Path $snapshot 'CPP/7zip/UI/FileManager'
$original = Join-Path $module 'FM.cpp'
$originalHash = (Get-FileHash -LiteralPath $original -Algorithm SHA256).Hash.ToLowerInvariant()
Move-Item -LiteralPath $original -Destination (Join-Path $module 'FM.original.cpp')
$adapter = Join-Path $project 'tests/fixtures/native_reference_entry.cpp'
Copy-Item -LiteralPath $adapter -Destination $original
$privateName = 'SevenzipDesktopReference-' + [Guid]::NewGuid().ToString('N')
$registryKey = 'Software\' + $privateName
$header = 'static const wchar_t *const kReferenceRegistryPath = L"Software\\' + $privateName + '";' + "`n"
[IO.File]::WriteAllText((Join-Path $module 'ReferenceKey.h'), $header)
$registrySources = [ordered]@{}
foreach ($entry in @(
    @{ File = 'RegistryUtils.cpp'; Macro = 'REG_PATH_7Z'; Tail = '' },
    @{ File = 'ViewSettings.cpp'; Macro = 'REG_PATH_FM'; Tail = ' TEXT(STRING_PATH_SEPARATOR) TEXT("FM")' }
)) {
    $path = Join-Path $module $entry.File
    $text = [IO.File]::ReadAllText($path)
    $before = '#define ' + $entry.Macro + ' TEXT("Software") TEXT(STRING_PATH_SEPARATOR) TEXT("7-Zip")' + $entry.Tail
    if (($text.Split(@($before), [StringSplitOptions]::None)).Count -ne 2) {
        throw "Expected exactly one source registry path in $($entry.File)."
    }
    $beforeHash = (Get-FileHash -LiteralPath $path -Algorithm SHA256).Hash.ToLowerInvariant()
    $after = '#define ' + $entry.Macro + ' TEXT("Software") TEXT(STRING_PATH_SEPARATOR) TEXT("' + $privateName + '")' + $entry.Tail
    [IO.File]::WriteAllText($path, $text.Replace($before, $after))
    $registrySources[$entry.File] = [ordered]@{
        originalSha256 = $beforeHash
        adaptedSha256 = (Get-FileHash -LiteralPath $path -Algorithm SHA256).Hash.ToLowerInvariant()
        adaptation = 'One validated registry base-path macro; all functional bodies unchanged.'
    }
}
Push-Location -LiteralPath $module
try {
    & nmake /nologo /f makefile PLATFORM=x64
    if ($LASTEXITCODE -ne 0) { throw 'Native reference build failed.' }
} finally { Pop-Location }
Copy-Item -LiteralPath (Join-Path $module 'x64/7zFM.exe') -Destination $output
foreach ($binary in @('7z.dll', '7z.exe')) {
    Copy-Item -LiteralPath (Join-Path $project "build/codec-runtime/$binary") -Destination $output
}
Copy-Item -LiteralPath (Join-Path $snapshot 'DOC/License.txt') -Destination (Join-Path $output 'LICENSE.txt')
[ordered]@{
    schema = 1
    producer = 'sevenzip-desktop/build-windows-reference'
    repository = [string]$baseline.repository
    revision = $revision
    source = 'CPP/7zip/UI/FileManager/makefile'
    entrySource = 'CPP/7zip/UI/FileManager/FM.cpp'
    entrySha256 = $originalHash
    adapter = 'tests/fixtures/native_reference_entry.cpp'
    adapterSha256 = (Get-FileHash -LiteralPath $adapter -Algorithm SHA256).Hash.ToLowerInvariant()
    registryKey = $registryKey
    registrySources = $registrySources
    binarySha256 = (Get-FileHash -LiteralPath (Join-Path $output '7zFM.exe') -Algorithm SHA256).Hash.ToLowerInvariant()
    system = 'Windows'
    architecture = 'x86_64'
    adaptation = 'Entry name alias and two private product registry path macros; original functional bodies/resources/menu handlers unchanged. Shell provider HKCU remains native.'
} | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $output 'reference.json') -Encoding UTF8
Write-Output "Native rendered reference: $output/7zFM.exe"
