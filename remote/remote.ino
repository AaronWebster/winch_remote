#include <WiFi.h>
#include <esp_now.h>

// Pin definitions.
constexpr int kPinForward = 17;
constexpr int kPinReverse = 16;

// Address of the control unit.
constexpr uint8_t kControlAddress[] = {0xC8, 0xF0, 0x9E, 0x9E, 0x4B, 0xA8};

// Delay between sending packets when the toggle switch is either FORWARD or
// REVERSE.
constexpr int kRemoteSendInterval = 25;

// Control unit registration info.
esp_now_peer_info_t peer_info;

// Callback for when a packet was sent.
void HandleTransmit(const uint8_t *mac_addr, esp_now_send_status_t status) {}

class TransmitTask {
 public:
  TransmitTask(uint32_t period_ms, std::function<void()> callback)
      : period_ms_(period_ms),
        next_due_ms_(millis() + period_ms_),
        callback_(std::move(callback)) {}

  // Same as above, but accepts a timespec.
  void Poll() {
    const uint32_t now = millis();
    if (next_due_ms_ <= now) {
      callback_();
      next_due_ms_ = now + period_ms_;
    }
  }

 private:
  const uint32_t period_ms_;
  uint32_t next_due_ms_ = 0;
  std::function<void()> callback_;
};

TransmitTask transmit_task(kRemoteSendInterval, [] {
  uint8_t direction =
      (!digitalRead(kPinForward) | (!digitalRead(kPinReverse)) << 1);
  if (direction == 1 || direction == 2) {
    esp_now_send(kControlAddress, &direction, 1);
  }
});

void setup() {
  pinMode(kPinForward, INPUT_PULLUP);
  pinMode(kPinReverse, INPUT_PULLUP);

  WiFi.mode(WIFI_STA);

  while (esp_now_init() != ESP_OK) delay(1000);

  esp_now_register_send_cb(HandleTransmit);

  memcpy(peer_info.peer_addr, kControlAddress, 6);
  peer_info.channel = 0;
  peer_info.encrypt = false;

  while (esp_now_add_peer(&peer_info) != ESP_OK) delay(1000);
}

void loop() { transmit_task.Poll(); }
