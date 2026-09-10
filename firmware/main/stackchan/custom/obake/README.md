# Obake face（CUSTOM 専用）

Port.A → PaHub の両目 OLED・ToF と、CoreS3 口 UI。サーボは扱わない。

| ファイル | 役割 |
|----------|------|
| `obake_runtime.*` | init / UI tick / hw タスク |
| `obake_pahub.*` | I2C port0 (GPIO2/1) + PaHub |
| `obake_eyes.*` | CH0/1 OLED |
| `obake_tof.*` | CH2 VL53L0X |
| `obake_mouth_ui.*` | 白地 ∪/∩ + cm |
| `obake_wake_config.h` | 「おばけちゃん」単一入口 |
| `vl53l0x.*` | Pololu 系ドライバ（ESP-IDF 移植） |

起動条件: `IsCustomSessionActive()` かつ `OnXiaozhiUiReady`。
