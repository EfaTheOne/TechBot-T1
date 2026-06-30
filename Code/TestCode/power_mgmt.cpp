#include "power_mgmt.h"
#include "config.h"
#include <bluefruit.h>

static uint32_t led_start_time = 0;
static bool led_is_on = false;

void power_init() {
    // Configure DC/DC converter for maximum efficiency
    sd_power_dcdc_mode_set(NRF_POWER_DCDC_ENABLE);
    
    // Set SoftDevice low-power mode
    sd_power_mode_set(NRF_POWER_MODE_LOWPWR);

    // Disable unused peripherals to save power
    NRF_UARTE0->ENABLE = 0;
    NRF_SAADC->ENABLE = 0;
    NRF_PWM0->ENABLE = 0;

    pinMode(PIN_LED1, OUTPUT);
    digitalWrite(PIN_LED1, LOW); // LEDs off by default
}

void power_idle_sleep() {
    // We use SoftDevice sd_app_evt_wait() to enter System ON idle sleep.
    // The CPU will wake up on any interrupt (BLE, GPIO, Timer).
    // FreeRTOS tickless idle also handles sleep implicitly in Adafruit BSP,
    // but this explicitly waits for an event.
    sd_app_evt_wait();
}

void power_led_flash() {
    digitalWrite(PIN_LED1, HIGH);
    led_start_time = millis();
    led_is_on = true;
}

void power_update() {
    // Non-blocking LED off check
    if (led_is_on) {
        if ((uint32_t)(millis() - led_start_time) >= LED_FLASH_DURATION_MS) {
            digitalWrite(PIN_LED1, LOW);
            led_is_on = false;
        }
    }
}

uint16_t power_get_batt_mv() {
    // The Feather nRF52840 has a voltage divider on A6 (PIN_VBAT).
    // It divides by 2. Reference is 3.6V (using internal reference) and 12-bit ADC.
    // To measure, we must briefly enable SAADC, measure, and disable.
    
    NRF_SAADC->ENABLE = 1;
    analogReference(AR_INTERNAL_3_0);
    analogReadResolution(12);
    
    int raw = analogRead(PIN_VBAT);
    
    NRF_SAADC->ENABLE = 0; // Turn off to save power

    // Convert raw to mV. 3.0V ref, 12-bit (4096), divider = 1/2.
    // mV = raw * 3000 * 2 / 4096
    return (raw * 6000) / 4096;
}

uint8_t power_get_batt_percent() {
    uint16_t mv = power_get_batt_mv();
    if (mv >= 4200) return 100;
    if (mv <= 3200) return 0;
    return (uint8_t)((mv - 3200) / 10); // simple linear map
}

uint32_t power_get_uptime_s() {
    return millis() / 1000;
}
