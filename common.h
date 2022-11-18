constexpr uint8_t kEncryptionKey[16] = {106, 1,  215, 197, 229, 102, 167, 178,
                                        96,  90, 179, 182, 24,  165, 89,  139};
constexpr char kForwardCommand[] = "forward";
constexpr char kReverseCommand[] = "reverse";

constexpr int kPinRf95ChipSelect = 8;
constexpr int kPinRf95Reset = 4;
constexpr int kPinRf95Interrupt = 7;
constexpr float kRf95Frequency = 434.0;
constexpr int kRf95TransmitPower= 7;
constexpr bool kRf95PaBoost = false;

RH_RF95 rf95(kPinRf95ChipSelect, kPinRf95Interrupt);
Speck cipher;
RHEncryptedDriver driver(rf95, cipher);
