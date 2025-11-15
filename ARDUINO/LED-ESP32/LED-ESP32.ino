#include <WiFi.h>
#include <WebServer.h>
#include <Preferences.h>
#include <Update.h>

const char* ap_ssid = "ESP32_AP";
const char* ap_password = "12345678";
const char* hostname = "my-esp32-device";

IPAddress apIP(10,10,10,1);

WebServer mainServer(80);  // основной веб-сервер в режиме STA
WebServer apServer(80);    // вспомогательный веб-сервер в режиме AP

Preferences preferences;

String wifiSSID = "";
String wifiPassword = "";

bool shouldConnectToWiFi = false;

const int outputPin = 4;       // PWM вывод для светодиода/лампы
const int buttonPin = 5;
const int ledPin = 6;

int pwmValue = 0;              // Хранение текущего значения яркости (0-255)

bool lastButtonState = HIGH;
unsigned long lastDebounceTime = 0;
const unsigned long debounceDelay = 50;
const unsigned long resetHoldTime = 10000; // 10 секунд удержания кнопки
unsigned long buttonPressTime = 0;
bool resetTriggered = false;

// Для мигания светодиода в AP режиме
unsigned long lastLedToggle = 0;
const unsigned long ledInterval = 500; // 500 мс
bool ledOn = false;

// ===== Обработчики основного сервера (STA) =====

void handleLedValue() {
  if (mainServer.method() == HTTP_GET) {
    if (mainServer.hasArg("value")) {
      int value = mainServer.arg("value").toInt();
      if (value < 0) value = 0;
      if (value > 100) value = 100;
      pwmValue = map(value, 0, 100, 0, 255);
      ledcWrite(0, pwmValue);  // Используем канал 0 для PWM
      mainServer.send(200, "text/plain", "Value set to " + String(value));
      Serial.println("Set PWM value: " + String(value) + " -> PWM: " + String(pwmValue));
    } else {
      mainServer.send(400, "text/plain", "Missing 'value' parameter");
    }
  } else {
    mainServer.send(405, "text/plain", "Метод не поддерживается");
  }
}

void handleLedStatus() {
  if (mainServer.method() == HTTP_GET) {
    // Преобразуем pwmValue обратно в 0-100 для статуса
    int statusValue = map(pwmValue, 0, 255, 0, 100);
    mainServer.send(200, "text/plain", String(statusValue));
  } else {
    mainServer.send(405, "text/plain", "Метод не поддерживается");
  }
}

// ===== Обработчики вспомогательного сервера (AP) =====

void handleRoot() {
  String page = "<!DOCTYPE html><html><head><title>Управление пином</title></head><body>";
  page += "<h2>Управление яркостью LED</h2>";
  page += "<form action=\"/toggle\" method=\"POST\">";
  page += "<input type=\"submit\" value=\"" + String(pwmValue > 0 ? "Выключить" : "Включить") + "\">";
  page += "</form>";
  page += "</body></html>";
  apServer.send(200, "text/html", page);
}

void handleToggle() {
  if (apServer.method() == HTTP_POST) {
    if (pwmValue > 0) {
      pwmValue = 0;
    } else {
      pwmValue = 255;
    }
    ledcWrite(0, pwmValue);
    apServer.sendHeader("Location", "/");
    apServer.send(303);
  } else {
    apServer.send(405, "text/plain", "Метод не поддерживается");
  }
}

void handleState() {
  String stateString = pwmValue > 0 ? "ON" : "OFF";
  apServer.send(200, "text/plain", stateString);
}

void handleSave() {
  if (apServer.method() == HTTP_POST) {
    wifiSSID = apServer.arg("ssid");
    wifiPassword = apServer.arg("password");
    preferences.begin("wifi-creds", false);
    preferences.putString("ssid", wifiSSID);
    preferences.putString("password", wifiPassword);
    preferences.end();
    String response = "<!DOCTYPE html><html><head><title>Сохранено</title></head><body>";
    response += "<h2>Данные сохранены!</h2>";
    response += "<p>ESP теперь попытается подключиться к сети: " + wifiSSID + "</p>";
    response += "</body></html>";
    apServer.send(200, "text/html", response);
    shouldConnectToWiFi = true;
  } else {
    apServer.send(405, "text/plain", "Метод не поддерживается");
  }
}

void handleOTAPage() {
  String page = "<!DOCTYPE html><html><head><title>OTA Update</title></head><body>";
  page += "<h1>OTA обновление прошивки</h1>";
  page += "<form method='POST' action='/update' enctype='multipart/form-data'>";
  page += "<input type='file' name='firmware'>";
  page += "<input type='submit' value='Обновить'>";
  page += "</form></body></html>";
  apServer.send(200, "text/html", page);
}

