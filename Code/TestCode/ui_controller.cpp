#include "ui_controller.h"
#include "config.h"
#include "message.h"
#include "power_mgmt.h"
#include "ble_handler.h"
#include "lora_handler.h"
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

static uint32_t display_wake_time = 0;
static bool display_is_on = false;

volatile bool btn1_pressed = false;
volatile bool btn2_pressed = false;
static uint32_t last_btn_time = 0;

static int current_screen = 0;
#define NUM_SCREENS 3

static MessageBuffer msg_buffer;

void btn1_isr() { btn1_pressed = true; }
void btn2_isr() { btn2_pressed = true; }

void draw_status_screen() {
    display.clearDisplay();
    display.setCursor(0, 0);
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    
    display.print("TechBotT1 Node 0x");
    if (NODE_ID < 16) display.print("0");
    display.println(NODE_ID, HEX);
    display.drawLine(0, 9, 128, 9, SSD1306_WHITE);
    
    display.setCursor(0, 14);
    display.print("BLE: ");
    display.println(ble_is_connected() ? "Connected" : "Adv/Discon");
    
    display.print("Uptime: ");
    display.print(power_get_uptime_s());
    display.println("s");
    
    display.print("Last TX: ");
    uint32_t tx = lora_get_last_tx_time();
    if(tx > 0) { display.print((millis()-tx)/1000); display.println("s ago"); } else { display.println("Never"); }
    
    display.print("Last RX: ");
    uint32_t rx = lora_get_last_rx_time();
    if(rx > 0) { display.print((millis()-rx)/1000); display.println("s ago"); } else { display.println("Never"); }
}

void draw_telemetry_screen() {
    display.clearDisplay();
    display.setCursor(0, 0);
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    
    display.println("Telemetry & GPS");
    display.drawLine(0, 9, 128, 9, SSD1306_WHITE);
    
    display.setCursor(0, 14);
    display.print("Batt: ");
    display.print(power_get_batt_mv());
    display.print("mV (");
    display.print(power_get_batt_percent());
    display.println("%)");
    
    GpsData gps = ble_get_gps_data();
    if (gps.valid) {
        display.print("Lat: "); display.println(gps.lat_1e7 / 10000000.0, 5);
        display.print("Lon: "); display.println(gps.lon_1e7 / 10000000.0, 5);
        display.print("Alt: "); display.print(gps.alt_cm / 100.0, 1); display.println("m");
        uint32_t age = (millis() - gps.timestamp_ms) / 1000;
        display.print("Age: "); display.print(age); display.println("s");
    } else {
        display.println("GPS: No fix/data");
    }
}

void draw_messages_screen() {
    display.clearDisplay();
    display.setCursor(0, 0);
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    
    display.println("Recent Messages");
    display.drawLine(0, 9, 128, 9, SSD1306_WHITE);
    
    int count = msg_buffer.getCount();
    if (count == 0) {
        display.setCursor(0, 20);
        display.println("No messages");
    } else {
        int y = 12;
        // Show up to 3 messages to fit on screen
        for (int i = 0; i < min(count, 3); i++) {
            const TextMessage* msg = msg_buffer.getRecent(i);
            if (msg) {
                display.setCursor(0, y);
                if (msg->sender_id == 0) display.print("Ph: ");
                else { display.print("["); display.print(msg->sender_id, HEX); display.print("]: "); }
                
                // Print a truncated version if it's too long
                char trunc[18];
                strncpy(trunc, msg->text, 17);
                trunc[17] = '\0';
                display.println(trunc);
                y += 10;
            }
        }
    }
}

void ui_refresh_screen() {
    if (!display_is_on) return;
    
    if (current_screen == 0) draw_status_screen();
    else if (current_screen == 1) draw_telemetry_screen();
    else if (current_screen == 2) draw_messages_screen();
    
    display.display();
}

void ui_init() {
    // Buttons
    pinMode(PIN_BUTTON1, INPUT_PULLUP);
    pinMode(PIN_BUTTON2, INPUT_PULLUP);
    
    attachInterrupt(digitalPinToInterrupt(PIN_BUTTON1), btn1_isr, FALLING);
    attachInterrupt(digitalPinToInterrupt(PIN_BUTTON2), btn2_isr, FALLING);
    
    // I2C for OLED
    Wire.setPins(PIN_OLED_SDA, PIN_OLED_SCL);
    Wire.begin();
    
    if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
        // Init failed, can't really do much.
    } else {
        display.clearDisplay();
        display.display();
        // Start powered off
        display.ssd1306_command(SSD1306_DISPLAYOFF);
    }
}

void ui_wake_display() {
    display_wake_time = millis();
    if (!display_is_on) {
        display.ssd1306_command(SSD1306_DISPLAYON);
        display_is_on = true;
    }
    ui_refresh_screen();
}

void ui_add_message(const char* text, uint8_t sender) {
    msg_buffer.add(text, sender);
}

void ui_update() {
    uint32_t now = millis();
    
    // Auto-sleep OLED
    if (display_is_on) {
        if ((uint32_t)(now - display_wake_time) >= OLED_WAKE_DURATION_MS) {
            display.ssd1306_command(SSD1306_DISPLAYOFF);
            display_is_on = false;
        }
    }
    
    // Handle button 1 (Screen change / Wake)
    if (btn1_pressed) {
        btn1_pressed = false;
        if ((uint32_t)(now - last_btn_time) >= BUTTON_DEBOUNCE_MS) {
            last_btn_time = now;
            if (!display_is_on) {
                ui_wake_display();
            } else {
                current_screen = (current_screen + 1) % NUM_SCREENS;
                ui_wake_display();
            }
        }
    }
    
    // Handle button 2 (Emergency TX)
    if (btn2_pressed) {
        btn2_pressed = false;
        if ((uint32_t)(now - last_btn_time) >= BUTTON_DEBOUNCE_MS) {
            last_btn_time = now;
            
            // Wake display and show EMERGENCY
            if (!display_is_on) {
                display.ssd1306_command(SSD1306_DISPLAYON);
                display_is_on = true;
            }
            display_wake_time = now;
            display.clearDisplay();
            display.setCursor(0, 20);
            display.setTextSize(2);
            display.setTextColor(SSD1306_WHITE);
            display.println("EMERGENCY");
            display.println(" TX SENT!");
            display.display();
            
            // Send emergency packet
            lora_send_telemetry(true);
        }
    }
}
