#include <WiFi.h>
#include <WebServer.h>
#include <Preferences.h>
#include <Update.h>

const char* ap_ssid = "ESP32_AP";
const char* ap_password = "12345678";
const char* hostname = "my-esp32-device";

IPAddress apIP(10,10,10,1);
WebServer server(80);

Preferences preferences;

String wifiSSID = "";
String wifiPassword = "";

bool shouldConnectToWiFi = false;

const int outputPin = 4;
const int buttonPin = 5;
const int ledPin = 6;

bool outputState = false;
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

void handleRoot() {
  String page = "<!DOCTYPE html><html><head><title>Управление пином</title></head><body>";
  page += "<h2>Управление цифровым пином 4</h2>";
  page += "<form action=\"/toggle\" method=\"POST\">";
  page += "<input type=\"submit\" value=\"" + String(outputState ? "Выключить" : "Включить") + "\">";
  page += "</form>";
  page += "</body></html>";
  server.send(200, "text/html", page);
}

void handleToggle() {
  if (server.method() == HTTP_POST) {
    outputState = !outputState;
    digitalWrite(outputPin, outputState ? HIGH : LOW);
    updateLed(); // Обновляем светодиод согласно новому состоянию
    server.sendHeader("Location", "/");
    server.send(303);
  } else {
    server.send(405, "text/plain", "Метод не поддерживается");
  }
}

void handleState() {
  String stateString = outputState ? "ON" : "OFF";
  server.send(200, "text/plain", stateString);
}

void handleSave() {
  if (server.method() == HTTP_POST) {
    wifiSSID = server.arg("ssid");
    wifiPassword = server.arg("password");

    preferences.begin("wifi-creds", false);
    preferences.putString("ssid", wifiSSID);
    preferences.putString("password", wifiPassword);
    preferences.end();

    String response = "<!DOCTYPE html><html><head><title>Сохранено</title></head><body>";
    response += "<h2>Данные сохранены!</h2>";
    response += "<p>ESP теперь попытается подключиться к сети: " + wifiSSID + "</p>";
    response += "</body></html>";
    server.send(200, "text/html", response);

    shouldConnectToWiFi = true;
  } else {
    server.send(405, "text/plain", "Метод не поддерживается");
  }
}

// Отображение страницы OTA обновления
void handleOTAPage() {
  String page = "<!DOCTYPE html><html><head><title>OTA Update</title></head><body>";
  page += "<h1>OTA обновление прошивки</h1>";
  page += "<form method='POST' action='/update' enctype='multipart/form-data'>";
  page += "<input type='file' name='firmware'>";
  page += "<input type='submit' value='Обновить'>";
  page += "</form></body></html>";
  server.send(200, "text/html", page);
}

// Обработка загрузки OTA прошивки
void handleOTAUpload() {
  HTTPUpload& upload = server.upload();
  if (upload.status == UPLOAD_FILE_START) {
    Serial.printf("Начало загрузки файла: %s\n", upload.filename.c_str());
    if (!Update.begin()) { // можно Specify max size if known
      Update.printError(Serial);
    }
  } else if (upload.status == UPLOAD_FILE_WRITE) {
    if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
      Update.printError(Serial);
    }
  } else if (upload.status == UPLOAD_FILE_END) {
    if (Update.end(true)) {
      Serial.printf("Загрузка прошивки завершена: %u байт\n", upload.totalSize);
      server.send(200, "text/plain", "OK, перезагрузка...");
      delay(1000);
      ESP.restart();
    } else {
      Update.printError(Serial);
      server.send(500, "text/plain", "Ошибка обновления");
    }
  }
}

void updateLed() {
  if (WiFi.getMode() == WIFI_AP) {
    // В AP режиме светодиод мигает, это обработается в loop()
    digitalWrite(ledPin, LOW);
  } else {
    digitalWrite(ledPin, outputState ? HIGH : LOW);
  }
}

void startAccessPoint() {
  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(apIP, apIP, IPAddress(255,255,255,0));
  WiFi.softAP(ap_ssid, ap_password);

  server.on("/", handleRoot);
  server.on("/toggle", handleToggle);
  server.on("/state", handleState);
  server.on("/save", handleSave);

  server.on("/update", HTTP_GET, handleOTAPage);
  server.on("/update", HTTP_POST, []() {
    server.sendHeader("Connection", "close");
  }, handleOTAUpload);

  server.begin();
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
  } else {
    Serial.println("Не удалось подключиться к WiFi");
    startAccessPoint();
  }
  updateLed();
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
    server.handleClient();

    if (shouldConnectToWiFi) {
      delay(1000);
      shouldConnectToWiFi = false;
      connectToWiFi();
    }

    unsigned long now = millis();
    if (now - lastLedToggle >= ledInterval) {
      ledOn = !ledOn;
      digitalWrite(ledPin, ledOn ? HIGH : LOW);
      lastLedToggle = now;
    }
  } else {
    server.handleClient();
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
        digitalWrite(outputPin, outputState ? HIGH : LOW);
        updateLed();
        Serial.println("Физическая кнопка нажата, состояние пина 4 изменено на " + String(outputState ? "ВКЛ" : "ВЫКЛ"));
      }
      buttonPressTime = 0;
      resetTriggered = false;
    }
  }
  lastButtonState = reading;
}
