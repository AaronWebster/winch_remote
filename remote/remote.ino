#include <RHEncryptedDriver.h>
#include <RH_RF95.h>
#include <SPI.h>
#include <Speck.h>

#include "common.h"

constexpr int kPinForward = 1;
constexpr int kPinReverse = 1;

enum PinState { kNone = 0, kForward = 1, kReverse = 2, kBoth = 3 };

void setup() {
  pinMode(kPinRf95Reset, OUTPUT);
  digitalWrite(kPinRf95Reset, HIGH);
  digitalWrite(kPinRf95Reset, LOW);
  delay(10);
  digitalWrite(kPinRf95Reset, HIGH);
  delay(10);

  rf95.init();
  rf95.setFrequency(kRf95Frequency);
  rf95.setTxPower(kRf95TransmitPower, kRf95PaBoost);
  rf95.setModeTx();
  cipher.setKey(kEncryptionKey, sizeof(kEncryptionKey));
}

void loop() {
  delay(250);
  PinState pin_state = static_cast<PinState>(digitalRead(kPinForward) |
                                             (digitalRead(kPinReverse) << 1));
  switch (pin_state) {
    case PinState::kForward:
      driver.send(kForwardCommand, sizeof(kForwardCommand));
      break;
    case PinState::kReverse:
      driver.send(kReverseCommand, sizeof(kReverseCommand));
    default:
      break;
  }
}
