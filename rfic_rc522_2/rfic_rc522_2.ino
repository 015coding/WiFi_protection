#include <SPI.h>
#include <MFRC522.h>

#define SS_PIN 5        // ESP32 GPIO for RC522 SDA
#define RST_PIN 22      // ESP32 GPIO for RC522 RST
#define BUZZER_PIN 3    // GPIO for Buzzer

// Custom SPI pins (if using GPIO 9 & 10)
#define SCK_PIN 18    
#define MOSI_PIN 9    
#define MISO_PIN 10   

MFRC522 rfid(SS_PIN, RST_PIN);

// Target UID for the "admin" card
byte adminUID[] = {0x13, 0xAE, 0x23, 0x28};  // Replace with your desired admin UID

void setup() {
  Serial.begin(115200);

  // Initialize SPI with custom pins
  SPI.begin(SCK_PIN, MISO_PIN, MOSI_PIN, SS_PIN);
  
  // Initialize RFID module
  rfid.PCD_Init();
  Serial.println("RC522 RFID Reader Ready!");
  
  // Set buzzer pin as output
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);  // Ensure buzzer is off initially
}

void loop() {
  // Check for a new RFID card
  if (!rfid.PICC_IsNewCardPresent() || !rfid.PICC_ReadCardSerial()) {
    return;
  }

  Serial.println("\nCard detected!");

  // Read and display the card UID
  Serial.print("Card UID: ");
  for (byte i = 0; i < rfid.uid.size; i++) {
    Serial.print(rfid.uid.uidByte[i] < 0x10 ? "0" : "");
    Serial.print(rfid.uid.uidByte[i], HEX);
    Serial.print(" ");
  }
  Serial.println();

  // Check if it's the admin card
  if (isAdminCard(rfid.uid.uidByte, rfid.uid.size)) {
    Serial.println("Admin detected!");
    activateBuzzer_admin();
  } else {
    Serial.println("Unknown card! Activating buzzer...");
    activateBuzzer_allert();
  }

  // Halt the card
  rfid.PICC_HaltA();
  rfid.PCD_StopCrypto1();
}

// Function to check if the card is the "admin" card
bool isAdminCard(byte *uid, byte size) {
  if (size != sizeof(adminUID)) {
    return false;  // UID size mismatch
  }
  for (byte i = 0; i < size; i++) {
    if (uid[i] != adminUID[i]) {
      return false;  // Byte mismatch
    }
  }
  return true;  // UID matches
}

// Function to activate the buzzer for 5 seconds
// void activateBuzzer_allert() {
//   Serial.println("Buzzer ON for 5 seconds...");
//   tone(BUZZER_PIN, 4000);  // 1000 Hz tone
//   delay(3000);             // 1 second
//   noTone(BUZZER_PIN);      // Stop tone

// }

// void activateBuzzer_admin() {
//   Serial.println("Welcome Admin");
//   for (int i = 0; i < 2; i++) {
//     tone(BUZZER_PIN, 4000);  // Play 4000 Hz tone
//     delay(300);              // Keep the tone for 300 ms
//     noTone(BUZZER_PIN);      // Stop the tone
//     delay(200);    
//     }          // Pause between beeps
// }

void activateBuzzer_allert() {
  for (int i = 0; i < 2 ; i++) {
    digitalWrite(BUZZER_PIN, HIGH);
    delay(3000);
    digitalWrite(BUZZER_PIN, LOW);
    delay (500);
  }
}

void activateBuzzer_admin() {
  for (int i = 0; i < 2 ; i++) {
    digitalWrite(BUZZER_PIN, HIGH);
    delay(300);
    digitalWrite(BUZZER_PIN, LOW);
    delay (200);
  }
}
