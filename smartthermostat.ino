#define BLYNK_TEMPLATE_ID "TMPL3rUfZQ5VP"
#define BLYNK_TEMPLATE_NAME "Smart ThermoStat"
#define BLYNK_AUTH_TOKEN    "7XqDIuBdnR8Iqo0TCbNDtFsVlOiaNu6w"

/* Temperature & Humidity Monitoring + LED Control (Blynk) */

#define BLYNK_PRINT Serial
#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <DHT.h>

LiquidCrystal_I2C lcd(0x27, 16, 2);

// ----- BLYNK AUTH -----
char auth[] = "7XqDIuBdnR8Iqo0TCbNDtFsVlOiaNu6w";
char ssid[] = "iot";
char pass[] = "12345678";

// ----- DHT SETTINGS -----
DHT dht(D3, DHT11);

// ----- LED PIN -----
#define LED_PIN D5

// ----- TEMP THRESHOLD -----
#define TEMP_THRESHOLD 40.0

// ----- MODE FLAGS -----
bool autoModeEnabled = true;   // V3 button controls this
                               // true  = auto LED on temp > 40°C
                               // false = manual only via V2

BlynkTimer timer;

// ==============================
//   V3 — AUTO MODE TOGGLE
// ==============================
BLYNK_WRITE(V3) {
  autoModeEnabled = param.asInt();   // 1 = auto ON, 0 = auto OFF

  if (autoModeEnabled) {
    Serial.println("Auto mode: ON");
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Auto Mode: ON");
    lcd.setCursor(0, 1);
    lcd.print("Temp ctrl active");
  } else {
    Serial.println("Auto mode: OFF");
    digitalWrite(LED_PIN, LOW);      // immediately turn off LED
    Blynk.virtualWrite(V2, 0);      // reset LED button in app
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Auto Mode: OFF");
    lcd.setCursor(0, 1);
    lcd.print("Manual ctrl only");
    delay(1500);                     // show message briefly
  }
}

// ==============================
//   V2 — MANUAL LED BUTTON
// ==============================
BLYNK_WRITE(V2) {
  if (!autoModeEnabled) {
    // Only allow manual control when auto mode is OFF
    int ledState = param.asInt();
    digitalWrite(LED_PIN, ledState);
    Serial.print("Manual LED: ");
    Serial.println(ledState ? "ON" : "OFF");
  } else {
    // Auto mode is ON — reject manual, re-sync button to actual state
    int currentLED = digitalRead(LED_PIN);
    Blynk.virtualWrite(V2, currentLED);
    Serial.println("Manual blocked — auto mode is ON");
  }
}

// ==============================
//      SEND SENSOR DATA
// ==============================
void sendSensor() {
  float h = dht.readHumidity();
  float t = dht.readTemperature();

  if (isnan(h) || isnan(t)) {
    Serial.println("Failed to read from DHT sensor!");
    return;
  }

  // Send data to Blynk regardless of mode
  Blynk.virtualWrite(V0, t);
  Blynk.virtualWrite(V1, h);

  Serial.print("Temp: ");
  Serial.print(t);
  Serial.print("C  Humidity: ");
  Serial.print(h);
  Serial.print("%  AutoMode: ");
  Serial.println(autoModeEnabled ? "ON" : "OFF");

  if (autoModeEnabled) {
    // ── AUTO MODE ──
    if (t > TEMP_THRESHOLD) {
      digitalWrite(LED_PIN, HIGH);
      Blynk.virtualWrite(V2, 1);

      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Temp : ");
      lcd.print(t);
      lcd.print("C");
      lcd.setCursor(0, 1);
      lcd.print("!! ALERT >40C !!");

    } else {
      digitalWrite(LED_PIN, LOW);
      Blynk.virtualWrite(V2, 0);

      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Temp : ");
      lcd.print(t);
      lcd.print("C");
      lcd.setCursor(0, 1);
      lcd.print("Humi : ");
      lcd.print(h);
      lcd.print("%");
    }

  } else {
    // ── MANUAL MODE — just update LCD, don't touch LED ──
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Temp : ");
    lcd.print(t);
    lcd.print("C");
    lcd.setCursor(0, 1);
    lcd.print("Humi : ");
    lcd.print(h);
    lcd.print("%");
  }
}

// ==============================
//     SYNC ON RECONNECT
// ==============================
BLYNK_CONNECTED() {
  Blynk.syncVirtual(V2);   // restore LED button state
  Blynk.syncVirtual(V3);   // restore auto mode state
}

// ==============================
//           SETUP
// ==============================
void setup() {
  Serial.begin(9600);

  Wire.begin(D2, D1);
  lcd.begin(16, 2);
  lcd.backlight();

  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  dht.begin();

  Blynk.begin(auth, ssid, pass, "blynk.cloud", 80);

  lcd.setCursor(1, 0);
  lcd.print("System Loading");

  for (int a = 0; a <= 15; a++) {
    lcd.setCursor(a, 1);
    lcd.print(".");
    delay(200);
  }

  lcd.clear();

  timer.setInterval(1000L, sendSensor);
}

// ==============================
//           LOOP
// ==============================
void loop() {
  Blynk.run();
  timer.run();
}