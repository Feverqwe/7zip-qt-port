# Windows 10/11, Windows PowerShell 5.1 or PowerShell 7. No administrator required.
[CmdletBinding()]
param(
    [string]$OutputDirectory = (Join-Path (Get-Location) 'windows-ui-reference'),
    [string]$State = 'initial',
    [string]$Language = 'unspecified',
    [switch]$Screenshots,
    [string]$TemplatesFrom,
    [switch]$SelfTest
)
$ErrorActionPreference = 'Stop'
if ($env:OS -ne 'Windows_NT') { throw 'Run this script on Windows.' }

if (-not ('SevenzipReference.Native' -as [type])) {
    Add-Type -TypeDefinition @'
using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using System.Text;

namespace SevenzipReference {
public static class Native {
    [StructLayout(LayoutKind.Sequential)] public struct Rect { public int Left, Top, Right, Bottom; }
    [StructLayout(LayoutKind.Sequential)] public struct Point { public int X, Y; }
    [StructLayout(LayoutKind.Sequential, CharSet = System.Runtime.InteropServices.CharSet.Unicode)] public struct LogFont {
        public int Height, Width, Escapement, Orientation, Weight;
        public byte Italic, Underline, StrikeOut, CharSet, OutPrecision, ClipPrecision, Quality, PitchAndFamily;
        [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 32)] public string FaceName;
    }
    public delegate bool EnumProc(IntPtr window, IntPtr parameter);
    delegate bool ResourceProc(IntPtr module, IntPtr type, IntPtr name, IntPtr parameter);
    delegate IntPtr DialogProc(IntPtr window, uint message, IntPtr wparam, IntPtr lparam);
    [StructLayout(LayoutKind.Sequential)] struct CommonControls { public uint Size, Classes; }
    [DllImport("comctl32.dll", SetLastError = true)] static extern bool InitCommonControlsEx(ref CommonControls controls);
    [StructLayout(LayoutKind.Sequential)] struct Message {
        public IntPtr Window; public uint Id; public UIntPtr WParam; public IntPtr LParam;
        public uint Time; public Point Position; public uint Private;
    }
    [DllImport("kernel32.dll", CharSet = CharSet.Unicode, SetLastError = true)] static extern IntPtr LoadLibraryEx(string path, IntPtr file, uint flags);
    [DllImport("kernel32.dll")] public static extern bool FreeLibrary(IntPtr module);
    [DllImport("kernel32.dll", CharSet = CharSet.Unicode, SetLastError = true)] static extern bool EnumResourceNames(IntPtr module, IntPtr type, ResourceProc callback, IntPtr parameter);
    [DllImport("kernel32.dll", CharSet = CharSet.Unicode)] static extern IntPtr FindResource(IntPtr module, IntPtr name, IntPtr type);
    [DllImport("kernel32.dll")] static extern IntPtr LoadResource(IntPtr module, IntPtr resource);
    [DllImport("kernel32.dll")] static extern IntPtr LockResource(IntPtr resource);
    [DllImport("user32.dll", CharSet = CharSet.Unicode, SetLastError = true)] static extern IntPtr CreateDialogIndirectParam(IntPtr module, IntPtr template, IntPtr parent, DialogProc procedure, IntPtr parameter);
    [DllImport("user32.dll", CharSet = CharSet.Unicode, SetLastError = true)] static extern IntPtr CreateWindowEx(uint exStyle, string cls, string title, uint style, int x, int y, int width, int height, IntPtr parent, IntPtr menu, IntPtr instance, IntPtr parameter);
    [DllImport("user32.dll")] public static extern bool DestroyWindow(IntPtr window);
    [DllImport("user32.dll")] static extern bool ShowWindow(IntPtr window, int command);
    [DllImport("user32.dll")] static extern bool UpdateWindow(IntPtr window);
    [DllImport("user32.dll")] static extern bool PeekMessage(out Message message, IntPtr window, uint min, uint max, uint remove);
    [DllImport("user32.dll")] static extern bool TranslateMessage(ref Message message);
    [DllImport("user32.dll")] static extern IntPtr DispatchMessage(ref Message message);
    static DialogProc previewProcedure = delegate(IntPtr window, uint message, IntPtr wp, IntPtr lp) { return IntPtr.Zero; };
    public static void InitializeControls() {
        // ICC_WIN95_CLASSES is the baseline needed by 7-Zip's list/tree/tab/progress
        // controls. Do not fail the entire run if a newer optional class is absent.
        var controls = new CommonControls { Size = 8, Classes = 0x00FF };
        if (!InitCommonControlsEx(ref controls))
            throw new System.ComponentModel.Win32Exception(Marshal.GetLastWin32Error(),
                "Cannot initialize baseline native common controls.");
        foreach (uint optional in new uint[] { 0x0100, 0x0200, 0x0400, 0x0800, 0x1000, 0x2000, 0x8000 }) {
            controls.Classes = optional;
            InitCommonControlsEx(ref controls);
        }
    }
    public static IntPtr LoadTemplates(string path) {
        InitializeControls();
        // LOAD_LIBRARY_AS_DATAFILE | LOAD_LIBRARY_AS_IMAGE_RESOURCE: no executable entry point.
        IntPtr module = LoadLibraryEx(path, IntPtr.Zero, 0x22);
        if (module == IntPtr.Zero) throw new System.ComponentModel.Win32Exception(Marshal.GetLastWin32Error());
        return module;
    }
    public static string[] TemplateNames(IntPtr module) {
        var names = new List<string>();
        ResourceProc callback = delegate(IntPtr m, IntPtr t, IntPtr name, IntPtr p) {
            names.Add(unchecked((ulong)name.ToInt64()) <= 65535 ? "#" + name.ToInt64() : Marshal.PtrToStringUni(name));
            return true;
        };
        if (!EnumResourceNames(module, new IntPtr(5), callback, IntPtr.Zero))
            throw new System.ComponentModel.Win32Exception(Marshal.GetLastWin32Error());
        GC.KeepAlive(callback);
        return names.ToArray();
    }
    public static IntPtr PreviewHost(string name) {
        // Property-page templates have WS_CHILD and require a real parent.
        IntPtr host = CreateWindowEx(0, "Static", "7-Zip resource preview " + name, 0x10CF0000,
            20, 20, 1000, 800, IntPtr.Zero, IntPtr.Zero, IntPtr.Zero, IntPtr.Zero);
        if (host == IntPtr.Zero) throw new System.ComponentModel.Win32Exception(Marshal.GetLastWin32Error());
        return host;
    }
    public static IntPtr Probe() {
        IntPtr host = PreviewHost("self-test");
        try {
            if (CreateWindowEx(0, "Static", "Reference label", 0x50000000, 10, 10, 120, 24,
                    host, new IntPtr(1001), IntPtr.Zero, IntPtr.Zero) == IntPtr.Zero ||
                CreateWindowEx(0, "Edit", "DO-NOT-CAPTURE", 0x50000000, 10, 40, 120, 24,
                    host, new IntPtr(1002), IntPtr.Zero, IntPtr.Zero) == IntPtr.Zero ||
                CreateWindowEx(0, "Button", "Hidden", 0x40000000, 10, 70, 120, 24,
                    host, new IntPtr(1003), IntPtr.Zero, IntPtr.Zero) == IntPtr.Zero)
                throw new InvalidOperationException("Cannot create capture test controls.");
            return host;
        } catch { DestroyWindow(host); throw; }
    }
    public static IntPtr Preview(IntPtr module, string name, IntPtr host) {
        IntPtr resourceName = name.StartsWith("#") ? new IntPtr(Int32.Parse(name.Substring(1))) : Marshal.StringToHGlobalUni(name);
        try {
            IntPtr resource = FindResource(module, resourceName, new IntPtr(5));
            if (resource == IntPtr.Zero) throw new InvalidOperationException("Dialog resource not found: " + name);
            IntPtr template = LockResource(LoadResource(module, resource));
            if (template == IntPtr.Zero) throw new InvalidOperationException("Cannot load dialog: " + name);
            IntPtr window = CreateDialogIndirectParam(module, template, host, previewProcedure, IntPtr.Zero);
            if (window == IntPtr.Zero) throw new System.ComponentModel.Win32Exception(Marshal.GetLastWin32Error());
            ShowWindow(window, 5); UpdateWindow(window); Pump(); return window;
        } finally { if (!name.StartsWith("#")) Marshal.FreeHGlobal(resourceName); }
    }
    public static void Pump() {
        Message message;
        while (PeekMessage(out message, IntPtr.Zero, 0, 0, 1)) {
            TranslateMessage(ref message); DispatchMessage(ref message);
        }
    }
    [DllImport("user32.dll")] public static extern IntPtr GetForegroundWindow();
    [DllImport("user32.dll")] public static extern short GetAsyncKeyState(int key);
    [DllImport("user32.dll")] public static extern uint GetWindowThreadProcessId(IntPtr window, out uint process);
    [DllImport("user32.dll")] public static extern IntPtr SetThreadDpiAwarenessContext(IntPtr context);
    [DllImport("user32.dll")] static extern bool GetWindowRect(IntPtr window, out Rect rect);
    [DllImport("user32.dll")] static extern bool GetClientRect(IntPtr window, out Rect rect);
    [DllImport("user32.dll")] static extern bool ClientToScreen(IntPtr window, ref Point point);
    [DllImport("user32.dll")] static extern bool EnumChildWindows(IntPtr window, EnumProc callback, IntPtr parameter);
    [DllImport("user32.dll")] static extern IntPtr GetParent(IntPtr window);
    [DllImport("user32.dll")] static extern int GetDlgCtrlID(IntPtr window);
    [DllImport("user32.dll")] static extern bool IsWindowVisible(IntPtr window);
    [DllImport("user32.dll")] static extern bool IsWindowEnabled(IntPtr window);
    [DllImport("user32.dll")] public static extern bool IsIconic(IntPtr window);
    [DllImport("user32.dll")] static extern uint GetDpiForWindow(IntPtr window);
    [DllImport("user32.dll")] static extern IntPtr GetWindowDpiAwarenessContext(IntPtr window);
    [DllImport("user32.dll")] static extern int GetAwarenessFromDpiAwarenessContext(IntPtr context);
    [DllImport("user32.dll")] static extern bool MapDialogRect(IntPtr window, ref Rect rect);
    [DllImport("user32.dll", CharSet = CharSet.Unicode)] static extern int GetClassName(IntPtr window, StringBuilder name, int size);
    [DllImport("user32.dll", CharSet = CharSet.Unicode, EntryPoint = "GetWindowLongW")] static extern int GetWindowLong(IntPtr window, int index);
    [DllImport("user32.dll", CharSet = CharSet.Unicode)] static extern IntPtr SendMessageTimeout(IntPtr window, uint message, IntPtr wparam, StringBuilder text, uint flags, uint timeout, out IntPtr result);
    [DllImport("user32.dll", EntryPoint = "SendMessageTimeoutW")] static extern IntPtr SendMessageTimeoutValue(IntPtr window, uint message, IntPtr wparam, IntPtr lparam, uint flags, uint timeout, out IntPtr result);
    [DllImport("gdi32.dll", CharSet = CharSet.Unicode, EntryPoint = "GetObjectW")] static extern int GetObject(IntPtr handle, int size, out LogFont font);
    [DllImport("dwmapi.dll")] static extern int DwmGetWindowAttribute(IntPtr window, int attribute, out Rect rect, int size);

    static string Handle(IntPtr window) { return "0x" + window.ToInt64().ToString("X"); }
    static int[] Bounds(Rect r) { return new int[] { r.Left, r.Top, r.Right-r.Left, r.Bottom-r.Top }; }
    static string Class(IntPtr window) {
        var name = new StringBuilder(256); GetClassName(window, name, name.Capacity); return name.ToString();
    }
    // Never read editable fields, selections, list rows, or password values.
    static object Text(IntPtr window, string cls, bool root) {
        if (!root && cls != "Static" && cls != "Button") return null;
        var buffer = new StringBuilder(4096); IntPtr result;
        if (SendMessageTimeout(window, 0x000D, new IntPtr(buffer.Capacity), buffer, 2, 200, out result) == IntPtr.Zero) return null;
        return buffer.ToString();
    }
    static object Font(IntPtr window) {
        IntPtr handle;
        if (SendMessageTimeoutValue(window, 0x0031, IntPtr.Zero, IntPtr.Zero, 2, 200, out handle) == IntPtr.Zero || handle == IntPtr.Zero) return null;
        LogFont font;
        if (GetObject(handle, Marshal.SizeOf(typeof(LogFont)), out font) == 0) return null;
        return new Dictionary<string, object> {
            {"face", font.FaceName}, {"height", font.Height}, {"width", font.Width},
            {"weight", font.Weight}, {"italic", font.Italic != 0}, {"charset", font.CharSet}
        };
    }
    static Dictionary<string, object> Node(IntPtr window, Point origin, bool root) {
        Rect outer, client; Point start = new Point();
        if (!GetWindowRect(window, out outer) || !GetClientRect(window, out client) || !ClientToScreen(window, ref start))
            throw new InvalidOperationException("Window disappeared during capture; retry.");
        string cls = Class(window);
        var relative = outer; relative.Left -= origin.X; relative.Right -= origin.X;
        relative.Top -= origin.Y; relative.Bottom -= origin.Y;
        return new Dictionary<string, object> {
            {"handle", Handle(window)}, {"parentHandle", Handle(GetParent(window))},
            {"class", cls}, {"controlId", GetDlgCtrlID(window)}, {"text", Text(window, cls, root)},
            {"visible", IsWindowVisible(window)}, {"enabled", IsWindowEnabled(window)},
            {"windowBoundsScreenPx", Bounds(outer)}, {"boundsRootClientPx", Bounds(relative)},
            {"clientSizePx", new int[] {client.Right-client.Left, client.Bottom-client.Top}},
            {"clientOriginScreenPx", new int[] {start.X, start.Y}},
            {"style", "0x" + unchecked((uint)GetWindowLong(window, -16)).ToString("X8")},
            {"exStyle", "0x" + unchecked((uint)GetWindowLong(window, -20)).ToString("X8")},
            {"font", Font(window)}
        };
    }
    // Caller must keep this context for the entire geometry + screenshot operation.
    public static IntPtr BeginPhysicalCoordinates() {
        IntPtr previous = SetThreadDpiAwarenessContext(new IntPtr(-4));
        if (previous == IntPtr.Zero) throw new InvalidOperationException("Per-monitor V2 DPI context unavailable (requires Windows 10 1703+).");
        return previous;
    }
    public static object Capture(IntPtr window) {
        Point origin = new Point();
        if (!ClientToScreen(window, ref origin)) throw new InvalidOperationException("Cannot measure window.");
        var root = Node(window, origin, true);
        root["dpi"] = GetDpiForWindow(window);
        root["dpiAwareness"] = GetAwarenessFromDpiAwarenessContext(GetWindowDpiAwarenessContext(window));
        Rect frame;
        root["visibleFrameBoundsScreenPx"] = DwmGetWindowAttribute(window, 9, out frame, Marshal.SizeOf(typeof(Rect))) == 0 ? Bounds(frame) : null;
        Rect units = new Rect { Right = 4, Bottom = 8 };
        root["dialogBaseUnitsPx"] = Class(window) == "#32770" && MapDialogRect(window, ref units) ? Bounds(units) : null;
        var controls = new List<object>();
        Exception error = null;
        EnumProc callback = delegate(IntPtr child, IntPtr unused) {
            try { controls.Add(Node(child, origin, false)); return true; }
            catch (Exception ex) { error = ex; return false; }
        };
        EnumChildWindows(window, callback, IntPtr.Zero);
        GC.KeepAlive(callback);
        if (error != null) throw error;
        return new Dictionary<string, object> { {"window", root}, {"controls", controls} };
    }
}
}
'@
}
if ($SelfTest) {
    [SevenzipReference.Native]::InitializeControls()
    $previous = [SevenzipReference.Native]::BeginPhysicalCoordinates()
    $probeWindow = [IntPtr]::Zero
    try {
        $probeWindow = [SevenzipReference.Native]::Probe()
        $probe = [SevenzipReference.Native]::Capture($probeWindow)
        $roundTrip = $probe | ConvertTo-Json -Depth 12 | ConvertFrom-Json
        $label = @($roundTrip.controls | Where-Object controlId -EQ 1001)
        $edit = @($roundTrip.controls | Where-Object controlId -EQ 1002)
        $hidden = @($roundTrip.controls | Where-Object controlId -EQ 1003)
        if ($roundTrip.controls.Count -ne 3 -or $label.Count -ne 1 -or $edit.Count -ne 1 -or $hidden.Count -ne 1) {
            throw 'Self-test: enumeration/control IDs/JSON round trip failed.'
        }
        if ($label[0].text -ne 'Reference label' -or $null -ne $edit[0].text -or $hidden[0].visible) {
            throw 'Self-test: static text/edit privacy/hidden control state failed.'
        }
        if (($label[0].boundsRootClientPx -join ',') -ne '10,10,120,24' -or
            $label[0].parentHandle -ne $roundTrip.window.handle) {
            throw 'Self-test: physical client geometry/parent relationship failed.'
        }
        Write-Host 'Self-test passed: native geometry, IDs, hierarchy, visibility, edit omission, JSON round trip.'
    } finally {
        if ($probeWindow -ne [IntPtr]::Zero) { [void][SevenzipReference.Native]::DestroyWindow($probeWindow) }
        [void][SevenzipReference.Native]::SetThreadDpiAwarenessContext($previous)
    }
    return
}
if ($Screenshots) { Add-Type -AssemblyName System.Drawing }
$outputPath = [IO.Path]::GetFullPath($OutputDirectory)
[void][IO.Directory]::CreateDirectory($outputPath)
Write-Host "Output: $outputPath"
Write-Host 'JSON omits editable values. PNG includes everything visible in the window.'
function Save-Reference([IntPtr]$window, [string]$exePath, [string]$resourceName = '') {
            $version = [Diagnostics.FileVersionInfo]::GetVersionInfo($exePath)
            $previous = [SevenzipReference.Native]::BeginPhysicalCoordinates()
            try {
                $geometry = [SevenzipReference.Native]::Capture($window)
                $stamp = (Get-Date).ToUniversalTime().ToString('yyyyMMddTHHmmssfffZ') + '-' + [Guid]::NewGuid().ToString('N').Substring(0, 6)
                $base = Join-Path $outputPath $stamp
                $screenshotName = $null
                if ($Screenshots) {
                    $rect = $geometry.window.visibleFrameBoundsScreenPx
                    if ($null -eq $rect) { $rect = $geometry.window.windowBoundsScreenPx }
                    if ($rect[2] -le 0 -or $rect[3] -le 0) { throw 'Invalid screenshot bounds.' }
                    $bitmap = New-Object System.Drawing.Bitmap($rect[2], $rect[3])
                    try {
                        $graphics = [Drawing.Graphics]::FromImage($bitmap)
                        try { $graphics.CopyFromScreen($rect[0], $rect[1], 0, 0, $bitmap.Size) }
                        finally { $graphics.Dispose() }
                        $bitmap.Save("$base.png", [Drawing.Imaging.ImageFormat]::Png)
                        $screenshotName = "$stamp.png"
                    } finally { $bitmap.Dispose() }
                }
            } finally { [void][SevenzipReference.Native]::SetThreadDpiAwarenessContext($previous) }
            $report = [ordered]@{
                schemaVersion = 1
                capturedAtUtc = (Get-Date).ToUniversalTime().ToString('o')
                state = $State
                language = $Language
                osVersion = [Environment]::OSVersion.VersionString
                powershellVersion = $PSVersionTable.PSVersion.ToString()
                coordinateSpace = 'physical-screen-pixels; controls relative to root client origin'
                executable = [ordered]@{
                    name = [IO.Path]::GetFileName($exePath)
                    fileVersion = $version.FileVersion
                    productVersion = $version.ProductVersion
                    sha256 = (Get-FileHash -LiteralPath $exePath -Algorithm SHA256).Hash.ToLowerInvariant()
                }
                screenshot = $screenshotName
                captureKind = $(if ($resourceName) { 'resource-template' } else { 'application-runtime' })
                resourceName = $resourceName
                window = $geometry.window
                controls = @($geometry.controls)
            }
            $json = $report | ConvertTo-Json -Depth 12
            [IO.File]::WriteAllText("$base.json", $json, (New-Object Text.UTF8Encoding($false)))
            Write-Host "Saved $stamp : $($geometry.window.class), $($geometry.controls.Count) controls"
}

