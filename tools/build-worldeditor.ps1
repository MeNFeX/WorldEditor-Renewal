param(
    [ValidateSet('WE-Debug', 'WE-Release')]
    [string]$Configuration = 'WE-Debug',
    [string]$DataDirectory = 'D:\ymir work'
)

$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path -Parent $PSScriptRoot
$vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
if (-not (Test-Path -LiteralPath $vswhere)) {
    throw 'Visual Studio Installer / vswhere.exe was not found.'
}
$installation = & $vswhere -latest -products '*' -requires Microsoft.Component.MSBuild -property installationPath
if (-not $installation) { throw 'Visual Studio with MSBuild was not found.' }
$msbuild = Join-Path $installation 'MSBuild\Current\Bin\MSBuild.exe'
$logDirectory = Join-Path $projectRoot '.build'
New-Item -ItemType Directory -Path $logDirectory -Force | Out-Null
$buildLog = Join-Path $logDirectory ($Configuration + '.log')
$solution = Join-Path $projectRoot 'Client\Metin2Client.sln'
& $msbuild $solution /t:WorldEditor "/p:Configuration=$Configuration" "/p:WorldEditorDataDirectory=$DataDirectory" /p:Platform=Win32 /m:4 /nologo /v:minimal /fl "/flp:logfile=$buildLog;verbosity=normal"
if ($LASTEXITCODE -ne 0) { throw "Build failed. See $buildLog" }
Write-Host "Executable: $(Join-Path $DataDirectory "WorldEditor_${Configuration}_Win32.exe")"
