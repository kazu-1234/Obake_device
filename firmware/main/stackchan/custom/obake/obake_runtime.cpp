/*
 * PaHub・目・ToF・口を Custom セッションに接続。サーボは扱わない。
 */
#include "obake_runtime.h"

#include "obake_config.h"
#include "obake_eyes.h"
#include "obake_mouth_ui.h"
#include "obake_pahub.h"
#include "obake_tof.h"
#include "obake_wake_config.h"

#include <stackchan/stackchan.h>  // GetStackChan（modifyLock）

#include <atomic>
#include <esp_log.h>
#include <esp_timer.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

namespace stackchan::obake {
namespace {

const char* TAG = "obake_runtime";
bool s_started = false;
TaskHandle_t s_task = nullptr;
std::atomic<bool> s_task_run{false};

uint32_t now_ms()
{
    return static_cast<uint32_t>(esp_timer_get_time() / 1000ULL);
}

void hw_task(void* /*arg*/)
{
    ESP_LOGI(TAG, "hw task start (wake display=%s)", kWakeDisplayName);
    while (s_task_run.load(std::memory_order_relaxed)) {
        const uint32_t t = now_ms();
        EyesTick(t);
        TofTick(t);
        // 周期は kHwTickMs（閉眼最短より短く保つ）
        vTaskDelay(pdMS_TO_TICKS(kHwTickMs));
    }
    s_task = nullptr;
    vTaskDelete(nullptr);
}

/** PaHub → 目 → ToF。失敗時は一度だけ deinit して再試行 */
bool start_pahub_chain()
{
    if (PahubInit()) {
        EyesInit();
        TofInit();
        return true;
    }
    ESP_LOGW(TAG, "PaHub miss — retry once");
    vTaskDelay(pdMS_TO_TICKS(120));
    PahubDeinit();
    if (PahubInit()) {
        EyesInit();
        TofInit();
        return true;
    }
    ESP_LOGW(TAG, "PaHub missing — eyes/ToF skipped");
    return false;
}

}  // namespace

void RuntimeStart()
{
    // HW 未起動ならバス・タスクを開始。口 UI は毎回試す（UI ready 後の再入用）
    if (!s_started) {
        s_started = true;
        ESP_LOGI(TAG, "start Obake face HW (wake=%s thr~%d%%)", kWakeDisplayName, kWakeThresholdPercentHint);
        start_pahub_chain();
        s_task_run.store(true, std::memory_order_relaxed);
        // Core 0: I2C。UI/LVGL は Core1 側が多いので分離
        xTaskCreatePinnedToCore(hw_task, "obake_hw", 8192, nullptr, 5, &s_task, 0);
    } else if (!PahubOk()) {
        // 起動済みだが PaHub 未検出なら再試行（配線遅延対策）
        start_pahub_chain();
    }
    MouthUiCreate();
}

void RuntimeOnUiFrame()
{
    if (!s_started) {
        return;
    }
    MouthUiUpdate();
}

void RuntimeOnPreUpdate()
{
    // I2C は hw タスク側。当面サーボは modifyLock でも止める
    if (GetStackChan().hasAvatar()) {
        auto& motion = GetStackChan().motion();
        if (!motion.isModifyLocked()) {
            motion.setModifyLock(true);
        }
    }
}

void RuntimeStop()
{
    if (!s_started) {
        return;
    }
    // I2C / hw タスクだけ止める。口 UI は CUSTOM セッション中ずっと残す
    // （MouthUiDestroy すると標準目口が戻り、おばけ口が消えるため呼ばない）
    s_task_run.store(false, std::memory_order_relaxed);
    for (int i = 0; i < 50 && s_task != nullptr; ++i) {
        vTaskDelay(pdMS_TO_TICKS(20));
    }
    EyesDeinit();
    TofDeinit();
    PahubDeinit();
    s_started = false;
    ESP_LOGI(TAG, "hw stopped (mouth UI kept)");
}

}  // namespace stackchan::obake
