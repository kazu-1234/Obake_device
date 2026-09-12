# メイン PC: サブから渡した flash bundle を CoreS3 に書く（IDF 不要）
# 使い方:
#   powershell -ExecutionPolicy Bypass -File scripts\flash_bundle.ps1
#   powershell -ExecutionPolicy Bypass -File scripts\flash_bundle.ps1 -Port COM5
#
# 前提: pip install esptool pyserial
# 探す順: dist/obake_flash_bundle → firmware/build
param(
    [string]$Port = "COM3",
    [string]$Baud = "921600",
    [string]$BundleDir = ""
)

$ErrorActionPreference = "Stop"
$RepoRoot = Join-Path $PSScriptRoot ".." | Resolve-Path

if (-not $BundleDir) {
    $cands = @(
        (Join-Path $RepoRoot "dist\obake_flash_bundle"),
        (Join-Path $RepoRoot "firmware\build")
    )
    foreach ($c in $cands) {
        if (Test-Path (Join-Path $c "flasher_args.json")) {
            $BundleDir = $c
            break
        }
    }
}

if (-not $BundleDir -or -not (Test-Path (Join-Path $BundleDir "flasher_args.json"))) {
    throw "bundle がありません。サブ PC で export_flash_bundle.ps1 -Zip し、dist\obake_flash_bundle\ に展開してください。"
}

$ArgsFile = Join-Path $BundleDir "flasher_args.json"
$json = Get-Content -LiteralPath $ArgsFile -Raw -Encoding UTF8 | ConvertFrom-Json
$files = $json.flash_files
if (-not $files) {
    throw "flasher_args.json に flash_files がありません。"
}

$mode = "dio"
$freq = "80m"
$size = "16MB"
if ($json.flash_settings) {
    if ($json.flash_settings.flash_mode) { $mode = $json.flash_settings.flash_mode }
    if ($json.flash_settings.flash_freq) { $freq = $json.flash_settings.flash_freq }
    if ($json.flash_settings.flash_size) { $size = $json.flash_settings.flash_size }
}

$pairs = @()
foreach ($prop in $files.PSObject.Properties) {
    $off = $prop.Name
    $rel = ($prop.Value -replace "/", "\")
    $bin = Join-Path $BundleDir $rel
    if (-not (Test-Path -LiteralPath $bin)) {
        throw "見つからない: $bin"
    }
    $pairs += [pscustomobject]@{ Offset = [int]$off; Hex = $off; Path = $bin }
}
$pairs = $pairs | Sort-Object Offset

$esptoolArgs = @(
    "-m", "esptool",
    "--chip", "esp32s3",
    "-p", $Port,
    "-b", $Baud,
    "write-flash",
    "--flash-mode", $mode,
    "--flash-size", $size,
    "--flash-freq", $freq
)
foreach ($p in $pairs) {
    $esptoolArgs += $p.Hex
    $esptoolArgs += $p.Path
}

Write-Host "[flash_bundle] dir=$BundleDir  port=$Port"
python @esptoolArgs
if ($LASTEXITCODE -ne 0) {
    exit $LASTEXITCODE
}
Write-Host "[flash_bundle] flash OK"
exit 0
