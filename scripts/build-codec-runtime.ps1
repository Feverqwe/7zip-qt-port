# Prepare the official Windows console sidecar and codec module from pinned source.
[CmdletBinding()]
param(
    [string]$OutputDirectory,
    [ValidateSet('arm64', 'x64', 'x86')]
    [string]$Architecture = $env:VSCMD_ARG_TGT_ARCH
)
$ErrorActionPreference = 'Stop'
$scriptDirectory = [IO.Path]::GetDirectoryName($MyInvocation.MyCommand.Path)
. (Join-Path $scriptDirectory 'lib/workspace.ps1')
Initialize-SevenzipWorkspace (Split-Path -Parent $scriptDirectory)
if (-not $OutputDirectory) { $OutputDirectory = Join-Path $env:SEVENZIP_ARTIFACT_ROOT 'dev/codec-runtime' }
Assert-SevenzipOutput (Split-Path -Parent $scriptDirectory) $OutputDirectory
if ($env:OS -ne 'Windows_NT') { throw 'Run this script on Windows.' }
if (-not $Architecture -or $env:VSCMD_ARG_TGT_ARCH -ne $Architecture) {
    throw 'Choose a matching MSVC developer environment and -Architecture arm64, x64 or x86.'
}
. (Join-Path $scriptDirectory 'lib/msvc-toolchain.ps1')
$msvcBin = Get-SevenzipMsvcTools
Write-Output "Using MSVC tools: $msvcBin"
# Git Bash's GNU tar treats drive-letter paths as remote archive names.
$tar = Join-Path $env:SystemRoot 'System32/tar.exe'
foreach ($tool in @($tar, 'nmake', 'cl', 'link', 'rc')) {
    $null = Get-Command $tool -ErrorAction Stop
}
function Invoke-Checked([string]$Program, [string[]]$Arguments) {
    & $Program @Arguments
    if ($LASTEXITCODE -ne 0) { throw "$Program failed (exit $LASTEXITCODE)." }
}
$projectRoot = [IO.Path]::GetFullPath((Join-Path $scriptDirectory '..'))
$baseline = Get-Content -LiteralPath (Join-Path $projectRoot 'docs/upstream-baseline.json') -Raw |
    ConvertFrom-Json
$revision = [string]$baseline.revision
if ($revision -notmatch '^[0-9a-f]{40}$') { throw 'Invalid recorded upstream revision.' }
$output = [IO.Path]::GetFullPath($OutputDirectory)
$null = New-Item -ItemType Directory -Path $output -Force
if ((Get-Item -LiteralPath $output).Attributes -band [IO.FileAttributes]::ReparsePoint) {
    throw 'Refusing a reparse-point runtime output directory.'
}
$artifacts = [ordered]@{
    '7z.exe' = 'CPP/7zip/UI/Console'
    '7z.dll' = 'CPP/7zip/Bundles/Format7zF'
}
$manifest = Join-Path $output 'runtime.json'
$existing = @($artifacts.Keys | ForEach-Object { Join-Path $output $_ }) +
            @($manifest, (Join-Path $output 'LICENSE.txt'))
