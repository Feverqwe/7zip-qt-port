function Get-SevenzipMsvcTools {
    $compiler = (Get-Command cl.exe -CommandType Application -ErrorAction Stop).Source
    $version = (Get-Item -LiteralPath $compiler).VersionInfo
    if ($version.FileMajorPart -lt 19 -or
        ($version.FileMajorPart -eq 19 -and $version.FileMinorPart -lt 43)) {
        throw 'C++23 mode requires MSVC 19.43+ (Visual Studio 2022 17.13+).'
    }
    $bin = Split-Path -Parent $compiler
    $null = Get-Command (Join-Path $bin 'link.exe') -ErrorAction Stop
    # NMake invokes link by name; Git Bash also ships an unrelated link.exe.
    $env:PATH = "$bin;$env:PATH"
    return $bin
}
