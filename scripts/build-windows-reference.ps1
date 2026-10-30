# Build/reuse the pinned native 7zFM in the persistent guest worktree.
[CmdletBinding()]
param([string]$UpstreamCheckout = 'upstream')
$ErrorActionPreference = 'Stop'
$project = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
. (Join-Path $PSScriptRoot 'lib/workspace.ps1')
Initialize-SevenzipWorkspace $project
$python = if ($env:SEVENZIP_RUNNER_PYTHON) { $env:SEVENZIP_RUNNER_PYTHON } else { 'python' }
$expectedHost = $env:SEVENZIP_WINDOWS_RUNNER_HOST
if (-not $expectedHost) {
    $expectedHost = & $python (Join-Path $PSScriptRoot 'lib/runner_config.py') windows-host
    if ($LASTEXITCODE -ne 0) { throw 'Configure the Windows test host in runners.local.json.' }
}
$compilerDirectory = [IO.Path]::GetDirectoryName((Get-Command cl.exe -ErrorAction Stop).Source)
$env:PATH = "$compilerDirectory;$env:PATH"
if ($env:OS -ne 'Windows_NT' -or $env:COMPUTERNAME -ne $expectedHost -or
    $env:VSCMD_ARG_TGT_ARCH -ne 'x64') {
    throw 'Use the authorized x64 Windows runner developer environment.'
}
$baseline = Get-Content -LiteralPath (Join-Path $project 'docs/upstream-baseline.json') -Raw |
    ConvertFrom-Json
