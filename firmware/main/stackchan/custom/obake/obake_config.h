/*
 * Obake ハード定数（.ino v0.4.11 正本）。サーボは載せない。
 */
#pragma once

#include <driver/gpio.h>
#include <stdint.h>

namespace stackchan::obake {

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
inline constexpr uint32_t kTofUpdateMs = 1000;

inline constexpr const char* kObakeUiLabel = "Obake face";

}  // namespace stackchan::obake
