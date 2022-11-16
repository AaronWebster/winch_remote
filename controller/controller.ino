#include <RHEncryptedDriver.h>
#include <RH_RF95.h>
#include <SPI.h>
#include <Speck.h>

class ControlTimeout {
 public:
  ControlTimeout(uint32_t timeout_ms)
      : timeout_ms_(timeout_ms), expiration_ms_(0), expired_(true) {}

  void Poll() {
    if (!expired_ && expiration_ms_ <= millis()) expired_ = true;
  }

  bool HasExpired() const { return expired_; }

  void Reset() {
    expiration_ms_ = millis() + timeout_ms_;
    expired_ = false;
  }

 private:
  const uint32_t timeout_ms_;
  uint32_t expiration_ms_;
  bool expired_ = true;
};

#define PIN_FORWARD 1
#define PIN_REVERSE 2
#define PIN_POWER 3

#define RFM95_CS 8
#define RFM95_RST 4
#define RFM95_INT 7
#define RF95_FREQ 434.0
RH_RF95 rf95(RFM95_CS, RFM95_INT);

Speck cipher;
RHEncryptedDriver driver(rf95, cipher);
constexpr uint8_t kEncryptionKey[16] = {106, 1,  215, 197, 229, 102, 167, 178,
                                        96,  90, 179, 182, 24,  165, 89,  139};

enum State { kForward, kReverse, kStopped };
State state = kStopped;

constexpr char kForwardCommand[] = "forward";
constexpr char kReverseCommand[] = "reverse";

constexpr int kControlTimeoutMs = 500;
ControlTimeout control_timeout(kControlTimeoutMs);

void setup() {
  pinMode(PIN_FORWARD, OUTPUT);
  pinMode(PIN_REVERSE, OUTPUT);
  pinMode(PIN_POWER, OUTPUT);

  digitalWrite(PIN_FORWARD, LOW);
  digitalWrite(PIN_REVERSE, LOW);
  digitalWrite(PIN_POWER, LOW);

  pinMode(RFM95_RST, OUTPUT);
  digitalWrite(RFM95_RST, HIGH);

  delay(100);
  digitalWrite(RFM95_RST, LOW);
  delay(10);
  digitalWrite(RFM95_RST, HIGH);
  delay(10);

  rf95.init();
  rf95.setFrequency(RF95_FREQ);
  rf95.setTxPower(23, false);
  rf95.setModeRx();

  cipher.setKey(kEncryptionKey, sizeof(kEncryptionKey));
}

void UpdateState(State new_state) {
  state = new_state;
  switch (state) {
    case State::kStopped:
      digitalWrite(PIN_POWER, LOW);
      digitalWrite(PIN_FORWARD, LOW);
      digitalWrite(PIN_REVERSE, LOW);
      break;
    case State::kForward:
      digitalWrite(PIN_POWER, HIGH);
      digitalWrite(PIN_FORWARD, HIGH);
      digitalWrite(PIN_REVERSE, LOW);
      break;
    case State::kReverse:
      digitalWrite(PIN_POWER, HIGH);
      digitalWrite(PIN_FORWARD, LOW);
      digitalWrite(PIN_REVERSE, HIGH);
      break;
  }
  if (state != State::kStopped) control_timeout.Reset();
}

void loop() {
  control_timeout.Poll();
  if (control_timeout.HasExpired()) UpdateState(State::kStopped);

  if (!driver.available()) return;
  uint8_t buf[driver.maxMessageLength()];
  memset(&buf, 0, driver.maxMessageLength());
  uint8_t len = sizeof(buf);
  if (!driver.recv(buf, &len)) return;

  if (len == 0) return;

  if (len == sizeof(kForwardCommand) && strcmp(buf, kForwardCommand) == 0) {
    return UpdateState(kForward);
  }

  if (len == sizeof(kReverseCommand) && strcmp(buf, kReverseCommand) == 0) {
    return UpdateState(kReverse);
  }
}
