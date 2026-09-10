# Obake firmware（ESP-IDF）

CoreS3 の中身は **ESP32-S3**。Stack-chan と同じ **ESP-IDF** で書く。  
会話（聞く・話す）と Grove（目・ToF・手）はすべてこのフォルダ。

## なぜ ESP-IDF か

Stack-chan の音声は `xiaozhi-esp32`（Opus + WebSocket）。STT / LLM / TTS はサーバ。  
このスタックは ESP-IDF 前提で、Arduino にそのままは載らない。CoreS3 なら最初から IDF でよい。

| 項目 | Stack-chan | Obake（このフォルダ） |
|------|------------|----------------------|
| SoC | ESP32-S3（CoreS3） | 同じ |
| IDF | v5.5.4 | 同じにする |
| 会話 | xiaozhi-esp32 v2.2.4 | 同じリポを `repos.json` で取る |
| 本体 | mooncake + HAL | これから。Grove 目 / ToF / Catch を HAL に足す |

最短で声を出すなら、既存の `Documents/Arduino/Stackchan/firmware` を書き込んで Xiaozhi を確認し、動きをこちらへ移植する。

## 準備

- [ESP-IDF v5.5.4](https://docs.espressif.com/projects/esp-idf/en/v5.5.4/esp32s3/index.html)
- ターゲットは `esp32s3`
- Wi-Fi や OTA URL は `sdkconfig.defaults.local`（git しない）

```powershell
# ESP-IDF の PowerShell を開いてから
cd C:\Users\kazuh\Documents\Arduino\Obake_device\firmware
idf.py set-target esp32s3
idf.py build
idf.py -p COMx flash monitor
```

いまの `main.cpp` は起動ログだけ。Xiaozhi の `Application::Initialize()` / `Run()` は次の作業。

## Xiaozhi の取り方（次）

```powershell
python fetch_repos.py
```

`repos.json` の `xiaozhi-esp32`（v2.2.4）を取る。Stack-chan 側のパッチを当てるかは、そちらで動いている構成に合わせる。

接続先の例（値は homelab の実体に合わせ、リポには書かない）:

- OTA: `http://<host>:8003/xiaozhi/ota/`
- WebSocket: `ws://<host>:8000/xiaozhi/v1/`

## これから足すもの

1. CoreS3 の I2S（ES7210 + AW88298）を Xiaozhi の AudioCodec に渡す（Stack-chan の `cores3_audio_codec.cc` 相当）
2. タッチまたはウェイクで `ToggleChatState()`
3. Grove: PaHUB + Unit LCD×2 + ToF4M + Catch（ルート README の配線）
4. 状態 LISTENING / SPEAKING で目を開閉
