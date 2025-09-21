#include <HardwareSerial.h>
#include "DHT.h"

// ---- LoRa ----
HardwareSerial LoRa(1);

// ---- Motor driver pins ----
#define DIR1 14
#define PWM1 27
#define DIR2 12
#define PWM2 26
int speedVal = 200;

// ---- DHT22 Sensor ----
#define DHTPIN 32
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);

// ---- LDR Sensor ----
#define LDR_PIN 33   // D0 output from LDR module

// ---- MQ135 Sensor ----
#define MQ135_PIN 35  // AO output from MQ135

unsigned long lastTelemetry = 0;
const long telemetryInterval = 5000; // 5 sec

void setup() {
  Serial.begin(115200);
  LoRa.begin(115200, SERIAL_8N1, 16, 17);

  Serial.println("LoRa + Motor Driver + DHT22 + LDR + MQ135 Receiver");

  // Init LoRa
  LoRa.println("AT+RESET"); delay(1000);
  LoRa.println("AT+ADDRESS=2"); delay(200);
  LoRa.println("AT+NETWORKID=5"); delay(200);
  LoRa.println("AT+PARAMETER=12,7,1,4"); delay(200);
  LoRa.println("AT+BAND=868000000"); delay(200);

  // Init motor pins
  pinMode(DIR1, OUTPUT);
  pinMode(DIR2, OUTPUT);
  ledcAttach(PWM1, 5000, 8);
  ledcAttach(PWM2, 5000, 8);
  stopMotors();

  // Init sensors
  dht.begin();
  pinMode(LDR_PIN, INPUT);
}

void forward() { digitalWrite(DIR1, HIGH); digitalWrite(DIR2, HIGH); ledcWrite(PWM1, speedVal); ledcWrite(PWM2, speedVal); }
void backward(){ digitalWrite(DIR1, LOW);  digitalWrite(DIR2, LOW);  ledcWrite(PWM1, speedVal); ledcWrite(PWM2, speedVal); }
void left()    { digitalWrite(DIR1, LOW);  digitalWrite(DIR2, HIGH); ledcWrite(PWM1, speedVal); ledcWrite(PWM2, speedVal); }
void right()   { digitalWrite(DIR1, HIGH); digitalWrite(DIR2, LOW);  ledcWrite(PWM1, speedVal); ledcWrite(PWM2, speedVal); }
void stopMotors(){ ledcWrite(PWM1, 0); ledcWrite(PWM2, 0); }

void loop() {
  // ---- LoRa command receiver ----
  if (LoRa.available()) {
    String incoming = LoRa.readStringUntil('\n');
    Serial.println("Raw LoRa: " + incoming);

    if (incoming.startsWith("+RCV=")) {
      int firstComma = incoming.indexOf(',');
      int secondComma = incoming.indexOf(',', firstComma + 1);
      int thirdComma = incoming.indexOf(',', secondComma + 1);

      if (secondComma != -1 && thirdComma != -1) {
        String payload = incoming.substring(secondComma + 1, thirdComma);
        payload.trim();

        if (payload.length() > 0) {
          char cmd = payload.charAt(0);
          Serial.println("Command: " + payload);

          switch (cmd) {
            case 'w': case 'W': backward(); Serial.println("Backward"); break;
            case 's': case 'S': forward();  Serial.println("Forward");  break;
            case 'a': case 'A': right();    Serial.println("Right");    break;
            case 'd': case 'D': left();     Serial.println("Left");     break;
            case 'x': case 'X': stopMotors(); Serial.println("Stop");   break;
          }
        }
      }
    }
  }

  // ---- Sensors: DHT22 + LDR + MQ135 telemetry ----
  unsigned long now = millis();
  if (now - lastTelemetry > telemetryInterval) {
    lastTelemetry = now;

    float h = dht.readHumidity();
    float t = dht.readTemperature();
    int ldrState = digitalRead(LDR_PIN); // 0 = Dark, 1 = Bright
    int mq135Raw = analogRead(MQ135_PIN);
    float mq135Volt = mq135Raw * (3.3 / 4095.0);

    if (isnan(h) || isnan(t)) {
      Serial.println("❌ Failed to read from DHT22 sensor!");
    } else {
      Serial.print("✅ Humidity: "); Serial.print(h);
      Serial.print(" %  Temp: "); Serial.print(t);
      Serial.print(" °C  Light: "); Serial.print(ldrState ? "Bright" : "Dark");
      Serial.print("  MQ135: "); Serial.print(mq135Raw);
      Serial.print(" ("); Serial.print(mq135Volt, 2); Serial.println(" V)");

      // Build telemetry string
      String msg = "T:" + String(t, 1) + "C H:" + String(h, 1) + "%" +
                   " L:" + (ldrState ? "Bright" : "Dark") +
                   " G:" + String(mq135Raw);
      String atCmd = "AT+SEND=1," + String(msg.length()) + "," + msg;
      LoRa.println(atCmd);
      Serial.println("📡 Sent telemetry: " + msg);
    }
  }
}
