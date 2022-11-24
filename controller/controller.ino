#include <WiFi.h>
#include <esp_now.h>

// Pin definitions.
constexpr int kPinPower = 12;
constexpr int kPinForward = 14;
constexpr int kPinReverse = 27;

// MAC address of the remote control unit.
constexpr uint8_t kRemoteAddress[] = {0xC0, 0x49, 0xEF, 0xCC, 0x9A, 0x20};

// Control timeout.  When no command has been received by this many
// milliseconds, disable the output.
constexpr int kControlTimeoutMs = 100;

// ON/OFF macros since the HIGH corresponds to the relay in the OFF position.
#define ON LOW
#define OFF HIGH

// Class to manage the control timeout.
class ControlTimeout {
 public:
  ControlTimeout(uint32_t timeout_ms, std::function<void()> callback)
      : timeout_ms_(timeout_ms),
        callback_(std::move(callback)),
        expiration_ms_(0),
        expired_(true) {}

  // Must be called at the start of loop().
  void Poll() {
    if (!expired_ && expiration_ms_ <= millis()) {
      callback_();
      expired_ = true;
    }
  }

  // Returns true if the timeout has expired, otherwise false.
  bool HasExpired() const { return expired_; }

  // Resets the control timeout using its configured expiration.
  void Reset() {
    expiration_ms_ = millis() + timeout_ms_;
    expired_ = false;
  }

 private:
  const uint32_t timeout_ms_;
  std::function<void()> callback_;
  uint32_t expiration_ms_;
  bool expired_ = true;
};

ControlTimeout control_timeout(kControlTimeoutMs, [] {
  digitalWrite(kPinPower, OFF);
  digitalWrite(kPinForward, OFF);
  digitalWrite(kPinReverse, OFF);
});

// Handle received messages.
void HandleReceive(const uint8_t *address, const uint8_t *data, int size) {
  // Ignore if not from the expected remote address.
  if (memcmp(address, kRemoteAddress, 6) != 0) return;

  switch (*data) {
    case 1:
      digitalWrite(kPinForward, ON);
      digitalWrite(kPinReverse, OFF);
      digitalWrite(kPinPower, ON);
      control_timeout.Reset();
      break;
    case 2:
      digitalWrite(kPinForward, OFF);
      digitalWrite(kPinReverse, ON);
      digitalWrite(kPinPower, ON);
      control_timeout.Reset();
      break;
    default:
      break;
  }
}

void setup() {
  // Set pins as input so that they will not trigger the relay on boot.
  pinMode(kPinPower, INPUT_PULLUP);
  pinMode(kPinForward, INPUT_PULLUP);
  pinMode(kPinReverse, INPUT_PULLUP);

  pinMode(kPinPower, OUTPUT);
  pinMode(kPinForward, OUTPUT);
  pinMode(kPinReverse, OUTPUT);

  digitalWrite(kPinPower, OFF);
  digitalWrite(kPinForward, OFF);
  digitalWrite(kPinReverse, OFF);

  WiFi.mode(WIFI_STA);
  while (esp_now_init() != ESP_OK) delay(1000);
  esp_now_register_recv_cb(HandleReceive);
}

void loop() { control_timeout.Poll(); }
