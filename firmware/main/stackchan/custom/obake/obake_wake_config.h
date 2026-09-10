/*
 * 呼びかけの単一設定入口。表示名・Multinet 用拼音をここだけ変える。
 *
 * sdkconfig.defaults.local で USE_CUSTOM_WAKE_WORD=y にし、
 * CUSTOM_WAKE_WORD / DISPLAY を下の定数に合わせる。
 * 効きが弱いときは AFE + CONFIG_SR_WN_WN9_HISTACKCHAN_TTS3 に戻す。
 */
#pragma once

namespace stackchan::obake {

/** UI・サーバ向け表示名（変更しやすい） */
inline constexpr const char* kWakeDisplayName = "おばけちゃん";

/** Multinet 用拼音（スペース区切り）= CONFIG_CUSTOM_WAKE_WORD */
inline constexpr const char* kWakeMultinetPinyin = "o ba ke chan";

/** true: Multinet を使う意図（実スイッチは sdkconfig） */
inline constexpr bool kPreferCustomMultinet = true;

}  // namespace stackchan::obake
