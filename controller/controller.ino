#include <WiFi.h>
#include <esp_now.h>

#include "packets.emb.h"

using namespace winch_remote;

// MAC address of the remote control unit.
constexpr uint8_t kRemoteAddress[] = {0xC0, 0x49, 0xEF, 0xCC, 0x9A, 0x20};

// Pin definitions.
constexpr int kPinPowerPositive = 12;
constexpr int kPinPowerNegative = 13;
constexpr int kPinForward = 14;
constexpr int kPinReverse = 27;

// Control timeout.  When no command has been received by this many
// milliseocnds, disable the output.
constexpr int kControlTimeoutMs = 50;

// ON/OFF macros since the HIGH corresponds to the relay in the OFF position.
#define ON LOW
#define OFF HIGH

// Class to manage the control timeout.
class ControlTimeout {
 public:
  ControlTimeout(uint32_t timeout_ms)
      : timeout_ms_(timeout_ms), expiration_ms_(0), expired_(true) {}

  // Must be called at the start of loop().
  void Poll() {
    if (!expired_ && expiration_ms_ <= millis()) expired_ = true;
  }

  // Returns true if the timeout has expired, otherwise false.
  bool HasExpired() const { return expired_; }

  // Resets the control timeout using its configured exipration.
  void Reset() {
    expiration_ms_ = millis() + timeout_ms_;
    expired_ = false;
  }

 private:
  const uint32_t timeout_ms_;
  uint32_t expiration_ms_;
  bool expired_ = true;
};

ControlTimeout control_timeout(kControlTimeoutMs);

// Handle received messages.
void HandleReceive(const uint8_t *address, const uint8_t *data, int size) {
  // Ignore if not from the whitelisted MAC address.
  if (memcmp(address, kRemoteAddress, 6) != 0) return;

  auto packet = MakePacketView(data, size);
  if (!packet.Ok()) return;

  // Ignore all direction commands except for FORWARD and REVERSE.
  switch (packet.direction().Read()) {
    case Direction::FORWARD:
      digitalWrite(kPinForward, ON);
      digitalWrite(kPinReverse, OFF);
      digitalWrite(kPinPowerPositive, ON);
      digitalWrite(kPinPowerNegative, ON);
      control_timeout.Reset();
      break;
    case Direction::REVERSE:
      digitalWrite(kPinForward, OFF);
      digitalWrite(kPinReverse, ON);
      digitalWrite(kPinPowerPositive, ON);
      digitalWrite(kPinPowerNegative, ON);
      control_timeout.Reset();
      break;
  }
}

void setup() {
  // Set pins as input so that they will not trigger the relay on boot.
  pinMode(kPinPowerPositive, INPUT_PULLUP);
  pinMode(kPinPowerNegative, INPUT_PULLUP);
  pinMode(kPinForward, INPUT_PULLUP);
  pinMode(kPinReverse, INPUT_PULLUP);

  pinMode(kPinPowerPositive, OUTPUT);
  pinMode(kPinPowerNegative, OUTPUT);
  pinMode(kPinForward, OUTPUT);
  pinMode(kPinReverse, OUTPUT);

  digitalWrite(kPinPowerPositive, OFF);
  digitalWrite(kPinPowerNegativeOFF);
  digitalWrite(kPinForward, OFF);
  digitalWrite(kPinReverse, OFF);

  WiFi.mode(WIFI_STA);
  if (esp_now_init() != ESP_OK) return setup();
  esp_now_register_recv_cb(HandleReceive);
}

void loop() {
  control_timeout.Poll();
  if (control_timeout.HasExpired()) {
    digitalWrite(kPinPowerPositive, OFF);
    digitalWrite(kPinPowerNegative, OFF);
    digitalWrite(kPinForward, OFF);
    digitalWrite(kPinReverse, OFF);
  }
}
