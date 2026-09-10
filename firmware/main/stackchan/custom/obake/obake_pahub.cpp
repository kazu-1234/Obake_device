/*
 * Port.A I2C（GPIO2/1）と PaHub 0x70。
 */
#include "obake_pahub.h"

#include "obake_config.h"

#include <driver/gpio.h>
#include <driver/i2c_master.h>
#include <esp_log.h>
#include <esp_rom_sys.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <freertos/task.h>

namespace stackchan::obake {
namespace {

const char* TAG = "obake_pahub";
i2c_master_bus_handle_t s_bus = nullptr;
SemaphoreHandle_t s_mutex = nullptr;
bool s_ok = false;
/** HUD 用: ok / bus / probe / -- */
const char* s_status_tag = "--";

/** 同一アドレスの add/remove 連打を避けるための小さなキャッシュ */
struct DevCache {
    uint8_t addr = 0;
    i2c_master_dev_handle_t handle = nullptr;
};
DevCache s_devs[4] = {};

/** PaHub probe 回数・間隔（配線・電源立ち上がり待ち） */
constexpr int kPahubProbeTries = 8;
constexpr uint32_t kPahubSettleMs = 100;
constexpr uint32_t kPahubProbeGapMs = 40;

i2c_master_dev_handle_t get_dev(uint8_t addr)
{
    for (auto& d : s_devs) {
        if (d.handle && d.addr == addr) {
            return d.handle;
        }
    }
    i2c_device_config_t cfg = {};
    cfg.dev_addr_length = I2C_ADDR_BIT_LEN_7;
    cfg.device_address = addr;
    cfg.scl_speed_hz = kI2cHz;
    i2c_master_dev_handle_t dev = nullptr;
    if (i2c_master_bus_add_device(s_bus, &cfg, &dev) != ESP_OK) {
        return nullptr;
    }
    for (auto& d : s_devs) {
        if (!d.handle) {
            d.addr = addr;
            d.handle = dev;
            return dev;
        }
    }
    // キャッシュ満杯: 先頭を入れ替え
    if (s_devs[0].handle) {
        i2c_master_bus_rm_device(s_devs[0].handle);
    }
    s_devs[0].addr = addr;
    s_devs[0].handle = dev;
    return dev;
}

void clear_devs()
{
    for (auto& d : s_devs) {
        if (d.handle) {
            i2c_master_bus_rm_device(d.handle);
            d.handle = nullptr;
            d.addr = 0;
        }
    }
}

/** レーザー等で GPIO が OUTPUT 化されたあとに I2C を取れるようリセット */
void reset_port_a_pins()
{
    gpio_reset_pin(kPortASda);
    gpio_reset_pin(kPortAScl);
}

bool probe_pahub_with_retries()
{
    s_ok = false;
    for (int try_i = 0; try_i < kPahubProbeTries && !s_ok; ++try_i) {
        s_ok = PahubProbe(kPahubAddr);
        if (!s_ok) {
            ESP_LOGW(TAG, "PaHub 0x%02X probe miss (%d/%d)", kPahubAddr, try_i + 1, kPahubProbeTries);
            vTaskDelay(pdMS_TO_TICKS(kPahubProbeGapMs));
        }
    }
    s_status_tag = s_ok ? "ok" : "probe";
    return s_ok;
}

}  // namespace

bool PahubInit()
{
    // バスはあるが probe 失敗のまま固まるのを防ぎ、再 probe する
    if (s_bus) {
        if (s_ok) {
            return true;
        }
        ESP_LOGW(TAG, "PaHub bus up but not ok — re-probe");
        probe_pahub_with_retries();
        ESP_LOGI(TAG, "PaHub 0x%02X %s (re-probe)", kPahubAddr, s_ok ? "ok" : "missing");
        return s_ok;
    }
    if (!s_mutex) {
        s_mutex = xSemaphoreCreateMutex();
    }
    // GPIO2 がレーザー OUTPUT だと new_master_bus が失敗しやすい
    reset_port_a_pins();
    i2c_master_bus_config_t bus_cfg = {};
    bus_cfg.i2c_port = static_cast<i2c_port_t>(kPortAI2cPort);
    bus_cfg.sda_io_num = kPortASda;
    bus_cfg.scl_io_num = kPortAScl;
    bus_cfg.clk_source = I2C_CLK_SRC_DEFAULT;
    bus_cfg.glitch_ignore_cnt = 7;
    bus_cfg.flags.enable_internal_pullup = true;
    esp_err_t err = i2c_new_master_bus(&bus_cfg, &s_bus);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "i2c_new_master_bus failed: %s", esp_err_to_name(err));
        s_bus = nullptr;
        s_ok = false;
        s_status_tag = "bus";
        return false;
    }
    vTaskDelay(pdMS_TO_TICKS(kPahubSettleMs));
    probe_pahub_with_retries();
    ESP_LOGI(TAG, "PaHub 0x%02X %s (SDA=GPIO%d SCL=GPIO%d port=%d)", kPahubAddr, s_ok ? "ok" : "missing",
             static_cast<int>(kPortASda), static_cast<int>(kPortAScl), kPortAI2cPort);
    return s_ok;
}

void PahubDeinit()
{
    clear_devs();
    if (s_bus) {
        i2c_del_master_bus(s_bus);
        s_bus = nullptr;
    }
    if (s_mutex) {
        vSemaphoreDelete(s_mutex);
        s_mutex = nullptr;
    }
    s_ok = false;
    s_status_tag = "--";
}

bool PahubOk()
{
    return s_ok;
}

const char* PahubStatusTag()
{
    return s_status_tag ? s_status_tag : "--";
}

bool PahubPortAInUse()
{
    // バス確保中は GPIO2 をレーザーに渡さない
    return s_bus != nullptr;
}

void PahubLock()
{
    if (s_mutex) {
        xSemaphoreTake(s_mutex, portMAX_DELAY);
    }
}

void PahubUnlock()
{
    if (s_mutex) {
        xSemaphoreGive(s_mutex);
    }
}

bool PahubSelect(uint8_t ch)
{
    if (!s_bus || ch > 7) {
        return false;
    }
    const uint8_t mask = static_cast<uint8_t>(1u << ch);
    const bool ok = PahubWrite(kPahubAddr, &mask, 1);
    esp_rom_delay_us(50);
    return ok;
}

bool PahubProbe(uint8_t addr)
{
    if (!s_bus) {
        return false;
    }
    return i2c_master_probe(s_bus, addr, pdMS_TO_TICKS(50)) == ESP_OK;
}

bool PahubWrite(uint8_t addr, const uint8_t* data, size_t len)
{
    if (!s_bus || !data || len == 0) {
        return false;
    }
    i2c_master_dev_handle_t dev = get_dev(addr);
    if (!dev) {
        return false;
    }
    return i2c_master_transmit(dev, data, len, pdMS_TO_TICKS(100)) == ESP_OK;
}

bool PahubWriteRead(uint8_t addr, const uint8_t* wdata, size_t wlen, uint8_t* rdata, size_t rlen)
{
    if (!s_bus || !wdata || wlen == 0 || !rdata || rlen == 0) {
        return false;
    }
    i2c_master_dev_handle_t dev = get_dev(addr);
    if (!dev) {
        return false;
    }
    return i2c_master_transmit_receive(dev, wdata, wlen, rdata, rlen, pdMS_TO_TICKS(100)) == ESP_OK;
}

}  // namespace stackchan::obake

/** Hal から C リンケージで参照（GPIO2 レーザー競合防止） */
extern "C" bool stackchan_obake_pahub_port_a_in_use()
{
    return stackchan::obake::PahubPortAInUse();
}
