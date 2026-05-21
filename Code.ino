#include <U8g2lib.h>
#include <Wire.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include <NTPClient.h>

// WiFi
const char* ssid = "PLDTHOMEFIBRa44FT";
const char* password = "PLDTWIFIa44ft";

// OLED SH1106
U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);

// NTP
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 28800, 60000);

// Alarm
int alarmHour = 11;
int alarmMinute = 00;
bool alarmTriggered = false;
bool alarmFiredOnce = false;

// Blink
unsigned long lastBlink = 0;
bool showColon = true;

// Buzzer
#define BUZZER 25

void setup() {
  Serial.begin(115200);

  pinMode(BUZZER, OUTPUT);
  digitalWrite(BUZZER, LOW);

  // OLED init
  Wire.begin(21, 22);
  Wire.setClock(400000);
  u8g2.begin();

  // START SCREEN (only temporary)
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_ncenB08_tr);
  u8g2.drawStr(10, 30, "Starting...");
  u8g2.sendBuffer();
  delay(1000);

  // WIFI (FIXED: no infinite freeze)
  WiFi.begin(ssid, password);

  unsigned long startAttempt = millis();

  while (WiFi.status() != WL_CONNECTED && millis() - startAttempt < 15000) {
    delay(500);
    Serial.print(".");
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi Connected!");
  } else {
    Serial.println("\nWiFi Failed - Offline mode");
  }

  // NTP start (safe)
  timeClient.begin();
  timeClient.update();
}

void loop() {

  // safe NTP update
  if (!timeClient.update()) {
    timeClient.forceUpdate();
  }

  int h = timeClient.getHours();
  int m = timeClient.getMinutes();
  int s = timeClient.getSeconds();

  // reset alarm daily
  if (h == 0 && m == 0) {
    alarmFiredOnce = false;
    alarmTriggered = false;
  }

  // trigger alarm once
  if (h == alarmHour && m == alarmMinute && !alarmFiredOnce) {
    alarmTriggered = true;
    alarmFiredOnce = true;
  }

  // alarm buzzer (simple ON/OFF)
  static unsigned long alarmStart = 0;

  if (alarmTriggered && alarmStart == 0) {
    alarmStart = millis();
  }

  if (alarmTriggered) {
    digitalWrite(BUZZER, HIGH);

    if (millis() - alarmStart > 10000) {
      alarmTriggered = false;
      digitalWrite(BUZZER, LOW);
      alarmStart = 0;
    }
  }

  // blink colon
  if (millis() - lastBlink > 500) {
    lastBlink = millis();
    showColon = !showColon;
  }

  char timeStr[10];

  if (showColon)
    sprintf(timeStr, "%02d:%02d", h, m);
  else
    sprintf(timeStr, "%02d %02d", h, m);

  // ================= OLED =================
  u8g2.clearBuffer();

  // BIG CLOCK
  u8g2.setFont(u8g2_font_logisoso28_tr);

  int w = u8g2.getStrWidth(timeStr);
  u8g2.drawStr((128 - w) / 2, 32, timeStr);

  // STATUS
  u8g2.setFont(u8g2_font_6x10_tr);

  char status[30];

  if (alarmTriggered) {
    sprintf(status, "ALARM!!!");
  } else {
    sprintf(status, "Alarm %02d:%02d", alarmHour, alarmMinute);
  }

  u8g2.drawStr(20, 55, status);

  u8g2.sendBuffer();

  delay(50);
}
