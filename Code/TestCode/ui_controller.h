#pragma once

#include <Arduino.h>

void ui_init();
void ui_update();
void ui_wake_display();
void ui_add_message(const char* text, uint8_t sender);