if ($TemplatesFrom) {
    $exePath = (Resolve-Path -LiteralPath $TemplatesFrom).Path
    $module = [SevenzipReference.Native]::LoadTemplates($exePath)
    $failures = @()
    try {
        $names = [SevenzipReference.Native]::TemplateNames($module)
        Write-Host "Capturing $($names.Count) dialog templates. Do not interact with preview windows."
        foreach ($name in $names) {
            $window = [IntPtr]::Zero
            $hostWindow = [IntPtr]::Zero
            $previous = [SevenzipReference.Native]::BeginPhysicalCoordinates()
            try {
                $hostWindow = [SevenzipReference.Native]::PreviewHost($name)
                $window = [SevenzipReference.Native]::Preview($module, $name, $hostWindow)
                # Let native controls finish painting before optional screen capture.
                for ($i = 0; $i -lt 10; $i++) {
                    [SevenzipReference.Native]::Pump()
                    Start-Sleep -Milliseconds 50
                }
                Save-Reference $window $exePath $name
            } catch {
                $failures += [ordered]@{ resourceName = $name; error = $_.Exception.Message }
                Write-Warning "Resource $name : $($_.Exception.Message)"
            } finally {
                if ($window -ne [IntPtr]::Zero) { [void][SevenzipReference.Native]::DestroyWindow($window) }
                if ($hostWindow -ne [IntPtr]::Zero) { [void][SevenzipReference.Native]::DestroyWindow($hostWindow) }
                [void][SevenzipReference.Native]::SetThreadDpiAwarenessContext($previous)
            }
        }
        $summary = [ordered]@{
            captureKind = 'resource-template'
            total = $names.Count
            succeeded = $names.Count - $failures.Count
            failures = @($failures)
            limitation = 'Resource templates only; no original OnInit, translations, handlers or runtime states.'
        }
        $summaryPath = Join-Path $outputPath ('templates-summary-' + (Get-Date).ToUniversalTime().ToString('yyyyMMddTHHmmssfffZ') + '.json')
        [IO.File]::WriteAllText($summaryPath, ($summary | ConvertTo-Json -Depth 6), (New-Object Text.UTF8Encoding($false)))
        if ($failures.Count) { throw "$($failures.Count) template(s) failed; see $summaryPath" }
    } finally { [void][SevenzipReference.Native]::FreeLibrary($module) }
    return
}

Write-Host 'Open a dialog in official 7zFM.exe (or 7zG.exe), keep it foreground, press F8.'
Write-Host 'Capture initial size before resizing. F8 can be repeated. Ctrl+C here stops.'
$wasDown = $false
while ($true) {
    $down = ([SevenzipReference.Native]::GetAsyncKeyState(0x77) -band 0x8000) -ne 0
    if ($down -and -not $wasDown) {
        try {
            $window = [SevenzipReference.Native]::GetForegroundWindow()
            [uint32]$targetProcessId = 0
            [void][SevenzipReference.Native]::GetWindowThreadProcessId($window, [ref]$targetProcessId)
            $target = Get-Process -Id $targetProcessId
            if ($target.ProcessName -notin @('7zFM', '7zG')) { throw 'Foreground window must belong to 7zFM.exe or 7zG.exe.' }
            if ([SevenzipReference.Native]::IsIconic($window)) { throw 'Restore the window first.' }
            Save-Reference $window $target.MainModule.FileName
        } catch { Write-Warning $_.Exception.Message }
    }
    $wasDown = $down
    Start-Sleep -Milliseconds 40
}
