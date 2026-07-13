/*
 * AeroPico ESP32 MAVLink UDP bridge skeleton.
 *
 * This is intentionally minimal and disabled from the FC build. Flash it to an
 * ESP32-class companion only after bench testing the Pico USB MAVLink path.
 */

#include <WiFi.h>
#include <WiFiUdp.h>

static constexpr uint32_t MAVLINK_UART_BAUD = 57600;
static constexpr uint16_t MAVLINK_UDP_PORT = 14550;
static constexpr int UART_RX_PIN = 16;
static constexpr int UART_TX_PIN = 17;

static const char* WIFI_SSID = "AeroPico-GCS";
static const char* WIFI_PASS = "change-me-strong-password";

static WiFiUDP udp;
static IPAddress gcsAddress;
static bool gcsKnown = false;

void setup() {
    Serial.begin(115200);
    Serial2.begin(MAVLINK_UART_BAUD, SERIAL_8N1, UART_RX_PIN, UART_TX_PIN);

    WiFi.mode(WIFI_AP);
    WiFi.softAP(WIFI_SSID, WIFI_PASS);
    udp.begin(MAVLINK_UDP_PORT);
    Serial.println("AeroPico MAVLink UDP bridge ready on UDP 14550");
}

void loop() {
    uint8_t buffer[128];

    const int packetSize = udp.parsePacket();
    if (packetSize > 0) {
        gcsAddress = udp.remoteIP();
        gcsKnown = true;
        while (udp.available()) {
            const int n = udp.read(buffer, sizeof(buffer));
            if (n > 0) {
                Serial2.write(buffer, (size_t)n);
            }
        }
    }

    if (gcsKnown && Serial2.available()) {
        size_t count = 0;
        while (Serial2.available() && count < sizeof(buffer)) {
            buffer[count++] = (uint8_t)Serial2.read();
        }
        udp.beginPacket(gcsAddress, MAVLINK_UDP_PORT);
        udp.write(buffer, count);
        udp.endPacket();
    }
}
