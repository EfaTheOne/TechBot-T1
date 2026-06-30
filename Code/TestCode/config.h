#pragma once

#include <Arduino.h>

// -----------------------------------------------------------------------------
// Pin Configuration
// -----------------------------------------------------------------------------

// I2C OLED (SSD1306)
#define PIN_OLED_SDA      13  // P0.13
#define PIN_OLED_SCL      14  // P0.14

// Status LEDs
#define PIN_LED1          35  // P1.03
// #define PIN_LED2       36  // P1.04 - CONFLICT WITH NSS. Disabled in software.
#warning "Pin 36 is mapped to both LED2 and LoRa NSS. LED2 is disabled."

// Push Buttons
#define PIN_BUTTON1       33  // P1.01
#define PIN_BUTTON2       34  // P1.02

// Internal SPI to SX1262
#define PIN_LORA_MISO     37  // P1.05
#define PIN_LORA_MOSI     38  // P1.06
#define PIN_LORA_SCK      39  // P1.07
#define PIN_LORA_NSS      36  // P1.04

// SX1262 Control
#define PIN_LORA_RESET    45  // P1.13
#define PIN_LORA_BUSY     46  // P1.14
#define PIN_LORA_DIO1     47  // P1.15

// Antenna RF Switch
#define PIN_LORA_RF_SW    21  // P0.21

// Battery Voltage Pin (Internal nRF52840 divider on A6 usually)
// If standard Adafruit nRF52840, VBAT is on A6 (pin 29 / P0.29)
#define PIN_VBAT          29

// -----------------------------------------------------------------------------
// LoRa Configuration
// -----------------------------------------------------------------------------
#define LORA_FREQUENCY    915000000 // 915 MHz (US ISM)
#define LORA_TX_POWER     14        // +14 dBm
#define LORA_SF           9         // Spreading Factor 9
#define LORA_BW           0         // 0 = 125 kHz
#define LORA_CR           1         // 1 = 4/5
#define LORA_PREAMBLE     8         // Preamble length

// Low Power RX Duty Cycle Configuration
// Using 15.625 us steps: 
// RX Window: 500ms = 32000 steps
// Sleep Window: 3000ms = 192000 steps
#define LORA_RX_TIME      32000
#define LORA_SLEEP_TIME   192000

// -----------------------------------------------------------------------------
// BLE Configuration
// -----------------------------------------------------------------------------
#define BLE_DEVICE_NAME   "TechBotT1_Node"
#define BLE_TX_POWER      -4        // -4 dBm
#define BLE_PIN_CODE      "123456"

// -----------------------------------------------------------------------------
// Application Configuration
// -----------------------------------------------------------------------------
#define NODE_ID           0x01      // Compile-time node ID
#define PROTOCOL_VERSION  0x01

#define OLED_WAKE_DURATION_MS     10000 // 10 seconds
#define TELEMETRY_INTERVAL_MS     900000 // 15 minutes (900,000 ms)
#define LED_FLASH_DURATION_MS     50    // 50 ms
#define BUTTON_DEBOUNCE_MS        200   // 200 ms
