#include <WiFi.h>
#include <esp_now.h>

// Replace with Device 2 AP MAC address
//uint8_t receiverAddress[] = {0x68, 0xB6, 0xB3, 0x38, 0x02, 0xA4};  // Replace with actual MAC
uint8_t receiverAddress[] = {0x6A, 0xB6, 0xB3, 0x38, 0x02, 0xA4};  // Correct AP MAC

typedef struct Message {
  char message[64];
} Message;

Message myData;

// ESP-NOW send callback
void onDataSent(const uint8_t *macAddr, esp_now_send_status_t status) {
  Serial.print("Send Status: ");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Success" : "Fail");
}

// ESP-NOW receive callback
void onDataRecv(const esp_now_recv_info_t *info, const uint8_t *incomingData, int len) {
  Serial.println("Reply received from Device 2:");
  Serial.println((char *)incomingData);
}

void setup() {
  Serial.begin(115200);

  // Set to Station mode and disconnect from any Wi-Fi
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);

  // Confirm MAC address
  Serial.print("Device 1 MAC Address: ");
  Serial.println(WiFi.macAddress());

  // Initialize ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW Initialization Failed");
    return;
  }

  esp_now_register_send_cb(onDataSent);
  esp_now_register_recv_cb(onDataRecv);

  // Register Device 2 as a peer
  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, receiverAddress, 6);
  peerInfo.channel = 1;  // Ensure same channel as AP
  peerInfo.encrypt = false;

  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Failed to add peer.");
  } else {
    Serial.println("ESP-NOW peer added successfully.");
  }

  // Send HelloWorld to Device 2
  strcpy(myData.message, "Hello from Device 1!");
  esp_err_t result = esp_now_send(receiverAddress, (uint8_t *)&myData, sizeof(myData));

  if (result == ESP_OK) {
    Serial.println("Message sent successfully!");
  } else {
    Serial.println("Failed to send message.");
  }
}

void loop() {
  // Listen for incoming replies
}
