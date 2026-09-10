/*
 * Obake 定数。先頭の「人が変える値」だけ触れば顔の見え方を調整できる。
 * 手順: 同フォルダ README.md「調整手順」を参照。
 */
#pragma once

#include <driver/gpio.h>
#include <stdint.h>

namespace stackchan::obake {

// =============================================================================
// 人が変える値（まばたき・きょろきょろ・明るさ・周期）
// ビルドし直して CUSTOM で確認。ハード配線はこのブロックの下。
// =============================================================================

/** OLED 明るさ（SH1106/SSD1306 contrast 0x81。0=暗〜255=明。既定 0x5A） */
inline constexpr uint8_t kOledContrast = 0x5A;

/** 開眼のまま待つ時間 = Min + rand%(Span+1) ms */
inline constexpr uint32_t kBlinkOpenMinMs = 1800;
inline constexpr uint32_t kBlinkOpenSpanMs = 2200;
/** 閉眼の長さ = Min + rand%(Span+1) ms */
inline constexpr uint32_t kBlinkClosedMinMs = 90;
inline constexpr uint32_t kBlinkClosedSpanMs = 80;
/** 起動後、初回まばたきまでの待ち */
inline constexpr uint32_t kBlinkFirstDelayMs = 2500;

/** きょろきょろ間隔 = Min + rand%(Span+1) ms（開眼中のみ） */
inline constexpr uint32_t kLookMinMs = 400;
inline constexpr uint32_t kLookSpanMs = 1400;
/** 開眼直後に次の視線を前倒しするオフセット */
inline constexpr uint32_t kLookAfterBlinkMs = 200;
/** 視線オフセット幅（中心±）。X は 0..2*Range、Y も同様 */
inline constexpr int kLookRangeX = 7;
inline constexpr int kLookRangeY = 10;

/** 口 ∪/∩ 切替間隔（視線と独立。長くすると口の再描画が減る） */
inline constexpr uint32_t kMouthFlipMinMs = 3000;
inline constexpr uint32_t kMouthFlipSpanMs = 4000;

/** PaHub 目・ToF を回す hw タスク周期 */
inline constexpr uint32_t kHwTickMs = 40;
/** ToF 表示更新周期 */
inline constexpr uint32_t kTofUpdateMs = 1000;
/** 口 UI: 標準顔・吹き出しを隠し直す保険間隔（毎フレームはやらない） */
inline constexpr uint32_t kMouthHideRetryMs = 500;

// =============================================================================
// ハード定数（配線・アドレス。普段は触らない）
// =============================================================================

/** Port.A Grove: SDA=G2 / SCL=G1（内部バス GPIO11/12 とは別） */
inline constexpr gpio_num_t kPortASda = GPIO_NUM_2;
inline constexpr gpio_num_t kPortAScl = GPIO_NUM_1;
inline constexpr uint32_t kI2cHz = 400000;
/** 内部 I2C が port 1 なので Port.A は 0 */
inline constexpr int kPortAI2cPort = 0;

inline constexpr uint8_t kPahubAddr = 0x70;
inline constexpr uint8_t kChLeft = 0;
inline constexpr uint8_t kChRight = 1;
inline constexpr uint8_t kChTof = 2;

inline constexpr uint8_t kOledAddr = 0x3C;
inline constexpr uint8_t kOledAddrAlt = 0x3D;
inline constexpr uint8_t kOledColOffset = 2;
inline constexpr bool kOledRotateCcw = false;

inline constexpr uint8_t kTofAddr = 0x29;
inline constexpr uint16_t kTofMaxMm = 2000;

inline constexpr const char* kObakeUiLabel = "Obake face";

}  // namespace stackchan::obake
