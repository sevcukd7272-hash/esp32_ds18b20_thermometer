/*
 * ESP32 + DS18B20 — веб-термометр
 * ---------------------------------
 * Показує поточну температуру у веб-браузері.
 * Сторінка автоматично оновлюється кожні 2 секунди через AJAX.
 *
 * Необхідні бібліотеки (Library Manager в Arduino IDE):
 *   - OneWire (Paul Stoffregen)
 *   - DallasTemperature (Miles Burton)
 *
 * Підключення DS18B20:
 *   VDD  -> 3.3V
 *   GND  -> GND
 *   DATA -> GPIO 4 (можна змінити нижче, ONE_WIRE_BUS)
 *   Між DATA і VDD — підтягуючий резистор 4.7 кОм
 */

#include <WiFi.h>
#include <WebServer.h>
#include <OneWire.h>
#include <DallasTemperature.h>

// ---------- Налаштування Wi-Fi ----------
const char* ssid     = "Asus";
const char* password = "1135432906";

// ---------- Налаштування датчика ----------
#define ONE_WIRE_BUS 4          // пін, до якого підключено DATA датчика
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

WebServer server(80);

float temperatureC = NAN;
unsigned long lastReadTime = 0;
const unsigned long readInterval = 2000; // читати датчик кожні 2 с

// ---------- HTML сторінка ----------
const char htmlPage[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="uk">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>ESP32 Термометр</title>
  <style>
    body {
      font-family: Arial, sans-serif;
      background: #0f172a;
      color: #f1f5f9;
      display: flex;
      flex-direction: column;
      align-items: center;
      justify-content: center;
      height: 100vh;
      margin: 0;
    }
    .card {
      background: #1e293b;
      border-radius: 20px;
      padding: 40px 60px;
      text-align: center;
      box-shadow: 0 10px 30px rgba(0,0,0,0.4);
    }
    h1 { margin: 0 0 10px 0; font-size: 22px; font-weight: 400; color: #94a3b8; }
    .temp { font-size: 72px; font-weight: bold; color: #38bdf8; }
    .unit { font-size: 32px; color: #64748b; }
    .status { margin-top: 15px; font-size: 14px; color: #64748b; }
  </style>
</head>
<body>
  <div class="card">
    <h1>Температура (DS18B20)</h1>
    <div class="temp"><span id="temp">--</span><span class="unit">&deg;C</span></div>
    <div class="status" id="status">Оновлення...</div>
  </div>
  <script>
    async function updateTemp() {
      try {
        const response = await fetch('/temperature');
        const data = await response.json();
        document.getElementById('temp').textContent = data.temperature.toFixed(1);
        document.getElementById('status').textContent =
          'Оновлено: ' + new Date().toLocaleTimeString();
      } catch (e) {
        document.getElementById('status').textContent = 'Помилка з\'єднання';
      }
    }
    updateTemp();
    setInterval(updateTemp, 2000);
  </script>
</body>
</html>
)rawliteral";

// ---------- Обробники запитів ----------
void handleRoot() {
  server.send_P(200, "text/html", htmlPage);
}

void handleTemperature() {
  String json = "{\"temperature\":" + String(temperatureC, 2) + "}";
  server.send(200, "application/json", json);
}

void handleNotFound() {
  server.send(404, "text/plain", "Not found");
}

void setup() {
  Serial.begin(115200);
  sensors.begin();

  WiFi.begin(ssid, password);
  Serial.print("Підключення до Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.print("Підключено! IP-адреса: ");
  Serial.println(WiFi.localIP());

  server.on("/", handleRoot);
  server.on("/temperature", handleTemperature);
  server.onNotFound(handleNotFound);
  server.begin();
  Serial.println("Веб-сервер запущено");
}

void loop() {
  server.handleClient();

  if (millis() - lastReadTime >= readInterval) {
    lastReadTime = millis();
    sensors.requestTemperatures();
    float t = sensors.getTempCByIndex(0);
    if (t != DEVICE_DISCONNECTED_C) {
      temperatureC = t;
      Serial.print("Температура: ");
      Serial.print(temperatureC);
      Serial.println(" °C");
    } else {
      Serial.println("Помилка: датчик не знайдено!");
    }
  }
}