$revision = [string]$baseline.revision
if ($revision -notmatch '^[0-9a-f]{40}$') { throw 'Invalid pinned revision.' }
$upstream = [IO.Path]::GetFullPath($UpstreamCheckout)
$output = Join-Path $env:SEVENZIP_ARTIFACT_ROOT 'windows-reference'
$manifest = Join-Path $output 'reference.json'
$owner = $null
if (Test-Path -LiteralPath $output) {
    if ((Get-Item -LiteralPath $output).Attributes -band [IO.FileAttributes]::ReparsePoint) {
        throw 'Refusing a reparse-point reference output.'
    }
    if (Get-ChildItem -LiteralPath $output -Force | Select-Object -First 1) {
        if (-not (Test-Path -LiteralPath $manifest -PathType Leaf)) {
            throw 'Refusing an unowned reference output.'
        }
        $owner = Get-Content -LiteralPath $manifest -Raw | ConvertFrom-Json
        if ($owner.producer -ne 'sevenzip-desktop/build-windows-reference') {
            throw 'Refusing an unowned reference output.'
        }
    }
}
$null = New-Item -ItemType Directory -Path $output -Force
$adapter = Join-Path $project 'tests/support/native_reference_entry.cpp'
$signature = [ordered]@{
    revision = $revision
    repository = [string]$baseline.repository
    builderSha256 = (Get-FileHash -LiteralPath $PSCommandPath -Algorithm SHA256).Hash
    snapshotBuilderSha256 = (Get-FileHash -LiteralPath (Join-Path $PSScriptRoot 'upstream_baseline.py') -Algorithm SHA256).Hash
    adapterSha256 = (Get-FileHash -LiteralPath $adapter -Algorithm SHA256).Hash
    compilerSha256 = (Get-FileHash -LiteralPath (Join-Path $compilerDirectory 'cl.exe') -Algorithm SHA256).Hash
    compilerEnvironment = @($env:CL, $env:_CL_, $env:INCLUDE, $env:LIB,
        $env:VCToolsVersion, $env:WindowsSDKVersion) -join ';'
} | ConvertTo-Json -Compress
foreach ($name in @('7zFM.exe', '7z.dll', '7z.exe', 'reference.json', 'LICENSE.txt')) {
    $path = Join-Path $output $name
    if ((Test-Path -LiteralPath $path) -and
        ((Get-Item -LiteralPath $path).Attributes -band [IO.FileAttributes]::ReparsePoint)) {
        throw 'Refusing a reparse-point reference artifact.'
    }
}
function Copy-Changed([string]$From, [string]$To) {
    if (-not (Test-Path -LiteralPath $To) -or
        (Get-FileHash -LiteralPath $From -Algorithm SHA256).Hash -ne
        (Get-FileHash -LiteralPath $To -Algorithm SHA256).Hash) {
        Copy-Item -LiteralPath $From -Destination $To -Force
    }
}
function Copy-Runtime {
    foreach ($binary in @('7z.dll', '7z.exe')) {
        Copy-Changed (Join-Path $env:SEVENZIP_ARTIFACT_ROOT "dev/codec-runtime/$binary") (Join-Path $output $binary)
    }
}
if ($owner -and $owner.inputSignature -eq $signature -and
    (Test-Path -LiteralPath (Join-Path $output 'LICENSE.txt') -PathType Leaf) -and
    (Get-FileHash -LiteralPath (Join-Path $output 'LICENSE.txt') -Algorithm SHA256).Hash.ToLowerInvariant() -eq $owner.licenseSha256 -and
    (Test-Path -LiteralPath (Join-Path $output '7zFM.exe') -PathType Leaf) -and
    (Get-FileHash -LiteralPath (Join-Path $output '7zFM.exe') -Algorithm SHA256).Hash.ToLowerInvariant() -eq $owner.binarySha256) {
    Copy-Runtime
    Write-Output "Reused native rendered reference: $output/7zFM.exe"
    return
}
$snapshot = Join-Path $output 'source'
$sourceOwner = Join-Path $snapshot '.sevenzip-runtime-source.json'
if ((Test-Path -LiteralPath $snapshot) -and
    ((Get-Item -LiteralPath $snapshot).Attributes -band [IO.FileAttributes]::ReparsePoint)) {
    throw 'Refusing a reparse-point reference source.'
}
# Adopt only an earlier snapshot belonging to this same reference builder.
if ($owner -and (Test-Path -LiteralPath $snapshot -PathType Container) -and
    -not (Test-Path -LiteralPath $sourceOwner)) {
    if ([string]$owner.revision -notmatch '^[0-9a-f]{40}$') { throw 'Invalid previous reference revision.' }
    $ownedFiles = @(& git -C $upstream ls-tree -r --name-only $owner.revision)
    if ($LASTEXITCODE -ne 0) { throw 'Cannot identify previous reference sources.' }
    $sourceRecord = [ordered]@{ producer = 'sevenzip-runtime-source'; files = $ownedFiles }
    [IO.File]::WriteAllText($sourceOwner, ($sourceRecord | ConvertTo-Json -Depth 4))
}
& $python (Join-Path $PSScriptRoot 'upstream_baseline.py') runtime-snapshot --original-only --no-build-rules `
    --snapshot $snapshot --upstream $upstream
if ($LASTEXITCODE -ne 0) { throw 'Cannot refresh owned reference source.' }
$module = Join-Path $snapshot 'CPP/7zip/UI/FileManager'
$original = Join-Path $module 'FM.cpp'
$originalHash = (Get-FileHash -LiteralPath $original -Algorithm SHA256).Hash.ToLowerInvariant()
Move-Item -LiteralPath $original -Destination (Join-Path $module 'FM.original.cpp') -Force
Copy-Item -LiteralPath $adapter -Destination $original -Force
$privateName = if ($owner -and $owner.registryKey -match '^Software\\(SevenzipDesktopReference-[0-9a-f]{32})$') {
    $Matches[1]
} else { 'SevenzipDesktopReference-' + [Guid]::NewGuid().ToString('N') }
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
    $makeArgs = @('/nologo', '/f', 'makefile', 'PLATFORM=x64')
    if (Get-Command ccache.exe -ErrorAction SilentlyContinue) { $makeArgs += 'CC=ccache cl' }
    & nmake @makeArgs
    if ($LASTEXITCODE -ne 0) { throw 'Native reference build failed.' }
} finally { Pop-Location }
Copy-Item -LiteralPath (Join-Path $module 'x64/7zFM.exe') -Destination $output
Copy-Runtime
Copy-Item -LiteralPath (Join-Path $snapshot 'DOC/License.txt') -Destination (Join-Path $output 'LICENSE.txt')
[ordered]@{
    schema = 1
    producer = 'sevenzip-desktop/build-windows-reference'
    repository = [string]$baseline.repository
    revision = $revision
    source = 'CPP/7zip/UI/FileManager/makefile'
    entrySource = 'CPP/7zip/UI/FileManager/FM.cpp'
    entrySha256 = $originalHash
    adapter = 'tests/support/native_reference_entry.cpp'
    adapterSha256 = (Get-FileHash -LiteralPath $adapter -Algorithm SHA256).Hash.ToLowerInvariant()
    registryKey = $registryKey
    registrySources = $registrySources
    binarySha256 = (Get-FileHash -LiteralPath (Join-Path $output '7zFM.exe') -Algorithm SHA256).Hash.ToLowerInvariant()
    licenseSha256 = (Get-FileHash -LiteralPath (Join-Path $output 'LICENSE.txt') -Algorithm SHA256).Hash.ToLowerInvariant()
    system = 'Windows'
    architecture = 'x86_64'
    inputSignature = $signature
    adaptation = 'Entry name alias and two private product registry path macros; original functional bodies/resources/menu handlers unchanged. Shell provider HKCU remains native.'
} | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $output 'reference.json') -Encoding UTF8
Write-Output "Native rendered reference: $output/7zFM.exe"
