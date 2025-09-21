#include <HardwareSerial.h>

HardwareSerial LoRa(1);  // UART1 for RYLR890

void setup() {
  Serial.begin(115200);  
  LoRa.begin(115200, SERIAL_8N1, 16, 17);  // RX=16, TX=17

  Serial.println("LoRa RYLR890 Transmitter");

  // Init LoRa
  LoRa.println("AT+RESET"); delay(1000);
  LoRa.println("AT+ADDRESS=1"); delay(200);
  LoRa.println("AT+NETWORKID=5"); delay(200);
  LoRa.println("AT+PARAMETER=12,7,1,4"); delay(200);
  LoRa.println("AT+BAND=868000000"); delay(200);

  Serial.println("WASD → LoRa ready");
}

void loop() {
  // ---- Send commands from Serial Monitor ----
  if (Serial.available()) {
    char cmd = Serial.read();
    String msg = String(cmd);
    String atCmd = "AT+SEND=2," + String(msg.length()) + "," + msg;
    LoRa.println(atCmd);
    Serial.print("➡ Sent: ");
    Serial.println(msg);
  }

  // ---- Receive telemetry from rover ----
  if (LoRa.available()) {
    String incoming = LoRa.readStringUntil('\n');
    if (incoming.startsWith("+RCV=")) {
      int secondComma = incoming.indexOf(',', incoming.indexOf(',') + 1);
      int thirdComma  = incoming.indexOf(',', secondComma + 1);
      if (secondComma != -1 && thirdComma != -1) {
        String payload = incoming.substring(secondComma + 1, thirdComma);
        payload.trim();

        Serial.println("📡 Raw Telemetry: " + payload);

        // --- Parse telemetry: T:xxC H:xx% L:Bright/Dark G:xxxx ---
        if (payload.indexOf("T:") != -1) {
          int tStart = payload.indexOf("T:") + 2;
          int tEnd = payload.indexOf("C");
          String temp = payload.substring(tStart, tEnd);

          int hStart = payload.indexOf("H:") + 2;
          int hEnd = payload.indexOf("%");
          String hum = payload.substring(hStart, hEnd);

          int lStart = payload.indexOf("L:") + 2;
          int gStart = payload.indexOf("G:");
          String light = payload.substring(lStart, gStart - 1);

          int gEnd = payload.length();
          String gas = payload.substring(gStart + 2, gEnd);

          // Print parsed values
          Serial.println("🌡 Temperature: " + temp + " °C");
          Serial.println("💧 Humidity: " + hum + " %");
          Serial.println("💡 Light: " + light);
          Serial.println("🟢 MQ135 Gas: " + gas);
                    Serial.println(" ");

        }
      }
    }
  }
}
