# Obake face（CUSTOM 専用）

Port.A → PaHub の両目 OLED・ToF と、CoreS3 口 UI。サーボは扱わない。

| ファイル | 役割 |
|----------|------|
| `obake_config.h` | **人が変える数値**（先頭）＋ハード定数 |
| `obake_runtime.*` | init / UI tick / hw タスク |
| `obake_pahub.*` | I2C port0 (GPIO2/1) + PaHub |
| `obake_eyes.*` | CH0/1 OLED |
| `obake_tof.*` | CH2 VL53L0X |
| `obake_mouth_ui.*` | 白地 ∪/∩ + cm |
| `obake_wake_config.h` | 「おばけちゃん」単一入口 |
| `vl53l0x.*` | Pololu 系ドライバ（ESP-IDF 移植） |

起動条件: `IsCustomSessionActive()` かつ `OnXiaozhiUiReady`。

---

## 調整手順（明るさ・まばたき・きょろきょろ）

数値は **[`obake_config.h`](obake_config.h) の先頭ブロック「人が変える値」だけ** 編集する。`.cpp` 内のリテラルは触らない。

### 手順

1. `firmware/main/stackchan/custom/obake/obake_config.h` を開く
2. 先頭の定数を変更（下表）
3. `firmware/` で `idf.py build` → `idf.py -p COMx flash`
4. ランチャーで **CUSTOM** を起動し、目・口・距離を確認

### よく触る定数

| 変えたいこと | 定数 | 目安 |
|--------------|------|------|
| 目を明るく／暗く | `kOledContrast` | 上げる=明（0–255）。既定 `0x5A` |
| まばたきを遅く | `kBlinkOpenMinMs` / `kBlinkOpenSpanMs` を大きく | 開眼の待ち = Min + 乱数(Span) |
| まばたきの閉眼を長く | `kBlinkClosedMinMs` / `kBlinkClosedSpanMs` | 閉眼時間 |
| きょろきょろを少なく | `kLookMinMs` / `kLookSpanMs` を大きく | 視線変更の間隔 |
| 視線の振れ幅 | `kLookRangeX` / `kLookRangeY` | 中心からの ± 幅 |
| 口 ∪/∩ の切替を少なく | `kMouthFlipMinMs` / `kMouthFlipSpanMs` | 視線とは独立 |
| ToF 表示の更新間隔 | `kTofUpdateMs` | 既定 1000 ms |
| hw ポーリング周期 | `kHwTickMs` | 既定 40（閉眼最短より短く） |

ハード配線（GPIO・PaHub CH・I2C アドレス）は同ファイル後半。普段は触らない。
