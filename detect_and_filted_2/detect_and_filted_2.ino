#include "WiFi.h"
#include "esp_wifi.h"
#include <vector>
#include <string>
#include <SPI.h>
#include <MFRC522.h>

#define SS_PIN 5        // RC522 SDA (Chip Select)
#define RST_PIN 22      // RC522 Reset
#define BUZZER_PIN 15   // Buzzer GPIO (Changed from GPIO 36)

// Updated SPI Pins (Safe for ESP32-S3)
#define SCK_PIN 18      // SPI Clock
#define MOSI_PIN 11     // SPI MOSI (Changed from GPIO 21)
#define MISO_PIN 13     // SPI MISO (Changed from GPIO 19)

// Admin Card UID (Replace with your own)
MFRC522 rfid(SS_PIN, RST_PIN);
byte adminUID[] = {0x13, 0xAE, 0x23, 0x28};  

// Wi-Fi Configuration
const char* ssid = "IOT-KU";
const int channel = 6;
const int maxConnections = 4;

// Modes
enum Mode { OFF, ADD_NEW_MAC, FILTERED_DETECT };
Mode currentMode = FILTERED_DETECT;

// MAC Address Storage
std::vector<String> trustedMACs;
std::vector<String> knownDevices;

// Convert MAC to String
String macToString(const uint8_t* mac) {
  char macStr[18];
  snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X",
           mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  return String(macStr);
}

// Add Trusted MAC
void addTrustedMAC(String mac) {
  if (std::find(trustedMACs.begin(), trustedMACs.end(), mac) == trustedMACs.end()) {
    trustedMACs.push_back(mac);
    Serial.printf("💾 MAC added to whitelist: %s\n", mac.c_str());
  } else {
    Serial.printf("ℹ️ MAC already whitelisted: %s\n", mac.c_str());
  }
}

// Handle New Connection
void onConnected(WiFiEvent_t event, WiFiEventInfo_t info) {
  wifi_event_ap_staconnected_t* connected = (wifi_event_ap_staconnected_t*)&info;
  String mac = macToString(connected->mac);

  if (std::find(knownDevices.begin(), knownDevices.end(), mac) == knownDevices.end()) {
    Serial.printf("\n🔔 New device connected: %s\n", mac.c_str());
    knownDevices.push_back(mac);

    if (currentMode == ADD_NEW_MAC) {
      addTrustedMAC(mac);
    }

    if (currentMode == FILTERED_DETECT && 
        std::find(trustedMACs.begin(), trustedMACs.end(), mac) == trustedMACs.end()) {
      Serial.println("❌ Unauthorized MAC detected! Disconnecting...");
      esp_wifi_disconnect();
    } else {
      Serial.println("✅ Connection allowed.");
    }
  }
}

// Handle Disconnection
void onDisconnected(WiFiEvent_t event, WiFiEventInfo_t info) {
  wifi_event_ap_stadisconnected_t* disconnected = (wifi_event_ap_stadisconnected_t*)&info;
  String mac = macToString(disconnected->mac);
  Serial.printf("🔓 Device disconnected: %s\n", mac.c_str());

  auto it = std::find(knownDevices.begin(), knownDevices.end(), mac);
  if (it != knownDevices.end()) {
    knownDevices.erase(it);
    Serial.println("🗑️ MAC removed from known list.");
  }
}

// Start Fake AP
void startFakeAP() {
  WiFi.softAP(ssid, "", channel, 0, maxConnections);
  Serial.printf("\n🚀 Fake AP '%s' started on channel %d.\n", ssid, channel);
  WiFi.onEvent(onConnected, ARDUINO_EVENT_WIFI_AP_STACONNECTED);
  WiFi.onEvent(onDisconnected, ARDUINO_EVENT_WIFI_AP_STADISCONNECTED);
}

// Stop Fake AP
void stopFakeAP() {
  WiFi.softAPdisconnect(true);
  Serial.println("\n❌ Fake AP stopped.");
}

// Switch Mode
void switchMode(int mode) {
  stopFakeAP();
  currentMode = static_cast<Mode>(mode);
  knownDevices.clear();

  switch (currentMode) {
    case OFF:
      Serial.println("\n🚫 Mode: OFF (AP disabled)");
      break;
    case ADD_NEW_MAC:
      Serial.println("\n📡 Mode: ADD NEW MAC (Open AP, add devices to whitelist)");
      startFakeAP();
      break;
    case FILTERED_DETECT:
      Serial.println("\n🔒 Mode: FILTERED DETECT (Only trusted MACs allowed)");
      startFakeAP();
      break;
  }
}

// Check Admin Card
bool isAdminCard(byte *uid, byte size) {
  if (size != sizeof(adminUID)) {
    return false;
  }
  for (byte i = 0; i < size; i++) {
    if (uid[i] != adminUID[i]) {
      return false;
    }
  }
  return true;
}

// Activate Buzzer
void activateBuzzer(bool isAdmin) {
  int beepDuration = isAdmin ? 300 : 1000;
  for (int i = 0; i < 2; i++) {
    digitalWrite(BUZZER_PIN, HIGH);
    delay(beepDuration);
    digitalWrite(BUZZER_PIN, LOW);
    delay(300);
  }
}

// Setup
void setup() {
  Serial.begin(115200);
  Serial.println("\n🔄 Enter mode (0=OFF, 1=ADD_NEW_MAC, 2=FILTERED_DETECT):");

  // Disable Wi-Fi before SPI Init (Avoid conflict)
  WiFi.mode(WIFI_OFF);
  delay(500);

  // Initialize SPI and RFID
  SPI.begin(SCK_PIN, MISO_PIN, MOSI_PIN, SS_PIN);
  rfid.PCD_Init();

  // Re-enable Wi-Fi after SPI init
  WiFi.mode(WIFI_AP);
  switchMode(currentMode);

  // Buzzer Setup
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);

  Serial.println("✅ System Ready!");
}

// Main Loop
void loop() {
  if (Serial.available()) {
    int mode = Serial.parseInt();
    if (mode >= 0 && mode <= 2) {
      Serial.printf("\n🔄 Changing mode to %d...\n", mode);
      switchMode(mode);
    } else {
      Serial.println("❌ Invalid mode. Use 0 (OFF), 1 (Add MAC), 2 (Filtered Detect).");
    }
  }

  // Check for RFID Card
  if (!rfid.PICC_IsNewCardPresent() || !rfid.PICC_ReadCardSerial()) {
    delay(100);  // Prevent watchdog reset by adding delay
    return;
  }

  Serial.println("\n🎫 Card detected!");

  if (isAdminCard(rfid.uid.uidByte, rfid.uid.size)) {
    Serial.println("🔑 Admin detected! Turning off AP...");
    activateBuzzer(true);
    switchMode(OFF);
  } else {
    Serial.println("❌ Unknown card! Activating alert...");
    activateBuzzer(false);
  }

  rfid.PICC_HaltA();
  rfid.PCD_StopCrypto1();
  delay(100);  // Ensure watchdog timer resets
}