void handleOTAUpload() {
  HTTPUpload& upload = apServer.upload();
  if (upload.status == UPLOAD_FILE_START) {
    Serial.printf("Начало загрузки файла: %s\n", upload.filename.c_str());
    if (!Update.begin()) {
      Update.printError(Serial);
    }
  } else if (upload.status == UPLOAD_FILE_WRITE) {
    if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
      Update.printError(Serial);
    }
  } else if (upload.status == UPLOAD_FILE_END) {
    if (Update.end(true)) {
      Serial.printf("Загрузка прошивки завершена: %u байт\n", upload.totalSize);
      apServer.send(200, "text/plain", "OK, перезагрузка...");
      delay(1000);
      ESP.restart();
    } else {
      Update.printError(Serial);
      apServer.send(500, "text/plain", "Ошибка обновления");
    }
  }
}

// ===== Общие функции =====

void updateLed() {
  if (WiFi.getMode() == WIFI_AP) {
    // В AP режиме светодиод мигает, мигание обрабатывается в loop()
    digitalWrite(ledPin, LOW);
  } else {
    // Устанавливаем текущий pwmValue
    ledcWrite(0, pwmValue);
  }
}

void startAccessPoint() {
  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(apIP, apIP, IPAddress(255,255,255,0));
  WiFi.softAP(ap_ssid, ap_password);

  apServer.on("/", handleRoot);
  apServer.on("/toggle", HTTP_POST, handleToggle);
  apServer.on("/state", handleState);
  apServer.on("/save", HTTP_POST, handleSave);

  apServer.on("/update", HTTP_GET, handleOTAPage);
  apServer.on("/update", HTTP_POST, []() {
    apServer.sendHeader("Connection", "close");
  }, handleOTAUpload);

  apServer.begin();
  Serial.println("Точка доступа запущена: " + String(ap_ssid));
  Serial.println("IP: " + WiFi.softAPIP().toString());

  ledOn = false;
  lastLedToggle = millis();
}

void connectToWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.setHostname(hostname);
  WiFi.begin(wifiSSID.c_str(), wifiPassword.c_str());
  Serial.println("Подключение к WiFi: " + wifiSSID);

  unsigned long startAttemptTime = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < 10000) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("Подключено к WiFi: " + wifiSSID);
    Serial.print("IP адрес: ");
    Serial.println(WiFi.localIP());

    // Настройка PWM канала (к примеру, канал 0, с частотой 5000, разрешение 8 бит)
    ledcSetup(0, 5000, 8);
    ledcAttachPin(outputPin, 0);
    updateLed();

    // Основной сервер маршруты
    mainServer.on("/led/1/value", handleLedValue);
    mainServer.on("/led/1/status", handleLedStatus);

    mainServer.begin();
  } else {
    Serial.println("Не удалось подключиться к WiFi");
    startAccessPoint();
  }
}

void setup() {
  Serial.begin(115200);

  pinMode(outputPin, OUTPUT);
  digitalWrite(outputPin, LOW);

  pinMode(buttonPin, INPUT_PULLUP);
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, LOW);

  lastButtonState = digitalRead(buttonPin);

  preferences.begin("wifi-creds", true);
  wifiSSID = preferences.getString("ssid", "");
  wifiPassword = preferences.getString("password", "");
  preferences.end();

  if (wifiSSID.length() > 0 && wifiPassword.length() > 0) {
    connectToWiFi();
  } else {
    startAccessPoint();
  }
}

void loop() {
  if (WiFi.getMode() == WIFI_AP) {
    apServer.handleClient();

    unsigned long now = millis();
    if (now - lastLedToggle >= ledInterval) {
      ledOn = !ledOn;
      digitalWrite(ledPin, ledOn ? HIGH : LOW);
      lastLedToggle = now;
    }
  } else if (WiFi.getMode() == WIFI_STA) {
    mainServer.handleClient();
  }

  int reading = digitalRead(buttonPin);
  if (reading != lastButtonState) {
    lastDebounceTime = millis();
  }

  if ((millis() - lastDebounceTime) > debounceDelay) {
    if (reading == LOW && !resetTriggered) {
      if (buttonPressTime == 0) {
        buttonPressTime = millis();
      } else {
        unsigned long held = millis() - buttonPressTime;
        if (held >= resetHoldTime) {
          preferences.begin("wifi-creds", false);
          preferences.clear();
          preferences.end();
          Serial.println("Сброс сохраненных WiFi данных. Перезагрузка...");
          resetTriggered = true;
          delay(1000);
          ESP.restart();
        }
      }
    } else if (reading == HIGH) {
      if (!resetTriggered && buttonPressTime != 0) {
        outputState = !outputState;
        pwmValue = outputState ? 255 : 0;
        updateLed();
        Serial.println("Физическая кнопка нажата, PWM изменено на " + String(pwmValue));
      }
      buttonPressTime = 0;
      resetTriggered = false;
    }
  }
  lastButtonState = reading;
}
