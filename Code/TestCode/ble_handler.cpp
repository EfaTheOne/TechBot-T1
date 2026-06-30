#include "ble_handler.h"
#include "config.h"
#include "power_mgmt.h"
#include <bluefruit.h>

// NUS (Nordic UART Service)
BLEUart bleuart;

// Custom GPS Telemetry Service (UUID: 0x1819 - Location and Navigation, but we use a custom one for simplicity)
// Using 128-bit custom UUID
const uint8_t GPS_SERVICE_UUID[] = {
    0x23, 0xD1, 0xBC, 0xEA, 0x5F, 0x78, 0x23, 0x15, 
    0xDE, 0xEF, 0x12, 0x12, 0x00, 0x00, 0x00, 0x00
};
const uint8_t GPS_CHAR_UUID[] = {
    0x23, 0xD1, 0xBC, 0xEA, 0x5F, 0x78, 0x23, 0x15, 
    0xDE, 0xEF, 0x12, 0x12, 0x01, 0x00, 0x00, 0x00
};

BLEService gpsService(GPS_SERVICE_UUID);
BLECharacteristic gpsChar(GPS_CHAR_UUID);

static bool is_connected = false;
static GpsData current_gps;

// External callback to LoRa module for text forwarding
extern void lora_send_message(const char* text);
// External callback to UI module to add message
extern void ui_add_message(const char* text, uint8_t sender);
extern void ui_wake_display();

// Connection callbacks
void connect_callback(uint16_t conn_handle) {
    is_connected = true;
    power_led_flash();
}

void disconnect_callback(uint16_t conn_handle, uint8_t reason) {
    is_connected = false;
    // Resume slow advertising automatically handled by restartOnDisconnect
}

// NUS RX callback
void bleuart_rx_callback(uint16_t conn_hdl) {
    char buf[MAX_MSG_LEN] = {0};
    int len = 0;
    
    while (bleuart.available() && len < MAX_MSG_LEN - 1) {
        buf[len++] = (char)bleuart.read();
    }
    buf[len] = '\0';
    
    if (len > 0) {
        power_led_flash();
        // Add to local UI buffer
        ui_add_message(buf, 0x00); // 0x00 = phone
        ui_wake_display();
        
        // Forward over LoRa
        lora_send_message(buf);
    }
}

// GPS Characteristic write callback
void gps_write_callback(uint16_t conn_hdl, BLECharacteristic* chr, uint8_t* data, uint16_t len) {
    // Expected 12 bytes: int32 lat, int32 lon, int32 alt
    if (len >= 12) {
        memcpy(&current_gps.lat_1e7, data, 4);
        memcpy(&current_gps.lon_1e7, data + 4, 4);
        memcpy(&current_gps.alt_cm, data + 8, 4);
        current_gps.timestamp_ms = millis();
        current_gps.valid = true;
        power_led_flash();
    }
}

void ble_init() {
    current_gps.valid = false;

    // Config Bluefruit
    Bluefruit.begin();
    Bluefruit.setTxPower(BLE_TX_POWER);
    Bluefruit.setName(BLE_DEVICE_NAME);
    
    // Callbacks
    Bluefruit.Periph.setConnectCallback(connect_callback);
    Bluefruit.Periph.setDisconnectCallback(disconnect_callback);

    // Security - Just Works bonding
    Bluefruit.Security.setIOCaps(true, false, false); // DisplayYesNo but we use Just Works

    // Start NUS
    bleuart.begin();
    bleuart.setRxCallback(bleuart_rx_callback);

    // Start GPS Service
    gpsService.begin();
    
    gpsChar.setProperties(CHR_PROPS_WRITE | CHR_PROPS_WRITE_WO_RESP);
    gpsChar.setPermission(SECMODE_OPEN, SECMODE_OPEN); // Allow open access for ease, could be SECMODE_ENC_NO_MITM
    gpsChar.setFixedLen(12);
    gpsChar.setWriteCallback(gps_write_callback);
    gpsChar.begin();

    // Set up advertising
    Bluefruit.Advertising.addFlags(BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE);
    Bluefruit.Advertising.addTxPower();
    Bluefruit.Advertising.addService(bleuart);
    Bluefruit.Advertising.addService(gpsService);
    Bluefruit.ScanResponse.addName();
    
    Bluefruit.Advertising.restartOnDisconnect(true);
    
    // Slow interval for power saving (1.2 seconds)
    // 1920 * 0.625ms = 1200ms
    Bluefruit.Advertising.setInterval(1920, 1920);
    Bluefruit.Advertising.setFastTimeout(0); // No fast timeout, always slow
    
    Bluefruit.Advertising.start(0); 
}

void ble_update() {
    // Callbacks handle the events, nothing to poll here for Adafruit BSP
}

bool ble_is_connected() {
    return is_connected;
}

void ble_send_text(const char* text) {
    if (is_connected) {
        bleuart.print(text);
    }
}

GpsData ble_get_gps_data() {
    return current_gps;
}