$existing = @($existing | Where-Object { Test-Path -LiteralPath $_ })
foreach ($path in $existing) {
    if ((Get-Item -LiteralPath $path).Attributes -band [IO.FileAttributes]::ReparsePoint) {
        throw 'Refusing to replace a reparse-point codec runtime.'
    }
}
if ($existing.Count -gt 0) {
    if (-not (Test-Path -LiteralPath $manifest -PathType Leaf)) {
        throw 'Refusing to replace an unowned codec runtime.'
    }
    $owner = Get-Content -LiteralPath $manifest -Raw | ConvertFrom-Json
    if ($owner.producer -ne 'sevenzip-desktop/build-codec-runtime-windows') {
        throw 'Refusing to replace an unowned codec runtime.'
    }
}
$snapshot = Join-Path $output '.build-runtime'
& {
    Invoke-Checked 'python' @((Join-Path $scriptDirectory 'upstream_baseline.py'),
        'runtime-snapshot', '--snapshot', $snapshot)
    Invoke-Checked 'python' @((Join-Path $scriptDirectory 'build-native-dependencies.py'),
        '--snapshot', $snapshot)
    $files = [ordered]@{}
    foreach ($name in $artifacts.Keys) {
        $bundle = Join-Path $snapshot $artifacts[$name]
        Push-Location -LiteralPath $bundle
        try {
            $makeArgs = @('/nologo', '/f', 'sevenzip-runtime.mak', "PLATFORM=$Architecture")
            if (Get-Command ccache.exe -ErrorAction SilentlyContinue) {
                $makeArgs += 'CC=ccache cl'
            }
            Invoke-Checked 'nmake' $makeArgs
        } finally { Pop-Location }
        $built = Join-Path $bundle "$Architecture/$name"
        if (-not (Test-Path -LiteralPath $built -PathType Leaf)) { throw "Missing built $name." }
        $files[$name] = [ordered]@{
            source = $artifacts[$name] + '/makefile'
            sha256 = (Get-FileHash -LiteralPath $built -Algorithm SHA256).Hash.ToLowerInvariant()
        }
    }
    $header = Get-Content -LiteralPath (Join-Path $snapshot 'C/7zVersion.h') -Raw
    $major = [regex]::Match($header, '#define MY_VER_MAJOR\s+(\d+)')
    $minor = [regex]::Match($header, '#define MY_VER_MINOR\s+(\d+)')
    if (-not $major.Success -or -not $minor.Success) { throw 'Missing upstream version.' }
    $record = [ordered]@{
        schema = 1
        producer = 'sevenzip-desktop/build-codec-runtime-windows'
        repository = [string]$baseline.repository
        revision = $revision
        adapted_sources = Get-Content -LiteralPath (Join-Path $snapshot 'runtime-adaptations.json') -Raw | ConvertFrom-Json
        system = 'Windows'
        architecture = @{ arm64 = 'arm64'; x64 = 'x86_64'; x86 = 'x86' }[$Architecture]
        version = ([int]$major.Groups[1].Value -shl 16) -bor [int]$minor.Groups[1].Value
        files = $files
    }
    $retired = @()
    if ($owner) {
        foreach ($property in $owner.files.PSObject.Properties) {
            $name = $property.Name
            if ($artifacts.Contains($name)) { continue }
            if ([IO.Path]::GetFileName($name) -ne $name) { throw 'Unsafe retired runtime artifact.' }
            $path = Join-Path $output $name
            if (Test-Path -LiteralPath $path) {
                $item = Get-Item -LiteralPath $path
                if (($item.Attributes -band [IO.FileAttributes]::ReparsePoint) -or
                    (Get-FileHash -LiteralPath $path -Algorithm SHA256).Hash.ToLowerInvariant() -ne $property.Value.sha256) {
                    throw 'Refusing to remove a modified retired runtime artifact.'
                }
                $retired += $path
            }
        }
    }
    $stagedManifest = Join-Path $snapshot 'runtime.json'
    [IO.File]::WriteAllText($stagedManifest, ($record | ConvertTo-Json -Depth 5) + "`n",
                          (New-Object Text.UTF8Encoding($false)))
    foreach ($name in $artifacts.Keys) {
        Copy-Item -LiteralPath (Join-Path (Join-Path $snapshot $artifacts[$name]) "$Architecture/$name") -Destination (Join-Path $output $name)
    }
    Copy-Item -LiteralPath (Join-Path $snapshot 'DOC/License.txt') -Destination (Join-Path $output 'LICENSE.txt')
    Move-Item -LiteralPath $stagedManifest -Destination $manifest -Force
    foreach ($path in $retired) { Remove-Item -LiteralPath $path }
    Write-Output "Prepared official Windows codec runtime: $output"
    Write-Output "Upstream: $revision"
}
