#include "WiFi.h"
#include "esp_wifi.h"
#include <vector>
#include <string>
#include <SPI.h>
#include <MFRC522.h>

#define SS_PIN 5        // ESP32 GPIO for RC522 SDA
#define RST_PIN 22      // ESP32 GPIO for RC522 RST
#define BUZZER_PIN 3   // Changed from GPIO3 to GPIO21

// Custom SPI pins (if using GPIO 9 & 10)
#define SCK_PIN 18    
#define MOSI_PIN 9    
#define MISO_PIN 10  

// Target UID for the "admin" card
MFRC522 rfid(SS_PIN, RST_PIN);
byte adminUID[] = {0x13, 0xAE, 0x23, 0x28};  // Replace with your desired admin UID

// Configuration
const char* ssid = "IOT-KU";    // Fake AP SSID
const int channel = 6;          // AP channel
const int maxConnections = 4;   // Max allowed connections

// Modes
enum Mode { OFF, ADD_NEW_MAC, FILTERED_DETECT, OPEN_AP };
Mode currentMode = FILTERED_DETECT;

// Store MAC addresses
std::vector<String> trustedMACs;
std::vector<String> knownDevices;

// Convert MAC to String
String macToString(const uint8_t* mac) {
  char macStr[18];
  snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X",
           mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  return String(macStr);
}

// Add MAC to whitelist
void addTrustedMAC(String mac) {
  if (std::find(trustedMACs.begin(), trustedMACs.end(), mac) == trustedMACs.end()) {
    trustedMACs.push_back(mac);
    Serial.printf("💾 MAC added to whitelist: %s\n", mac.c_str());
  } else {
    Serial.printf("ℹ️ MAC already whitelisted: %s\n", mac.c_str());
  }
}

// Handle new connection
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
      esp_wifi_disconnect();  // Disconnect unknown MAC
    } else {
      Serial.println("✅ Connection allowed.");
    }
  }
}

// Handle disconnection
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
      stopFakeAP();
      break;
    case ADD_NEW_MAC:
      Serial.println("\n📡 Mode: ADD NEW MAC (Open AP, add devices to whitelist)");
      startFakeAP();
      break;
    case FILTERED_DETECT:
      Serial.println("\n🔒 Mode: FILTERED DETECT (Only trusted MACs allowed)");
      startFakeAP();
      break;
    // case OPEN_AP:
    //   startFakeAP();
    //   break;
  }
}

// Check if the card is admin
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

// Activate buzzer for unauthorized access
void activateBuzzer_alert() {
  for (int i = 0; i < 2; i++) {
    digitalWrite(BUZZER_PIN, HIGH);
    delay(3000);
    digitalWrite(BUZZER_PIN, LOW);
    delay(500);
  }
}

// Activate buzzer for admin access
void activateBuzzer_admin() {
  for (int i = 0; i < 2; i++) {
    digitalWrite(BUZZER_PIN, HIGH);
    delay(300);
    digitalWrite(BUZZER_PIN, LOW);
    delay(200);
  }
}

// Setup
void setup() {
  Serial.begin(115200);
  Serial.println("\n🔄 Enter mode (0=OFF, 1=ADD_NEW_MAC, 2=FILTERED_DETECT):");
  WiFi.mode(WIFI_AP);
  switchMode(currentMode);

  SPI.begin(SCK_PIN, MISO_PIN, MOSI_PIN, SS_PIN);
  rfid.PCD_Init();
  Serial.println("RC522 RFID Reader Ready!");

  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);
}

// Loop: Check for mode change & RFID scanning
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

  // Check for RFID card
  if (!rfid.PICC_IsNewCardPresent() || !rfid.PICC_ReadCardSerial()) {
    return;
  }

  Serial.println("\nCard detected!");
  
  if (isAdminCard(rfid.uid.uidByte, rfid.uid.size)) {
    Serial.println("Admin detected!");
    activateBuzzer_admin();
    switchMode(0);
  } else {
    Serial.println("Unknown card! Activating buzzer...");
    activateBuzzer_alert();
  }

  rfid.PICC_HaltA();
  rfid.PCD_StopCrypto1();
}
