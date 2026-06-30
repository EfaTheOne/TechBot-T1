#pragma once

#include <Arduino.h>
#include "message.h"

void lora_init();
void lora_update();
void lora_send_telemetry(bool emergency = false);
void lora_send_message(const char* text);

// Getters for UI
uint32_t lora_get_last_tx_time();
uint32_t lora_get_last_rx_time();
