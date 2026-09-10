/*
 * アバターの目口を隠し、白キャンバスに ∪/∩ と cm / ALT_CM を描く。
 */
#include "obake_mouth_ui.h"

#include "obake_config.h"
#include "obake_eyes.h"
#include "obake_pahub.h"
#include "obake_tof.h"
#include "obake_wake_config.h"

#include <stackchan/avatar/skins/default/default.h>
#include <stackchan/stackchan.h>

#include <esp_heap_caps.h>
#include <esp_log.h>
#include <lvgl.h>

namespace stackchan::obake {
namespace {

const char* TAG = "obake_mouth";
constexpr int kW = 320;
constexpr int kH = 240;
constexpr uint16_t kWhite = 0xFFFF;
constexpr uint16_t kBlack = 0x0000;

lv_obj_t* s_canvas = nullptr;
lv_obj_t* s_label_tl = nullptr;
lv_obj_t* s_label_tr = nullptr;
lv_obj_t* s_label_bl = nullptr;
uint16_t* s_buf = nullptr;
int s_drawn_cm = -2;
bool s_drawn_smile = false;

void fill_ellipse(uint16_t* buf, int cx, int cy, int rx, int ry, uint16_t color)
{
    if (rx <= 0 || ry <= 0) {
        return;
    }
    const long rx2 = static_cast<long>(rx) * rx;
    const long ry2 = static_cast<long>(ry) * ry;
    const long r2 = rx2 * ry2;
    for (int y = -ry; y <= ry; ++y) {
        const int py = cy + y;
        if (py < 0 || py >= kH) {
            continue;
        }
        const long yy = static_cast<long>(y) * y * rx2;
        for (int x = -rx; x <= rx; ++x) {
            const int px = cx + x;
            if (px < 0 || px >= kW) {
                continue;
            }
            const long xx = static_cast<long>(x) * x * ry2;
            if (xx + yy <= r2) {
                buf[py * kW + px] = color;
            }
        }
    }
}

void paint_mouth(bool smile)
{
    if (!s_buf || !s_canvas) {
        return;
    }
    for (int i = 0; i < kW * kH; ++i) {
        s_buf[i] = kWhite;
    }
    const int cx = kW / 2;
    const int cy = kH / 2 + 16;
    const int rx = 70;
    const int ry = 36;
    if (smile) {
        fill_ellipse(s_buf, cx, cy + 12, rx, ry, kBlack);
        fill_ellipse(s_buf, cx, cy - 10, rx + 6, ry, kWhite);
    } else {
        fill_ellipse(s_buf, cx, cy - 12, rx, ry, kBlack);
        fill_ellipse(s_buf, cx, cy + 10, rx + 6, ry, kWhite);
    }
    lv_obj_invalidate(s_canvas);
}

void update_status_labels(int cm)
{
    if (!s_label_tl || !s_label_tr || !s_label_bl) {
        return;
    }
    lv_label_set_text_fmt(s_label_tl, "%s\nP%s T%s L%s R%s", kObakeUiLabel, PahubOk() ? "ok" : "--",
                          TofOk() ? "ok" : "--", EyesLeftOk() ? "ok" : "--", EyesRightOk() ? "ok" : "--");
    if (cm >= 0) {
        lv_label_set_text_fmt(s_label_tr, "%3d cm", cm);
        lv_label_set_text_fmt(s_label_bl, "ALT_CM=%d", cm);
    } else {
        lv_label_set_text(s_label_tr, "-- cm");
        lv_label_set_text(s_label_bl, "ALT_CM=----");
    }
}

void set_default_face_hidden(bool hide)
{
    auto& stackchan = GetStackChan();
    if (!stackchan.hasAvatar()) {
        return;
    }
    stackchan.avatar().leftEye().setVisible(!hide);
    stackchan.avatar().rightEye().setVisible(!hide);
    stackchan.avatar().mouth().setVisible(!hide);
}

/** 吹き出しは不要。毎フレーム消す（SetStatus で再表示されても隠す） */
void hide_speech_bubble()
{
    auto& stackchan = GetStackChan();
    if (!stackchan.hasAvatar()) {
        return;
    }
    stackchan.avatar().clearSpeech();
    if (auto* bubble = stackchan.avatar().getKeyElements().speechBubble.get()) {
        bubble->setVisible(false);
    }
}

lv_obj_t* make_label(lv_obj_t* parent, lv_align_t align, int x, int y)
{
    lv_obj_t* lab = lv_label_create(parent);
    lv_obj_set_style_text_color(lab, lv_color_hex(0x555555), 0);
    lv_obj_set_style_text_font(lab, &lv_font_montserrat_16, 0);
    lv_obj_align(lab, align, x, y);
    // 距離ラベルはタップを奪わない（下スワイプホーム用）
    lv_obj_clear_flag(lab, LV_OBJ_FLAG_CLICKABLE);
    return lab;
}

}  // namespace

void MouthUiCreate()
{
    // 再 Start 時も標準目口が被らないよう、既存キャンバスなら隠し直しだけする
    if (s_canvas) {
        set_default_face_hidden(true);
        hide_speech_bubble();
        return;
    }
    auto& stackchan = GetStackChan();
    if (!stackchan.hasAvatar()) {
        ESP_LOGW(TAG, "no avatar");
        return;
    }
    auto* def = dynamic_cast<stackchan::avatar::DefaultAvatar*>(&stackchan.avatar());
    if (!def || !def->getPanel()) {
        ESP_LOGW(TAG, "DefaultAvatar panel missing");
        return;
    }

    set_default_face_hidden(true);
    hide_speech_bubble();
    def->getPanel()->setBgColor(lv_color_white());

    lv_obj_t* parent = def->getPanel()->get();
    const size_t bytes = static_cast<size_t>(kW) * kH * sizeof(uint16_t);
    s_buf = static_cast<uint16_t*>(heap_caps_malloc(bytes, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
    if (!s_buf) {
        s_buf = static_cast<uint16_t*>(heap_caps_malloc(bytes, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT));
    }
    if (!s_buf) {
        // 確保失敗時のみ標準顔を戻す（口 UI 未表示のため）
        ESP_LOGE(TAG, "canvas buffer alloc failed");
        set_default_face_hidden(false);
        return;
    }

    s_canvas = lv_canvas_create(parent);
    lv_canvas_set_buffer(s_canvas, s_buf, kW, kH, LV_COLOR_FORMAT_RGB565);
    lv_obj_set_size(s_canvas, kW, kH);
    lv_obj_align(s_canvas, LV_ALIGN_CENTER, 0, 0);
    // キャンバスは背面。下スワイプ／タップを奪わない
    lv_obj_clear_flag(s_canvas, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_move_background(s_canvas);

    s_label_tl = make_label(parent, LV_ALIGN_TOP_LEFT, 6, 6);
    s_label_tr = make_label(parent, LV_ALIGN_TOP_RIGHT, -6, 6);
    s_label_bl = make_label(parent, LV_ALIGN_BOTTOM_LEFT, 6, -6);
    lv_obj_set_style_text_color(s_label_tr, lv_color_black(), 0);
    lv_obj_move_foreground(s_label_tl);
    lv_obj_move_foreground(s_label_tr);
    lv_obj_move_foreground(s_label_bl);

    s_drawn_cm = -2;
    s_drawn_smile = true;
    paint_mouth(true);
    update_status_labels(-1);
    ESP_LOGI(TAG, "mouth UI ready (wake=%s)", kWakeDisplayName);
}

void MouthUiDestroy()
{
    // RuntimeStop / OnHalInit からは呼ばない想定。呼んでも標準目口は戻さない
    // （CUSTOM 中に Stack-chan 標準顔でおばけ口を潰さない）
    if (s_label_tl) {
        lv_obj_del(s_label_tl);
        s_label_tl = nullptr;
    }
    if (s_label_tr) {
        lv_obj_del(s_label_tr);
        s_label_tr = nullptr;
    }
    if (s_label_bl) {
        lv_obj_del(s_label_bl);
        s_label_bl = nullptr;
    }
    if (s_canvas) {
        lv_obj_del(s_canvas);
        s_canvas = nullptr;
    }
    if (s_buf) {
        heap_caps_free(s_buf);
        s_buf = nullptr;
    }
    // set_default_face_hidden(false) はしない — パネルは白のまま
    s_drawn_cm = -2;
}

void MouthUiUpdate()
{
    if (!s_canvas || !s_buf) {
        return;
    }
    // 標準顔が何かで再表示されても口を優先（毎フレーム隠し直し）
    set_default_face_hidden(true);
    hide_speech_bubble();
    const bool smile = EyesMouthSmile();
    const int cm = TofLastCm();
    if (smile != s_drawn_smile) {
        paint_mouth(smile);
        s_drawn_smile = smile;
    }
    if (cm != s_drawn_cm) {
        s_drawn_cm = cm;
        update_status_labels(cm);
    }
}

}  // namespace stackchan::obake
