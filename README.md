# Obake_device

白い丸いおばけの本体ハードとファーム。天井移動は他担当。

参考図: [reference/content.png](reference/content.png)

表示版: v0.5.0

別 PC へ移すときは [引き継ぎ.md](引き継ぎ.md) を先に読む。次段階は **Stack-chan ファーム＋ESP-IDF**（Arduino 確認用は本番ではない）。

当面の脳は **Stack-chan の CoreS3**。カメラ・マイク・スピーカも Stack-chan。会話は当面 Xiaozhi（あとで独自に差し替え）。液晶は口。  
免責: 個人の製作・ハッカソン用途。部品・配線の焼損や落下の責任は負わない。

## 方針（現行）

はんだ・3Dプリンタなし。PaHub / ToF は Grove。目の SSD1306 は 2.54 mm ピンなので **Grove–Dupont 変換**。  
主控は当面 Stack-chan。図中の Pi はサイズの目安（外径 110–130 mm）だけ使う。

## 部品（現行）

所持して動かすもの

- Stack-chan（CoreS3）… 脳・給電・I2C
- PaHub v2.1 ×1
- SSD1306 1インチ級 ×2（目）
- Unit ToF U010 ×1（床）

追加で買うもの

- **Grove–Dupont メス変換** ×2（目用。HY2.0-4P 2.0 mm → ピンヘッダ 2.54 mm）
- Grove 20 cm 予備（球内の引き回し。ToF は付属ケーブルで可）

旧買い物リスト（専用 CoreS3・Unit LCD・ToF4M・Catch）は使わない。戻さない。

## 配線（Stack-chan Port.A）

CoreS3 の Grove 5V はファーム側でポート電源を入れる。

Port.A（黒 GND / 赤 5V / 黄 G2 SDA / 白 G1 SCL）→ PaHub v2.1 入力（0x70）。

- PaHub CH0 → 左目 SSD1306（Grove–Dupont。I2C 既定 0x3C）
- PaHub CH1 → 右目 SSD1306（同じ 0x3C でよい。CH が違うので衝突しない）
- PaHub CH2 → Unit ToF U010（I2C 0x29）下向き。Grove 同士なのでそのまま

Grove（HY2.0、ピッチ 2.0 mm）と 1インチ OLED（ピンヘッダ 2.54 mm）は形状が違うので直接刺さらない。変換ケーブルで **色で**つなぐ（ピン位置では合わせない）。

| Grove | OLED 側（基板印刷） |
|-------|---------------------|
| 黒 GND | GND |
| 赤 5V  | VCC（3.3–5V 対応モジュール） |
| 黄 SDA | SDA |
| 白 SCL | SCL |

アドレスをハードで分ける必要はない。分けたいときだけ OLED の ADDR ジャンパで 0x3C / 0x3D。ソフトの 7bit は 0x3C / 0x3D（基板の 0x78 / 0x7A は 8bit 表記）。

## 顔の配置

- 両目: 1インチ SSD1306。隙間約 10 mm
- 底面: ToF U010 を床向き
- 内部: PaHub。脳は Stack-chan（球の外でも可）

## 書き込み

本番: Stack-chan と同じ **ESP-IDF**（[引き継ぎ.md](引き継ぎ.md) 7・8 節）。  
Flash の退避と戻し: [バックアップと戻し方.md](バックアップと戻し方.md)  
ハード確認用 Arduino のみ: [arduino/obake_pahub_bringup/FLASH.md](arduino/obake_pahub_bringup/FLASH.md)

## これから載せる動き

- 呼びかけ「おばけちゃん」（変更しやすい）。ウェイク LED は Stack-chan のまま
- 口（CoreS3 液晶）。目は OLED
- 会話は当面 Xiaozhi。独自化は後
- 移動マイコンへ「来て」「20 cm 維持」（通信は Wi-Fi or BLE、未定）

## 飛行担当へ渡すもの

- `ALT_CM=` の cm 値
- 重量目安（おばけ側）: PaHub 約 7 g + OLED×2 + ToF 約 4 g ≒ **25 g**＋ケーブル・布。脳は Stack-chan 側
- 外径 110–130 mm
- 5V 共有は先に相談（サーボは未搭載）
