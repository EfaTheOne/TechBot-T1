#include "lora_handler.h"
#include "config.h"
#include "power_mgmt.h"
#include "ble_handler.h"
#include <SX126x-Arduino.h>
#include <SPI.h>

extern void ui_add_message(const char* text, uint8_t sender);
extern void ui_wake_display();

static hw_config hwConfig;
static uint32_t last_tx_time = 0;
static uint32_t last_rx_time = 0;
static uint32_t last_telemetry_time = 0;

void OnTxDone(void);
void OnRxDone(uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr);
void OnTxTimeout(void);
void OnRxTimeout(void);
void OnRxError(void);

RadioEvents_t RadioEvents = {
    .TxDone = OnTxDone,
    .TxTimeout = OnTxTimeout,
    .RxDone = OnRxDone,
    .RxTimeout = OnRxTimeout,
    .RxError = OnRxError,
};

void lora_init() {
    // Custom SPI pins
    SPI.setPins(PIN_LORA_MISO, PIN_LORA_SCK, PIN_LORA_MOSI);

    // Setup RF switch pin
    pinMode(PIN_LORA_RF_SW, OUTPUT);
    digitalWrite(PIN_LORA_RF_SW, LOW); // RX mode by default

    // Config SX1262
    hwConfig.CHIP_TYPE = SX1262_CHIP;
    hwConfig.PIN_LORA_RESET = PIN_LORA_RESET;
    hwConfig.PIN_LORA_NSS = PIN_LORA_NSS;
    hwConfig.PIN_LORA_SCLK = PIN_LORA_SCK;
    hwConfig.PIN_LORA_MISO = PIN_LORA_MISO;
    hwConfig.PIN_LORA_MOSI = PIN_LORA_MOSI;
    hwConfig.PIN_LORA_DIO_1 = PIN_LORA_DIO1;
    hwConfig.PIN_LORA_BUSY = PIN_LORA_BUSY;
    
    // We are manually controlling the RF switch via pin 21, not using DIO2 or TXEN/RXEN from library
    hwConfig.RADIO_TXEN = -1;
    hwConfig.RADIO_RXEN = -1;
    hwConfig.USE_DIO2_ANT_SWITCH = false;
    hwConfig.USE_DIO3_TCXO = false;
    hwConfig.USE_DIO3_ANT_SWITCH = false;

    lora_hardware_init(hwConfig);
    Radio.Init(&RadioEvents);

    Radio.SetChannel(LORA_FREQUENCY);

    Radio.SetTxConfig(
        MODEM_LORA, LORA_TX_POWER, 0, LORA_BW, LORA_SF, LORA_CR,
        LORA_PREAMBLE, false, true, 0, 0, false, 3000
    );

    Radio.SetRxConfig(
        MODEM_LORA, LORA_BW, LORA_SF, LORA_CR, 0, LORA_PREAMBLE,
        0, false, 0, true, 0, 0, false, true
    );

    // Start low power RX duty cycle mode
    Radio.SetRxDutyCycle(LORA_RX_TIME, LORA_SLEEP_TIME);
}

void lora_update() {
    // SX126x-Arduino v2 handles IRQs in a background FreeRTOS task.
    // We don't need to poll Radio.IrqProcess().
    
    // Handle timed telemetry broadcast
    uint32_t current_time = millis();
    if ((uint32_t)(current_time - last_telemetry_time) >= TELEMETRY_INTERVAL_MS) {
        lora_send_telemetry(false);
    }
}

void lora_send_telemetry(bool emergency) {
    power_led_flash();
    
    TelemetryPayload payload;
    payload.version = PROTOCOL_VERSION;
    payload.node_id = NODE_ID;
    payload.flags = emergency ? 0x01 : 0x00;
    payload.batt_mv = power_get_batt_mv();
    payload.uptime_s = power_get_uptime_s();
    
    GpsData gps = ble_get_gps_data();
    if (gps.valid) {
        payload.lat_1e7 = gps.lat_1e7;
        payload.lon_1e7 = gps.lon_1e7;
        payload.alt_cm = gps.alt_cm;
    } else {
        payload.lat_1e7 = 0;
        payload.lon_1e7 = 0;
        payload.alt_cm = 0;
    }

    uint8_t buffer[sizeof(TelemetryPayload) + 1];
    buffer[0] = MSG_TYPE_TELEMETRY;
    memcpy(&buffer[1], &payload, sizeof(TelemetryPayload));

    // Switch RF path to TX
    digitalWrite(PIN_LORA_RF_SW, HIGH);
    
    Radio.Send(buffer, sizeof(buffer));
    last_telemetry_time = millis();
    last_tx_time = millis();
}

void lora_send_message(const char* text) {
    power_led_flash();
    
    int len = strlen(text);
    if (len > MAX_MSG_LEN - 1) len = MAX_MSG_LEN - 1;
    
    uint8_t buffer[MAX_MSG_LEN + 2];
    buffer[0] = MSG_TYPE_TEXT;
    buffer[1] = NODE_ID;
    memcpy(&buffer[2], text, len);
    
    digitalWrite(PIN_LORA_RF_SW, HIGH);
    Radio.Send(buffer, len + 2);
    last_tx_time = millis();
}

// ---- Callbacks ----

void OnTxDone(void) {
    // TX complete, switch back to RX
    digitalWrite(PIN_LORA_RF_SW, LOW);
    Radio.SetRxDutyCycle(LORA_RX_TIME, LORA_SLEEP_TIME);
}

void OnTxTimeout(void) {
    digitalWrite(PIN_LORA_RF_SW, LOW);
    Radio.SetRxDutyCycle(LORA_RX_TIME, LORA_SLEEP_TIME);
}

void OnRxDone(uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr) {
    last_rx_time = millis();
    
    if (size > 0) {
        uint8_t type = payload[0];
        
        if (type == MSG_TYPE_TEXT && size >= 2) {
            uint8_t sender = payload[1];
            char text[MAX_MSG_LEN] = {0};
            int text_len = size - 2;
            if (text_len >= MAX_MSG_LEN) text_len = MAX_MSG_LEN - 1;
            memcpy(text, &payload[2], text_len);
            
            power_led_flash();
            ui_add_message(text, sender);
            ui_wake_display();
            
            // Forward to phone over BLE
            ble_send_text(text);
        }
    }
    
    // Resume duty cycle
    Radio.SetRxDutyCycle(LORA_RX_TIME, LORA_SLEEP_TIME);
}

void OnRxTimeout(void) {
    Radio.SetRxDutyCycle(LORA_RX_TIME, LORA_SLEEP_TIME);
}

void OnRxError(void) {
    Radio.SetRxDutyCycle(LORA_RX_TIME, LORA_SLEEP_TIME);
}

uint32_t lora_get_last_tx_time() { return last_tx_time; }
uint32_t lora_get_last_rx_time() { return last_rx_time; }
