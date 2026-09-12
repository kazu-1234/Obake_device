# サブ PC: IDF build から書き込みに必要なファイルだけを抜き出す
# 使い方:
#   powershell -ExecutionPolicy Bypass -File scripts\export_flash_bundle.ps1
#   powershell -ExecutionPolicy Bypass -File scripts\export_flash_bundle.ps1 -Zip
#
# 出力: dist/obake_flash_bundle/ （flasher_args.json + bin）
#       -Zip 時は dist/obake_flash_bundle.zip も作る → メイン PC へ渡す
param(
    [switch]$Zip
)

$ErrorActionPreference = "Stop"
$RepoRoot = Join-Path $PSScriptRoot ".." | Resolve-Path
$Build = Join-Path $RepoRoot "firmware\build"
$ArgsFile = Join-Path $Build "flasher_args.json"
$OutDir = Join-Path $RepoRoot "dist\obake_flash_bundle"
$ZipPath = Join-Path $RepoRoot "dist\obake_flash_bundle.zip"

if (-not (Test-Path $ArgsFile)) {
    throw "firmware/build/flasher_args.json がありません。先に scripts/fast_build.ps1 を実行してください。"
}

$json = Get-Content -LiteralPath $ArgsFile -Raw -Encoding UTF8 | ConvertFrom-Json
$files = $json.flash_files
if (-not $files) {
    throw "flasher_args.json に flash_files がありません。"
}

if (Test-Path $OutDir) {
    Remove-Item -LiteralPath $OutDir -Recurse -Force
}
New-Item -ItemType Directory -Force -Path $OutDir | Out-Null

Copy-Item -LiteralPath $ArgsFile -Destination (Join-Path $OutDir "flasher_args.json")

foreach ($prop in $files.PSObject.Properties) {
    $rel = ($prop.Value -replace "/", "\")
    $src = Join-Path $Build $rel
    if (-not (Test-Path -LiteralPath $src)) {
        throw "見つからない: $src"
    }
    $dst = Join-Path $OutDir $rel
    $dstParent = Split-Path -Parent $dst
    New-Item -ItemType Directory -Force -Path $dstParent | Out-Null
    Copy-Item -LiteralPath $src -Destination $dst
}

$howto = @"
Obake flash bundle（サブ PC のビルド成果）

メイン PC:
1. このフォルダごと Obake_device\dist\obake_flash_bundle\ に置く
   （zip なら展開して、中に flasher_args.json が見えること）
2. シリアルモニタを閉じ、COM を確認
3. リポで:
   .\scripts\flash_bundle.ps1 -Port COM3
   または tools\obake_flash_gui.pyw でこのフォルダを選ぶ
"@
$howtoPath = Join-Path $OutDir "README_MAIN_PC.txt"
[System.IO.File]::WriteAllText($howtoPath, $howto, [System.Text.UTF8Encoding]::new($false))

Write-Host "[export_flash_bundle] OK  $OutDir"

if ($Zip) {
    if (Test-Path $ZipPath) {
        Remove-Item -LiteralPath $ZipPath -Force
    }
    Compress-Archive -Path (Join-Path $OutDir "*") -DestinationPath $ZipPath
    Write-Host "[export_flash_bundle] zip  $ZipPath"
}

exit 0
