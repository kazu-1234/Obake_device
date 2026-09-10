/*
 * StackChan custom firmware version (user patch on top of official FIRMWARE_VERSION).
 * SPDX-License-Identifier: MIT
 */
#pragma once

namespace stackchan::custom {

/** Increment on each user-original change (1, 2, 3, …). */
inline constexpr const char* kCustomPatch = "3";

/** Version number without prefix (official PROJECT_VER + patch). Must match CMake PROJECT_VER. */
inline constexpr const char* kFullVersion = "1.5.1.3";

/** Display label including V prefix. */
inline constexpr const char* kFullVersionLabel = "V1.5.1.3";

}  // namespace stackchan::custom
