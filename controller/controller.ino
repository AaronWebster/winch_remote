#include <esp_now.h>
#include <WiFi.h>
#include "packets.emb.h"

#define CHANNEL 1

#define PIN_FORWARD 1
#define PIN_REVERSE 2
#define PIN_POWER 3


using namespace internal;

Direction direction = Direction::STOP;

class ControlTimeout {
 public:
  ControlTimeout(uint32_t timeout_ms)
      : timeout_us_(timeout_ms), expiration_us_(0), expired_(true) {}

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
    return (uint64_t) high32 << 32 | low32;
}
  const uint32_t timeout_us_;
  uint32_t expiration_us_;
  bool expired_ = true;
};


constexpr int kControlTimeoutMicros = 500000;
ControlTimeout control_timeout(kControlTimeoutMicros);


void UpdateDirection(Direction new_direction) {
  if(direction != new_direction){
  switch (new_direction) {
    case Direction::STOP:
      Serial.println("STOPPED");
      // digitalWrite(PIN_POWER, LOW);
      // digitalWrite(PIN_FORWARD, LOW);
      // digitalWrite(PIN_REVERSE, LOW);
      break;
    case Direction::FORWARD:
      Serial.println("FORWARD");
      // digitalWrite(PIN_POWER, HIGH);
      // digitalWrite(PIN_FORWARD, HIGH);
      // digitalWrite(PIN_REVERSE, LOW);
      break;
    case Direction::REVERSE:
      Serial.println("REVERSE");
      // digitalWrite(PIN_POWER, HIGH);
      // digitalWrite(PIN_FORWARD, LOW);
      // digitalWrite(PIN_REVERSE, HIGH);
      break;
  }
  }
  if (new_direction != Direction::STOP) control_timeout.Reset();
  direction = new_direction;
}



// Init ESP Now with fallback
void InitESPNow() {
  WiFi.disconnect();
  if (esp_now_init() == ESP_OK) {
    Serial.println("ESPNow Init Success");
  }
  else {
    Serial.println("ESPNow Init Failed");
    // Retry InitESPNow, add a counte and then restart?
    // InitESPNow();
    // or Simply Restart
    ESP.restart();
  }
}

// config AP SSID
void configDeviceAP() {
  const char *SSID = "Slave_1";
  bool result = WiFi.softAP(SSID, "Slave_1_Password", CHANNEL, 0);
  if (!result) {
    Serial.println("AP Config failed.");
  } else {
    Serial.println("AP Config Success. Broadcasting with AP: " + String(SSID));
    Serial.print("AP CHANNEL "); Serial.println(WiFi.channel());
  }
}

void setup() {
  //  pinMode(PIN_FORWARD, OUTPUT);
  //pinMode(PIN_REVERSE, OUTPUT);
  //pinMode(PIN_POWER, OUTPUT);
  //digitalWrite(PIN_FORWARD, LOW);
  //digitalWrite(PIN_REVERSE, LOW);
  //digitalWrite(PIN_POWER, LOW);

  Serial.begin(115200);
  Serial.println("ESPNow/Basic/Slave Example");
  //Set device in AP mode to begin with
  WiFi.mode(WIFI_AP);
  // configure device AP mode
  configDeviceAP();
  // This is the mac address of the Slave in AP Mode
  Serial.print("AP MAC: "); Serial.println(WiFi.softAPmacAddress());
  // Init ESPNow with a fallback logic
  InitESPNow();
  // Once ESPNow is successfully Init, we will register for recv CB to
  // get recv packer info.
  esp_now_register_recv_cb(OnDataRecv);
}

// callback when data is recv from Master
void OnDataRecv(const uint8_t *mac_addr, const uint8_t *data, int data_len) {
  char macStr[18];
  snprintf(macStr, sizeof(macStr), "%02x:%02x:%02x:%02x:%02x:%02x",
           mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
  auto view = MakePacketView(data, data_len);
  if(!view.Ok()){
   Serial.println("Invalid data."); 
  } else {
  Serial.print(macStr);
  Serial.print(" ");
  Serial.print(view.sequence_id().Read());
  Serial.print(" ");
  Serial.println(TryToGetNameFromEnum(view.direction().Read()));
  UpdateDirection(view.direction().Read());
  }
}

void loop() {
  control_timeout.Poll();
  if (control_timeout.HasExpired()) UpdateDirection(Direction::STOP);
}
