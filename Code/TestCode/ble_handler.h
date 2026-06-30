#pragma once

#include <Arduino.h>
#include "message.h"

void ble_init();
void ble_update();
bool ble_is_connected();
void ble_send_text(const char* text);
GpsData ble_get_gps_data();
