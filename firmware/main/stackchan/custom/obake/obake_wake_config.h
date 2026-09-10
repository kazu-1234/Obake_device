/*
 * 呼びかけの単一設定入口。表示名・Multinet 用拼音をここだけ揃える。
 *
 * 実スイッチは sdkconfig.defaults.local:
 *   CONFIG_USE_CUSTOM_WAKE_WORD=y
 *   CONFIG_CUSTOM_WAKE_WORD / DISPLAY を下の定数に合わせる
 *   CONFIG_CUSTOM_WAKE_WORD_THRESHOLD（小さいほど敏感。既定 5）
 * 効きが弱いときは AFE + CONFIG_SR_WN_WN9_HISTACKCHAN_TTS3 に戻す。
 */
#pragma once

namespace stackchan::obake {

/** UI・サーバ向け表示名（変更しやすい） */
inline constexpr const char* kWakeDisplayName = "おばけちゃん";

/** Multinet 用拼音（スペース区切り）= CONFIG_CUSTOM_WAKE_WORD */
inline constexpr const char* kWakeMultinetPinyin = "o ba ke chan";

/**
 * Multinet 検知閾値の目安（実値は sdkconfig の CONFIG_CUSTOM_WAKE_WORD_THRESHOLD）。
 * sdkconfig は百分率整数（5 → 0.05）。小さいほど敏感。
 */
inline constexpr int kWakeThresholdPercentHint = 5;

/** true: Multinet を使う意図（実スイッチは sdkconfig） */
inline constexpr bool kPreferCustomMultinet = true;

}  // namespace stackchan::obake
