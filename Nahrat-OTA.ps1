<#
  Priame OTA nahrani Solar Assistantu podle IP adresy.
  Nevyzaduje Bonjour, mDNS ani sitovy port v Arduino IDE.

  Spusteni z PowerShellu ve slozce projektu:
    powershell -ExecutionPolicy Bypass -File .\Nahrat-OTA.ps1
#>
param(
  [string]$Ip,
  [string]$Bin
)

$ErrorActionPreference = 'Stop'
$projectDir = Split-Path -Parent $PSCommandPath

if ([string]::IsNullOrWhiteSpace($Ip)) {
  $Ip = Read-Host 'IP adresa ESP32'
}

$parsedIp = $null
if (-not [System.Net.IPAddress]::TryParse($Ip, [ref]$parsedIp)) {
  throw "Neplatna IP adresa: $Ip"
}

if ([string]::IsNullOrWhiteSpace($Bin)) {
  $cli = Join-Path $env:ProgramFiles 'Arduino CLI\arduino-cli.exe'
  if (-not (Test-Path -LiteralPath $cli)) {
    $cliCommand = Get-Command 'arduino-cli.exe' -ErrorAction SilentlyContinue
    if ($null -eq $cliCommand) {
      throw 'Arduino CLI nebylo nalezeno. Nainstalujte jej nebo zadejte cestu k souboru BIN parametrem -Bin.'
    }
    $cli = $cliCommand.Source
  }

  $buildDir = Join-Path ([System.IO.Path]::GetTempPath()) ("solarassistant-ota-" + [guid]::NewGuid().ToString('N'))
  New-Item -ItemType Directory -Path $buildDir | Out-Null
  Write-Host 'Kompiluji firmware pro OTA oddily...'
  & $cli compile --fqbn 'esp32:esp32:esp32:PartitionScheme=min_spiffs' --output-dir $buildDir $projectDir
  if ($LASTEXITCODE -ne 0) { throw "Kompilace selhala (kod $LASTEXITCODE)." }
  $Bin = Join-Path $buildDir 'SolarAssistant-TMK.ino.bin'
}

if (-not (Test-Path -LiteralPath $Bin)) {
  throw "Soubor BIN nebyl nalezen: $Bin"
}

$coreRoot = Join-Path $env:LOCALAPPDATA 'Arduino15\packages\esp32\hardware\esp32'
if (-not (Test-Path -LiteralPath $coreRoot)) {
  throw 'Jadro ESP32 v Arduino15 nebylo nalezeno.'
}
$core = Get-ChildItem -LiteralPath $coreRoot -Directory | Sort-Object Name -Descending | Select-Object -First 1
$espota = Join-Path $core.FullName 'tools\espota.exe'
if (-not (Test-Path -LiteralPath $espota)) {
  throw "Nastroj espota.exe nebyl nalezen: $espota"
}

$securePassword = Read-Host 'OTA heslo (nebude zobrazeno)' -AsSecureString
$passwordPtr = [Runtime.InteropServices.Marshal]::SecureStringToBSTR($securePassword)
try {
  $password = [Runtime.InteropServices.Marshal]::PtrToStringBSTR($passwordPtr)
  Write-Host "Nahravam na $Ip..."
  & $espota -i $Ip -p 3232 "--auth=$password" -f $Bin
  if ($LASTEXITCODE -ne 0) { throw "OTA nahrani selhalo (kod $LASTEXITCODE)." }
}
finally {
  if ($passwordPtr -ne [IntPtr]::Zero) { [Runtime.InteropServices.Marshal]::ZeroFreeBSTR($passwordPtr) }
}

Write-Host 'OTA nahrani dokonceno.' -ForegroundColor Green
