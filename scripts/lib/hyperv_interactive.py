"""Run the persistent Windows worktree in the logged-on desktop session."""

import base64
import re
from .runner_config import runner_config

def interactive_child_command(root, elevated=False, token=None):
    """The task owns its result even when the SSH observer disconnects."""
    # check.cmd supplies the same toolchain, engine, environment and command as
    # the SSH path. Only the Windows logon session changes; no shell UI or app
    # visibility workaround is injected into the product or its tests.
    run_level = "Highest" if elevated else "Limited"
    completion = (f'    [IO.File]::WriteAllText("$root/.runner-complete", \'{token}\')'
                  if token is not None else "")
    return f"""$ErrorActionPreference='Stop'
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
{completion}
}}
exit $code
"""


def interactive_command(root, elevated=False, token=None):
    workspace = runner_config("windows")["workspace"]["root"]
    relative = root[len(workspace) + 1:] if root.startswith(workspace + "/") else ""
    if relative != "current" and not re.fullmatch(r"branch-[A-Za-z0-9_-]{1,48}-[0-9a-f]{20}", relative):
        raise ValueError("Interactive task requires the configured persistent worktree")
    run_level = "Highest" if elevated else "Limited"
    child = interactive_child_command(root, elevated, token)
    encoded = base64.b64encode(child.encode("utf-16le")).decode()
    return f"""Remove-Item '{root}/interactive-exit.txt','{root}/interactive-session.txt' -Force -ErrorAction SilentlyContinue
$taskName='Sevenzip-check-'+[guid]::NewGuid().ToString('N')
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
