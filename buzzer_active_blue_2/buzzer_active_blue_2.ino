#define BUZZER_PIN 8  // Change GPIO pin as needed

void setup() {
  pinMode(BUZZER_PIN, OUTPUT);
}

void loop() {
  tone(BUZZER_PIN, 1000);  // 1000 Hz tone
  delay(1000);             // 1 second
  noTone(BUZZER_PIN);      // Stop tone
  delay(1000);             // 1 second
}
