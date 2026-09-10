/*
 * 左右 OLED: 縦長黒楕円・閉眼∪/∩・SH1106 132列白埋め・ページ交互送信。
 */
#include "obake_eyes.h"

#include "obake_config.h"
#include "obake_pahub.h"

#include <atomic>
#include <esp_log.h>
#include <esp_random.h>
#include <esp_timer.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <string.h>

namespace stackchan::obake {
namespace {

const char* TAG = "obake_eyes";
bool s_left_ok = false;
bool s_right_ok = false;
uint8_t s_left_addr = kOledAddr;
uint8_t s_right_addr = kOledAddr;
bool s_eyes_open = true;
/** hw タスク書き込み / UI 読み取り */
std::atomic<bool> s_mouth_smile{true};
int s_look_x = 0;
int s_look_y = 0;
uint32_t s_blink_at = 0;
uint32_t s_look_at = 0;

bool oled_cmd(uint8_t addr, uint8_t cmd)
{
    uint8_t buf[2] = {0x00, cmd};
    return PahubWrite(addr, buf, 2);
}

bool oled_init(uint8_t addr)
{
    static const uint8_t kInit[] = {
        0xAE, 0xD5, 0xF0, 0xA8, 0x3F, 0xD3, 0x00, 0x40, 0x8D, 0x14, 0x20, 0x02,
        0xA1, 0xC8, 0xDA, 0x12, 0x81, 0x5A, 0xD9, 0x22, 0xDB, 0x40, 0xA4, 0xA6, 0xAF,
    };
    for (uint8_t c : kInit) {
        if (!oled_cmd(addr, c)) {
            return false;
        }
    }
    vTaskDelay(pdMS_TO_TICKS(50));
    // SH1106 端の未書き込み黒しみ対策: 132 列を全面白
    uint8_t white[133];
    white[0] = 0x40;
    memset(white + 1, 0xFF, 132);
    for (int page = 0; page < 8; ++page) {
        uint8_t setcol[4] = {0x00, static_cast<uint8_t>(0xB0 | page), 0x00, 0x10};
        if (!PahubWrite(addr, setcol, 4)) {
            return false;
        }
        if (!PahubWrite(addr, white, 133)) {
            return false;
        }
    }
    return true;
}

void oled_send_page(uint8_t addr, const uint8_t* buf, int page)
{
    uint8_t setcol[4] = {0x00, static_cast<uint8_t>(0xB0 | page), 0x00, 0x10};
    PahubWrite(addr, setcol, 4);
    uint8_t row[133];
    row[0] = 0x40;
    memset(row + 1, 0xFF, 132);
    memcpy(row + 1 + kOledColOffset, buf + page * 128, 128);
    PahubWrite(addr, row, 133);
}

uint8_t find_oled_addr()
{
    if (PahubProbe(kOledAddr)) {
        return kOledAddr;
    }
    if (PahubProbe(kOledAddrAlt)) {
        return kOledAddrAlt;
    }
    for (uint8_t a = 0x08; a < 0x78; ++a) {
        if (a == kPahubAddr) {
            continue;
        }
        if (PahubProbe(a)) {
            return a;
        }
    }
    return 0;
}

void set_pixel(uint8_t* buf, int x, int y, bool on)
{
    if (x < 0 || x >= 128 || y < 0 || y >= 64) {
        return;
    }
    const int i = x + (y / 8) * 128;
    const uint8_t bit = static_cast<uint8_t>(1u << (y & 7));
    if (on) {
        buf[i] |= bit;
    } else {
        buf[i] &= static_cast<uint8_t>(~bit);
    }
}

void set_pixel_portrait(uint8_t* buf, int x, int y, bool on)
{
    int px;
    int py;
    if (kOledRotateCcw) {
        px = 127 - y;
        py = x;
    } else {
        px = y;
        py = 63 - x;
    }
    set_pixel(buf, px, py, on);
}

void fill_ellipse_portrait(uint8_t* buf, int cx, int cy, int rx, int ry, bool on)
{
    if (rx <= 0 || ry <= 0) {
        return;
    }
    const long rx2 = static_cast<long>(rx) * rx;
    const long ry2 = static_cast<long>(ry) * ry;
    const long r2 = rx2 * ry2;
    for (int y = -ry; y <= ry; ++y) {
        const long yy = static_cast<long>(y) * y * rx2;
        for (int x = -rx; x <= rx; ++x) {
            const long xx = static_cast<long>(x) * x * ry2;
            if (xx + yy <= r2) {
                set_pixel_portrait(buf, cx + x, cy + y, on);
            }
        }
    }
}

void render_eye(uint8_t* buf, bool open, int look_x, int look_y, bool smile)
{
    memset(buf, 0xFF, 1024);
    if (open) {
        fill_ellipse_portrait(buf, 32 + look_x, 64 + look_y, 16, 40, false);
    } else {
        const int cx = 32;
        const int cy = 96;
        if (smile) {
            fill_ellipse_portrait(buf, cx, cy + 8, 22, 16, false);
            fill_ellipse_portrait(buf, cx, cy - 14, 26, 18, true);
        } else {
            fill_ellipse_portrait(buf, cx, cy - 8, 22, 16, false);
            fill_ellipse_portrait(buf, cx, cy + 14, 26, 18, true);
        }
    }
}

void show_eyes(bool open)
{
    const bool smile = s_mouth_smile.load(std::memory_order_relaxed);
    uint8_t left_buf[1024];
    uint8_t right_buf[1024];
    if (s_left_ok) {
        render_eye(left_buf, open, s_look_x, s_look_y, smile);
    }
    if (s_right_ok) {
        render_eye(right_buf, open, s_look_x, s_look_y, smile);
    }
    // PaHub は同時不可。ページごとに左右連続で送り、ズレを抑える
    for (int page = 0; page < 8; ++page) {
        if (s_left_ok && PahubSelect(kChLeft)) {
            oled_send_page(s_left_addr, left_buf, page);
        }
        if (s_right_ok && PahubSelect(kChRight)) {
            oled_send_page(s_right_addr, right_buf, page);
        }
    }
}

void pick_look()
{
    s_look_x = static_cast<int>(esp_random() % 15) - 7;
    s_look_y = static_cast<int>(esp_random() % 21) - 10;
}

void pick_mouth()
{
    s_mouth_smile.store((esp_random() & 1) != 0, std::memory_order_relaxed);
}

}  // namespace

bool EyesInit()
{
    s_left_ok = false;
    s_right_ok = false;
    if (!PahubOk()) {
        return false;
    }
    PahubLock();
    if (PahubSelect(kChLeft)) {
        s_left_addr = find_oled_addr();
        s_left_ok = (s_left_addr != 0) && oled_init(s_left_addr);
        ESP_LOGI(TAG, "CH0 oled 0x%02X %s", s_left_addr, s_left_ok ? "ok" : "fail");
    }
    if (PahubSelect(kChRight)) {
        s_right_addr = find_oled_addr();
        s_right_ok = (s_right_addr != 0) && oled_init(s_right_addr);
        ESP_LOGI(TAG, "CH1 oled 0x%02X %s", s_right_addr, s_right_ok ? "ok" : "fail");
    }
    pick_look();
    pick_mouth();
    s_eyes_open = true;
    show_eyes(true);
    const uint32_t now = static_cast<uint32_t>(esp_timer_get_time() / 1000ULL);
    s_blink_at = now + 2500;
    s_look_at = now + 400 + (esp_random() % 1400);
    PahubUnlock();
    return s_left_ok || s_right_ok;
}

void EyesDeinit()
{
    s_left_ok = false;
    s_right_ok = false;
}

bool EyesLeftOk()
{
    return s_left_ok;
}

bool EyesRightOk()
{
    return s_right_ok;
}

bool EyesMouthSmile()
{
    return s_mouth_smile.load(std::memory_order_relaxed);
}

void EyesTick(uint32_t now_ms)
{
    if (!s_left_ok && !s_right_ok) {
        return;
    }
    PahubLock();
    bool dirty = false;
    if (now_ms >= s_look_at && s_eyes_open) {
        pick_look();
        pick_mouth();
        dirty = true;
        s_look_at = now_ms + 400 + (esp_random() % 1400);
    }
    if (now_ms >= s_blink_at) {
        if (s_eyes_open) {
            s_eyes_open = false;
            dirty = true;
            s_blink_at = now_ms + 90 + (esp_random() % 80);
        } else {
            s_eyes_open = true;
            dirty = true;
            s_blink_at = now_ms + 1800 + (esp_random() % 2200);
            s_look_at = now_ms + 200;
        }
    }
    if (dirty) {
        show_eyes(s_eyes_open);
    }
    PahubUnlock();
}

}  // namespace stackchan::obake
