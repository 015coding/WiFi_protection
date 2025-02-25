#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>

// MAC address of Device 1 (Sender)
uint8_t device1Address[] = {0x68, 0xB6, 0xB3, 0x37, 0xF2, 0xBC};  // Replace with Device 1 MAC

typedef struct Message {
  char message[64];
} Message;

Message myData;

// ESP-NOW receive callback
void onDataRecv(const esp_now_recv_info_t *info, const uint8_t *incomingData, int len) {
  Serial.println("Message received:");
  Serial.println((char *)incomingData);

  // Send reply to Device 1
  strcpy(myData.message, "HelloBack from Device 2!");

  esp_err_t result = esp_now_send(device1Address, (uint8_t *)&myData, sizeof(myData));
  if (result == ESP_OK) {
    Serial.println("Reply sent successfully!");
  } else {
    Serial.println("Failed to send reply.");
  }
}

void setup() {
  Serial.begin(115200);

  // Set ESP32 as AP without password
  WiFi.mode(WIFI_AP);
  WiFi.softAP("ESP32_AP");  // Open AP

  Serial.println("ESP32 AP started...");
  Serial.print("AP MAC Address: ");
  Serial.println(WiFi.softAPmacAddress());

  // Initialize ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW Initialization Failed");
    return;
  }

  // Register receive callback
  esp_now_register_recv_cb(onDataRecv);

  // Register Device 1 as a peer
  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, device1Address, 6);
  peerInfo.channel = 1;  // Ensure same channel as AP
  peerInfo.encrypt = false;

  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Failed to add peer for Device 1.");
  } else {
    Serial.println("ESP-NOW peer added successfully for Device 1.");
  }

  Serial.println("ESP-NOW Receiver is ready.");
}

void loop() {
  // Listen for incoming messages and reply
}
