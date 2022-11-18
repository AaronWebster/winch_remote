#include <RHEncryptedDriver.h>
#include <RH_RF95.h>
#include <SPI.h>
#include <Speck.h>

#include "common.h"

constexpr int kPinForward = 1;
constexpr int kPinReverse = 1;
constexpr int kPinPower = 1;

class ControlTimeout {
 public:
  ControlTimeout(uint64_t timeout_us)
      : timeout_us_(timeout_us), expiration_us_(0), expired_(true) {}

  void Poll() {
    if (!expired_ && expiration_us_ <= Micros64()) expired_ = true;
  }

  bool HasExpired() const { return expired_; }

  void Reset() {
    expiration_us_ = Micros64() + timeout_us_;
    expired_ = false;
  }

 private:
  uint64_t Micros64() {
    static uint32_t low32, high32;
    uint32_t new_low32 = micros();
    if (new_low32 < low32) high32++;
    low32 = new_low32;
    return (uint64_t)high32 << 32 | low32;
  }

  const uint64_t timeout_us_;
  uint64_t expiration_us_;
  bool expired_ = true;
};

enum State { kForward, kReverse, kStopped };
State state = kStopped;

constexpr int kControlTimeoutMs = 500;
ControlTimeout control_timeout(kControlTimeoutMs);

void setup() {
  pinMode(kPinForward, OUTPUT);
  pinMode(kPinReverse, OUTPUT);
  pinMode(kPinPower, OUTPUT);

  digitalWrite(kPinForward, LOW);
  digitalWrite(kPinReverse, LOW);
  digitalWrite(kPinPower, LOW);

  pinMode(kPinRf95Reset, OUTPUT);
  digitalWrite(kPinRf95Reset, HIGH);

  delay(100);
  digitalWrite(kPinRf95Reset, LOW);
  delay(10);
  digitalWrite(kPinRf95Reset, HIGH);
  delay(10);

  rf95.init();
  rf95.setFrequency(kRf95Frequency);
  rf95.setTxPower(kRf95TransmitPower, kRf95PaBoost);
  rf95.setModeRx();

  cipher.setKey(kEncryptionKey, sizeof(kEncryptionKey));
}

void UpdateState(State new_state) {
  state = new_state;
  switch (state) {
    case State::kStopped:
      digitalWrite(kPinPower, LOW);
      digitalWrite(kPinForward, LOW);
      digitalWrite(kPinReverse, LOW);
      break;
    case State::kForward:
      digitalWrite(kPinPower, HIGH);
      digitalWrite(kPinForward, HIGH);
      digitalWrite(kPinReverse, LOW);
      break;
    case State::kReverse:
      digitalWrite(kPinPower, HIGH);
      digitalWrite(kPinForward, LOW);
      digitalWrite(kPinReverse, HIGH);
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
