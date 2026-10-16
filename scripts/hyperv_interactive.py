"""Run a snapshot's Windows gate in the logged-on runner's desktop session."""

import base64
import re


def interactive_command(root, elevated=False):
    if not re.fullmatch(r"C:/runner/sevenzip-desktop/runs/[0-9]{8}T[0-9]{6}Z-[0-9a-f]{8}", root):
        raise ValueError("Interactive task requires a project-owned snapshot directory")
    # check.cmd supplies the same toolchain, engine, environment and command as
    # the SSH path. Only the Windows logon session changes; no shell UI or app
    # visibility workaround is injected into the product or its tests.
    run_level = "Highest" if elevated else "Limited"
    child = f"""$ErrorActionPreference='Stop'
$ProgressPreference='SilentlyContinue'
$root='{root}'
$code=1
try {{
    Set-Location $root
    $env:TEMP=$env:TMP=$env:TMPDIR="$root/runner-tmp"
    $session=(Get-Process -Id $PID).SessionId
    if ($session -eq 0) {{ throw 'Interactive runner entered session 0' }}
    $identity=[Security.Principal.WindowsIdentity]::GetCurrent()
    $principal=[Security.Principal.WindowsPrincipal]::new($identity)
    $elevated=$principal.IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)
    $runnerPriority=[Diagnostics.Process]::GetCurrentProcess().PriorityClass
    [IO.File]::WriteAllText("$root/interactive-session.txt", "Session=$session User=$env:USERNAME RunLevel={run_level} Elevated=$elevated Priority=$runnerPriority")
    if ({'$true' if elevated else '$false'} -and !$elevated) {{ throw 'Elevated fixtures require an administrator runner token' }}
    if ($runnerPriority -ne 'Normal') {{ throw 'Interactive fixtures require normal process priority' }}
    $env:SEVENZIP_RUNNER_EXPECT_NORMAL_PRIORITY='1'
    & cmd.exe /d /c 'check.cmd > check.log 2>&1'
    $code=$LASTEXITCODE
}} catch {{
    $_ | Out-String | Add-Content "$root/check.log"
}} finally {{
    [IO.File]::WriteAllText("$root/interactive-exit.txt", [string]$code)
}}
exit $code
"""
    encoded = base64.b64encode(child.encode("utf-16le")).decode()
    return f"""$taskName='Sevenzip-check-'+[guid]::NewGuid().ToString('N')
$user=[Security.Principal.WindowsIdentity]::GetCurrent().Name
$action=New-ScheduledTaskAction -Execute 'powershell.exe' -Argument '-NoProfile -EncodedCommand {encoded}'
$principal=New-ScheduledTaskPrincipal -UserId $user -LogonType Interactive -RunLevel {run_level}
$settings=New-ScheduledTaskSettingsSet -Priority 4 -ExecutionTimeLimit (New-TimeSpan -Hours 2)
Register-ScheduledTask -TaskName $taskName -Action $action -Principal $principal -Settings $settings | Out-Null
Write-Output "Interactive task=$taskName user=$user"
try {{
    Start-ScheduledTask -TaskName $taskName
    $deadline=(Get-Date).AddHours(2)
    while (!(Test-Path '{root}/interactive-exit.txt')) {{
        if ((Get-Date) -gt $deadline) {{ throw "Interactive task timeout: $taskName" }}
        Start-Sleep -Seconds 1
        $info=Get-ScheduledTaskInfo -TaskName $taskName
        $state=(Get-ScheduledTask -TaskName $taskName).State
        if ($state -ne 'Running' -and $info.LastRunTime.Year -gt 2000) {{
            if (!(Test-Path '{root}/interactive-exit.txt')) {{
                throw "Interactive task produced no result: $($info.LastTaskResult); active logon required for $user"
            }}
        }}
    }}
    Get-Content '{root}/interactive-session.txt'
    $code=[int](Get-Content '{root}/interactive-exit.txt' -Raw)
    # A result file can precede the final process exit by a few milliseconds.
    while ((Get-ScheduledTask -TaskName $taskName).State -eq 'Running') {{ Start-Sleep -Milliseconds 100 }}
}} finally {{
    if ((Get-ScheduledTask -TaskName $taskName).State -ne 'Running') {{
        Unregister-ScheduledTask -TaskName $taskName -Confirm:$false
    }} else {{
        Write-Output "Owned task still running: $taskName"
    }}
}}
"""
