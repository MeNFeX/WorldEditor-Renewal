param(
    [ValidateSet('WE-Debug', 'WE-Release')]
    [string]$Configuration = 'WE-Release',
    [string]$DataDirectory = 'D:\ymir work'
)

$ErrorActionPreference = 'Stop'
$executable = Join-Path $DataDirectory "WorldEditor_${Configuration}_Win32.exe"
if (-not (Test-Path -LiteralPath $executable)) {
    throw "Build the editor first with tools\build-worldeditor.ps1 -Configuration $Configuration"
}
if (-not (Test-Path -LiteralPath $DataDirectory -PathType Container)) {
    throw "The game data directory does not exist: $DataDirectory"
}
Start-Process -FilePath $executable -WorkingDirectory $DataDirectory -WindowStyle Normal
