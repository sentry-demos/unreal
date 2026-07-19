# Runs the SentryTower demo on an Android device.
#
# Scenario:
#   Run 1: launch the game and wait for its intentional crash (captured, not yet uploaded).
#   Run 2: relaunch with -upload-only so the pending crash report is sent; the game exits
#          itself shortly after Sentry initializes in this mode.
#
# Platforms:
#   AndroidSauceLabs (default) - real device via app-runner; requires SAUCE_* env vars.
#   Adb - local device/emulator; launches via adb directly for full control/visibility.
#
# Requires environment variable SENTRY_DSN (passed to the game as a launch arg).

param(
    [Parameter(Mandatory = $true)]
    [string]$ApkDir,

    [ValidateSet('AndroidSauceLabs', 'Adb')]
    [string]$DevicePlatform = 'AndroidSauceLabs'
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

Import-Module "$PSScriptRoot/../../app-runner/app-runner/SentryAppRunner.psm1"

if (-not $env:SENTRY_DSN) {
    throw 'Environment variable SENTRY_DSN must be set'
}

# SauceLabs devices are arm64; CI emulators are x86_64
$preferredArch = if ($DevicePlatform -eq 'Adb') { 'x64|x86' } else { 'arm64' }
$apk = Get-ChildItem -Path $ApkDir -Filter 'SentryTower*.apk' -Recurse |
    Sort-Object { $_.Name -match $preferredArch } -Descending |
    Select-Object -First 1
if (-not $apk) {
    throw "No SentryTower APK found in $ApkDir"
}
Write-Host "Using APK: $($apk.FullName)"

$package = 'io.sentry.tower'
$activity = "$package/com.epicgames.unreal.GameActivity"

function Dump-AdbLogcat([string]$Label) {
    Write-Host "--- logcat ($Label) ---"
    adb logcat -d -v time | Select-String -Pattern 'Sentry|LogSentrySdk|GameActivity|UE ' -CaseSensitive | ForEach-Object { $_.Line }
    adb logcat -c
    Write-Host "--- end logcat ($Label) ---"
}

# Launches the app via adb with the given UE command line and waits for it to exit.
function Invoke-AdbRun([string]$UeCommandLine, [int]$TimeoutSec) {
    # Single quotes protect [brackets] and spaces from the on-device shell
    $shellCmd = "am start -W -S -n $activity -e cmdline '$UeCommandLine'"
    Write-Host "adb shell $shellCmd"
    adb shell $shellCmd 2>&1 | ForEach-Object { Write-Host "  am: $_" }

    $started = $false
    $deadline = (Get-Date).AddSeconds(30)
    while ((Get-Date) -lt $deadline) {
        adb shell pidof $package *> $null
        if ($LASTEXITCODE -eq 0) { $started = $true; break }
        Start-Sleep -Seconds 1
    }
    if (-not $started) {
        Write-Host '::warning::App process never appeared after am start'
        return
    }
    Write-Host 'App is running, waiting for it to exit...'

    $deadline = (Get-Date).AddSeconds($TimeoutSec)
    while ((Get-Date) -lt $deadline) {
        adb shell pidof $package *> $null
        if ($LASTEXITCODE -ne 0) { Write-Host 'App exited.'; return }
        Start-Sleep -Seconds 5
    }
    Write-Host "App still running after ${TimeoutSec}s, force-stopping..."
    adb shell am force-stop $package
}

if ($DevicePlatform -eq 'Adb') {
    # Emulators hang creating a Vulkan device on SwiftShader; force the GLES fallback
    # (-nullrhi is not supported on Android: no null shader platform)
    $commonArgs = "-ini:Engine:[SystemSettings]:r.Android.DisableVulkanSupport=1 " +
        "-ini:Engine:[/Script/Sentry.SentrySettings]:Dsn=$env:SENTRY_DSN " +
        "-ini:Engine:[/Script/Sentry.SentrySettings]:Debug=True"

    Write-Host 'Installing APK...'
    adb install -r $apk.FullName

    Dump-AdbLogcat 'pre-run'

    Write-Host 'Run 1: launching game simulation, waiting for intentional crash...'
    Invoke-AdbRun -UeCommandLine "--idle $commonArgs" -TimeoutSec 420
    Dump-AdbLogcat 'run 1'

    Write-Host 'Run 2: relaunching in upload-only mode to flush the crash report...'
    Invoke-AdbRun -UeCommandLine "-upload-only $commonArgs" -TimeoutSec 180
    Dump-AdbLogcat 'run 2'

    Write-Host 'Demo run completed.'
    return
}

$dsnArg = "-ini:Engine:\[/Script/Sentry.SentrySettings\]:Dsn=$env:SENTRY_DSN\ -ini:Engine:\[/Script/Sentry.SentrySettings\]:Debug=True"

try {
    Write-Host "Connecting to device (platform: $DevicePlatform)..."
    Connect-Device -Platform $DevicePlatform

    Write-Host 'Installing APK...'
    Install-DeviceApp -Path $apk.FullName

    Write-Host 'Run 1: launching game simulation, waiting for intentional crash...'
    $crashRun = Invoke-DeviceApp -ExecutablePath $activity -Arguments "-e cmdline --idle\ $dsnArg"
    Write-Host "Run 1 finished (exit code: $($crashRun.ExitCode))"

    Write-Host 'Run 2: relaunching in upload-only mode to flush the crash report...'
    $uploadRun = Invoke-DeviceApp -ExecutablePath $activity -Arguments "-e cmdline -upload-only\ $dsnArg"
    Write-Host "Run 2 finished (exit code: $($uploadRun.ExitCode))"

    Write-Host 'Demo run completed.'
}
finally {
    Write-Host 'Disconnecting from device...'
    Disconnect-Device
}
