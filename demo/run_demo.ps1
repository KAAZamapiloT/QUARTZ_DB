# ===================================================================
# QuartzDB Demo Runner (PowerShell)
# Runs the SQL Quickstart demo and C API demo
# ===================================================================

$ErrorActionPreference = "Stop"

$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$repoDir = Split-Path -Parent $scriptDir

# Locate quartz.exe
$cliCandidates = @(
    "$repoDir\out\build\x64-Debug\quartz.exe",
    "$repoDir\build\quartz.exe",
    "$repoDir\build\Debug\quartz.exe",
    "$repoDir\build\Release\quartz.exe"
)

$cliPath = $cliCandidates | Where-Object { Test-Path $_ } | Select-Object -First 1

if (-not $cliPath) {
    Write-Host "[!] quartz.exe not found. Please build the project first:" -ForegroundColor Yellow
    Write-Host "    cmake --build out\build\x64-Debug --target quartz quartz_c_demo" -ForegroundColor White
    exit 1
}

Write-Host "===================================================================" -ForegroundColor Cyan
Write-Host "                QuartzDB Automated Demonstration" -ForegroundColor Cyan
Write-Host "===================================================================" -ForegroundColor Cyan
Write-Host "CLI Binary : $cliPath" -ForegroundColor Gray
Write-Host ""

# Clean previous demo database if exists
$demoDb = "$scriptDir\demo_quickstart.db"
if (Test-Path $demoDb) {
    Remove-Item -Force $demoDb
}

Write-Host "[1/2] Running SQL Quickstart Demo (demo/quickstart.sql)..." -ForegroundColor Green
Get-Content "$scriptDir\quickstart.sql" | & $cliPath "$demoDb"

Write-Host ""
Write-Host "[2/2] Checking for compiled C API Demo (quartz_c_demo.exe)..." -ForegroundColor Green

$cDemoCandidates = @(
    "$repoDir\out\build\x64-Debug\quartz_c_demo.exe",
    "$repoDir\build\quartz_c_demo.exe",
    "$repoDir\build\Debug\quartz_c_demo.exe",
    "$repoDir\build\Release\quartz_c_demo.exe"
)

$cDemoPath = $cDemoCandidates | Where-Object { Test-Path $_ } | Select-Object -First 1

if ($cDemoPath) {
    Write-Host "Running $cDemoPath..." -ForegroundColor Gray
    & $cDemoPath
} else {
    Write-Host "Notice: quartz_c_demo.exe not built yet." -ForegroundColor Yellow
    Write-Host "Build it via: cmake --build out\build\x64-Debug --target quartz_c_demo" -ForegroundColor Gray
}

Write-Host ""
Write-Host "Demonstrations finished!" -ForegroundColor Cyan
