#include <Arduino.h>
#include "power_mgmt.h"
#include "ui_controller.h"
#include "ble_handler.h"
#include "lora_handler.h"

// Forward declaration of idle callback used by FreeRTOS tickless idle
// Adafruit BSP calls this when all tasks are idle
extern "C" void rtos_idle_callback(void) {
    // Don't block here.
}

void setup() {
    // 1. Initialize power management (disables unused peripherals, sets low power mode)
    power_init();
    
    // 2. Initialize UI (OLED, Buttons, I2C)
    // Display starts powered off.
    ui_init();
    
    // 3. Initialize BLE stack
    ble_init();
    
    // 4. Initialize LoRa radio and start RX duty cycle
    lora_init();
}

void loop() {
    // Event-driven check pattern. No blocking delays!
    // Each update function is fast and non-blocking, driven by flags and millis() timers.
    
    power_update(); // Check LED flash timer
    ui_update();    // Check button flags and OLED sleep timer
    ble_update();   // (Empty right now as Adafruit BLE callbacks are async/interrupt driven)
    lora_update();  // Check for telemetry broadcast timer
    
    // Put MCU into System ON idle sleep.
    // Wakes upon any interrupt (BLE, GPIO buttons, Timer, Radio DIO1)
    power_idle_sleep();
}
