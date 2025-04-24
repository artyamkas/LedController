#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ArduinoJson.h>
#include <math.h>

// ——— Настройки Wi-Fi —————————————————————————————————————————————————
const char* WIFI_SSID     = "Redmi Note 12";
const char* WIFI_PASSWORD = "sarik323";

// ——— Пины ленты и микрофона ————————————————————————————————————————
#define PIN_R   D5
#define PIN_G   D3
#define PIN_B   D2
#define PIN_MIC A0

// ——— Глобальные переменные состояния —————————————————————————————
ESP8266WebServer server(80);

bool   isOn = false;
uint8_t r    = 255;
uint8_t g    = 255;
uint8_t b    = 255;
uint8_t mode = 0;  // 0=статичный, 1=дыхание, 2=радуга, 3=музыка

void setup() {
  Serial.begin(9600);
  
  // Инициализация пинов
  pinMode(PIN_R, OUTPUT);
  pinMode(PIN_G, OUTPUT);
  pinMode(PIN_B, OUTPUT);
  analogWrite(PIN_R, 0);  // Начальное состояние выключено
  analogWrite(PIN_G, 0);
  analogWrite(PIN_B, 0);

  // Подключение к Wi-Fi
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.print("Ready, IP = ");
  Serial.println(WiFi.localIP());

  // Маршруты веб-сервера
  server.on("/", handleRoot);
  server.on("/on", handleOn);
  server.on("/off", handleOff);
  server.on("/color", handleColor);
  server.on("/mode", handleModeSetting);
  server.begin();
}

void loop() {
  server.handleClient();
  handleModeAnimations();
}

// ——— Обработчики веб-запросов ———————————————————————————————————
void handleRoot() {
  String html = "<!DOCTYPE html><html>"
                "<head><title>LED Control</title></head>"
                "<body>"
                "<h1>LED Strip Control</h1>"
                "<p><a href='/on'><button>Включить</button></a> "
                "<a href='/off'><button>Выключить</button></a></p>"
                "<form action='/color' method='GET'>"
                "R: <input type='number' name='r' min=0 max=255 value='" + String(r) + "'><br>"
                "G: <input type='number' name='g' min=0 max=255 value='" + String(g) + "'><br>"
                "B: <input type='number' name='b' min=0 max=255 value='" + String(b) + "'><br>"
                "<input type='submit' value='Установить цвет'>"
                "</form>"
                "<form action='/mode' method='GET'>"
                "Режим: <select name='m'>"
                "<option value='0'>Статичный</option>"
                "<option value='1'>Дыхание</option>"
                "<option value='2'>Радуга</option>"
                "<option value='3'>Музыка</option>"
                "</select>"
                "<input type='submit' value='Выбрать режим'>"
                "</form>"
                "</body></html>";
  server.send(200, "text/html", html);
}

void handleOn() {
  isOn = true;
  applyColor(r, g, b);  // Применяем сохраненный цвет
  server.send(200, "application/json", "{\"status\":\"on\"}");
}

void handleOff() {
  isOn = false;
  applyColor(0, 0, 0);  // Выключаем ленту
  server.send(200, "application/json", "{\"status\":\"off\"}");
}

void handleColor() {
  if (server.hasArg("r")) r = server.arg("r").toInt();
  if (server.hasArg("g")) g = server.arg("g").toInt();
  if (server.hasArg("b")) b = server.arg("b").toInt();
  
  r = constrain(r, 0, 255);
  g = constrain(g, 0, 255);
  b = constrain(b, 0, 255);
  
  if(isOn && mode == 0) {
    applyColor(r, g, b);
  }
  
  server.send(200, "application/json", 
    "{ \"r\": " + String(r) + 
    ", \"g\": " + String(g) + 
    ", \"b\": " + String(b) + " }");
}

void handleModeSetting() {
  if (server.hasArg("m")) {
    mode = server.arg("m").toInt();
    if(mode < 0 || mode > 3) mode = 0;
    Serial.print("Режим изменен на: "); Serial.println(mode);  // Отладка
  }
  server.send(200, "application/json", "{ \"mode\": " + String(mode) + " }");
}

// ——— Управление лентой ———————————————————————————————————————
void applyColor(uint8_t R, uint8_t G, uint8_t B) {
  // Выводим текущие значения для отладки
  Serial.print("R: "); Serial.print(R); 
  Serial.print(" (PWM: "); Serial.print(255 - R); Serial.print(")");
  Serial.print(" | G: "); Serial.print(G); 
  Serial.print(" (PWM: "); Serial.print(255 - G); Serial.print(")");
  Serial.print(" | B: "); Serial.print(B); 
  Serial.print(" (PWM: "); Serial.println(255 - B); Serial.print(")");
  
  analogWrite(PIN_R, 255 - R);
  analogWrite(PIN_G, 255 - G);
  analogWrite(PIN_B, 255 - B);
}

void handleModeAnimations() {
  if(!isOn) return;

  static unsigned long lastUpdate = 0;
  static uint8_t rainbowPhase = 0;

  switch(mode) {
    case 1:  // Дыхание
      if(millis() - lastUpdate > 30) {
        float brightness = (sin(millis()/1000.0 * PI) + 1) / 2;
        applyColor(r * brightness, g * brightness, b * brightness);
        lastUpdate = millis();
      }
      break;

    case 2:  // Радуга
      if(millis() - lastUpdate > 20) {
        rainbowPhase = (rainbowPhase + 1) % 255;
        applyColor(
          wheel((rainbowPhase) % 255),
          wheel((rainbowPhase + 85) % 255),
          wheel((rainbowPhase + 170) % 255)
        );
        lastUpdate = millis();
      }
      break;

    case 3:  // Музыка
      int micValue = analogRead(PIN_MIC);
      int level = map(micValue, 0, 1023, 0, 255);
      level = constrain(level, 0, 255);

      // Добавьте эти строки для вывода в Serial
      Serial.print("Микрофон: ");
      Serial.print(micValue);
      Serial.print(" | Уровень: ");
      Serial.println(level);

      applyColor(r * level/255, g * level/255, b * level/255);
      break;
  }
}

// Вспомогательная функция для радуги
uint32_t wheel(byte pos) {
  pos = 255 - pos;
  if(pos < 85) return (255 - pos * 3) << 16 | 0 << 8 | pos * 3;
  if(pos < 170) { pos -= 85; return 0 << 16 | pos * 3 << 8 | (255 - pos * 3); }
  pos -= 170; return pos * 3 << 16 | (255 - pos * 3) << 8 | 0;
}