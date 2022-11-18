#include <RHEncryptedDriver.h>
#include <RH_RF95.h>
#include <SPI.h>
#include <Speck.h>

constexpr uint8_t kEncryptionKey[16] = {106, 1,  215, 197, 229, 102, 167, 178,
                                        96,  90, 179, 182, 24,  165, 89,  139};

constexpr char kForwardCommand[] = "forward";
constexpr char kReverseCommand[] = "reverse";

#define PIN_FORWARD
#define PIN_REVERSE

#define RFM95_CS 8
#define RFM95_RST 4
#define RFM95_INT 7
#define RF95_FREQ 434.0
RH_RF95 rf95(RFM95_CS, RFM95_INT);
Speck cipher;
RHEncryptedDriver driver(rf95, cipher);

void setup() {
  pinMode(RFM95_RST, OUTPUT);
  digitalWrite(RFM95_RST, HIGH);
  digitalWrite(RFM95_RST, LOW);
  delay(10);
  digitalWrite(RFM95_RST, HIGH);
  delay(10);

  rf95.init();
  rf95.setFrequency(RF95_FREQ);
  rf95.setTxPower(23, false);
  rf95.setModeTx();
  cipher.setKey(kEncryptionKey, sizeof(kEncryptionKey));
}

void loop() {
  if (digitalRead(PIN_FORWARD)) {
    driver.send(kForwardCommand, sizeof(kForwardCommand));
  }

  if (digitalRead(PIN_REVERSE)) {
    driver.send(kReverseCommand, sizeof(kReverseCommand));
  }
  delay(250);
}
