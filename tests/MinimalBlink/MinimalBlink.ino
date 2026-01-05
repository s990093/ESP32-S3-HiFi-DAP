void setup() {
  pinMode(2, OUTPUT);  // LED on GPIO2 (common on ESP32 dev boards)
}

void loop() {
  digitalWrite(2, HIGH);
  delay(1000);
  digitalWrite(2, LOW);
  delay(1000);
}
