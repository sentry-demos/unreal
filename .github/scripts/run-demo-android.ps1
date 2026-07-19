# Runs the SentryTower demo on a SauceLabs Android device via app-runner.
#
# Scenario:
#   Run 1: launch the game and wait for its intentional crash (captured, not yet uploaded).
#   Run 2: relaunch with -upload-only so the pending crash report is sent; the game exits
#          itself shortly after Sentry initializes in this mode.
#
# Requires environment variables:
#   SAUCE_USERNAME, SAUCE_ACCESS_KEY, SAUCE_REGION, SAUCE_DEVICE_NAME, SAUCE_SESSION_NAME
#   SENTRY_DSN (passed to the game as a launch arg)

param(
    [Parameter(Mandatory = $true)]
    [string]$ApkDir,

    # 'AndroidSauceLabs' (real device) or 'Adb' (local device/emulator)
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

$activity = 'io.sentry.tower/com.epicgames.unreal.GameActivity'
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
