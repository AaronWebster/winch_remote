#include <WiFi.h>
#include <esp_now.h>

#include "packets.emb.h"

using namespace winch_remote;

// Address of the control unit.
constexpr uint8_t kBroadcastAddress[] = {0xC8, 0xF0, 0x9E, 0x9E, 0x4B, 0xA8};

// Pin definitions.
constexpr int kPinForward = 17;
constexpr int kPinReverse = 16;

// Delay between sending packets when the toggle switch is either FORWARD or
// REVERSE.
constexpr int kTransmitDelayMs = 25;

// Control unit registration info.
esp_now_peer_info_t peer_info;

// Packet buffer.
uint8_t packet_buf = 0;
PacketWriter packet{&packet_buf, sizeof(packet_buf)};

// Callback for when a packet was sent.
void HandleTransmit(const uint8_t *mac_addr, esp_now_send_status_t status) {}

void setup() {
  pinMode(kPinForward, INPUT_PULLUP);
  pinMode(kPinReverse, INPUT_PULLUP);

  WiFi.mode(WIFI_STA);

  if (esp_now_init() != ESP_OK) return setup();

  esp_now_register_send_cb(HandleTransmit);

  memcpy(peer_info.peer_addr, kBroadcastAddress, 6);
  peer_info.channel = 0;
  peer_info.encrypt = false;

  if (esp_now_add_peer(&peer_info) != ESP_OK) return setup();
}

void loop() {
  delay(kTransmitDelayMs);

  // Read packet direction.  This maps directly to an enum value if valid.
  // Otherwise, Ok() returns false and the packet is not sent.
  packet.direction().Write(static_cast<Direction>(
      !digitalRead(kPinForward) | (!digitalRead(kPinReverse)) << 1));

  if (packet.Ok()) {
    esp_now_send(kBroadcastAddress, packet.BackingStorage().data(),
                 packet.SizeInBytes());
  }
}
