#pragma once

#include <Arduino.h>

enum MessageType : uint8_t {
    MSG_TYPE_TELEMETRY = 0x01,
    MSG_TYPE_TEXT      = 0x02,
    MSG_TYPE_EMERGENCY = 0x03
};

// 21-byte compact binary telemetry payload
#pragma pack(push, 1)
struct TelemetryPayload {
    uint8_t  version;     // Protocol version
    int32_t  lat_1e7;     // Latitude * 10^7
    int32_t  lon_1e7;     // Longitude * 10^7
    int32_t  alt_cm;      // Altitude in cm
    uint16_t batt_mv;     // Battery voltage in mV
    uint8_t  node_id;     // Sender Node ID
    uint8_t  flags;       // Status flags (e.g., bit 0 = emergency)
    uint32_t uptime_s;    // Uptime in seconds
};
#pragma pack(pop)

struct GpsData {
    int32_t lat_1e7;
    int32_t lon_1e7;
    int32_t alt_cm;
    uint32_t timestamp_ms; // Last time GPS was updated
    bool valid;
};

// Circular buffer for text messages
#define MAX_MESSAGES 5
#define MAX_MSG_LEN 64

struct TextMessage {
    char text[MAX_MSG_LEN];
    uint32_t timestamp_ms;
    uint8_t sender_id;
};

class MessageBuffer {
private:
    TextMessage messages[MAX_MESSAGES];
    int head = 0;
    int count = 0;

public:
    void add(const char* text, uint8_t sender) {
        strncpy(messages[head].text, text, MAX_MSG_LEN - 1);
        messages[head].text[MAX_MSG_LEN - 1] = '\0';
        messages[head].timestamp_ms = millis();
        messages[head].sender_id = sender;
        
        head = (head + 1) % MAX_MESSAGES;
        if (count < MAX_MESSAGES) count++;
    }

    int getCount() const { return count; }
    
    // index 0 is most recent
    const TextMessage* getRecent(int index) const {
        if (index >= count) return nullptr;
        int idx = (head - 1 - index + MAX_MESSAGES) % MAX_MESSAGES;
        return &messages[idx];
    }
};
