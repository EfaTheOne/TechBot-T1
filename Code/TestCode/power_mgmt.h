#pragma once

#include <Arduino.h>

void power_init();
void power_idle_sleep();
void power_led_flash();
void power_update();
uint16_t power_get_batt_mv();
uint8_t power_get_batt_percent();
uint32_t power_get_uptime_s();
