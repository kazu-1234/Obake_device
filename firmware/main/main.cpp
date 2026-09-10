/*
 * Obake_device firmware v0.3.1
 * 対象: M5Stack CoreS3（ESP32-S3）。ESP-IDF のみ。
 * 会話の本線は Stack-chan と同じ xiaozhi-esp32（WebSocket + Opus）。
 * このファイルは土台。Xiaozhi の Application::Run() は次でつなぐ。
 */
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char* TAG = "obake";

extern "C" void app_main(void) {
    ESP_LOGI(TAG, "Obake v%s (ESP-IDF / CoreS3)", FIRMWARE_VERSION);
    ESP_LOGI(TAG, "Voice path: Xiaozhi like Stack-chan");
    ESP_LOGI(TAG, "Grove eyes / ToF / Catch: port after Xiaozhi boots");

    while (true) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
