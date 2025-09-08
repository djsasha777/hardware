
#include <WiFi.h>

const char* ssid     = "********";
const char* password = "********";

WiFiServer server(80);

int value = 0;
unsigned long previousMillis = 0;
unsigned long interval = 30000; 

void setup()
{
    Serial.begin(115200);
    pinMode(33, OUTPUT);      // set the LED pin mode
    digitalWrite(33, HIGH); 
    delay(10);
    Serial.println();
    Serial.println();
    Serial.print("Connecting to ");
    Serial.println(ssid);

    WiFi.begin(ssid, password);

    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }

    Serial.println("");
    Serial.println("WiFi connected.");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
    server.begin();

  pinMode(21, OUTPUT);
  pinMode(32, OUTPUT);

}

void loop(){
  unsigned long currentMillis = millis();
  // if WiFi is down, try reconnecting every CHECK_WIFI_TIME seconds
  if ((WiFi.status() != WL_CONNECTED) && (currentMillis - previousMillis >=interval)) {
    Serial.print(millis());
    Serial.println("Reconnecting to WiFi...");
    WiFi.disconnect();
    WiFi.reconnect();
    previousMillis = currentMillis;
  }
 WiFiClient client = server.available();   // listen for incoming clients

  if (client) {                             // if you get a client,
    Serial.println("New Client.");           // print a message out the serial port
    String currentLine = "";                // make a String to hold incoming data from the client
    while (client.connected()) {            // loop while the client's connected
      if (client.available()) {             // if there's bytes to read from the client,
        char c = client.read();             // read a byte, then
        Serial.write(c);                    // print it out the serial monitor
        if (c == '\n') {                    // if the byte is a newline character

          // if the current line is blank, you got two newline characters in a row.
          // that's the end of the client HTTP request, so send a response:
          if (currentLine.length() == 0) {
            // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
            // and a content-type so the client knows what's coming, then a blank line:
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println();

            client.println("<!DOCTYPE html><html>");
            client.println("<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
            client.println("<link rel=\"icon\" href=\"data:,\">");
            client.println("<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}");
            client.println(".button { background-color: #195B6A; border: none; color: white; padding: 16px 40px;");
            client.println("text-decoration: none; font-size: 30px; margin: 2px; cursor: pointer;}");
            client.println(".button2 {background-color: #77878A;}</style></head>");
            client.println("<body><h1>SPONGO SWITCH</h1>");
            client.println("<p><a href=\"/on\"><button class=\"button\">POWER ON</button></a></p>");
            client.println("<p><a href=\"/off\"><button class=\"button\">POWER OFF</button></a></p>");
            client.println("<p><a href=\"/1on\"><button class=\"button\">1 POWER ON</button></a></p>");
            client.println("<p><a href=\"/1off\"><button class=\"button\">1 POWER OFF</button></a></p>");
            client.println("<p><a href=\"/2on\"><button class=\"button\">2 POWER ON</button></a></p>");
            client.println("<p><a href=\"/2off\"><button class=\"button\">2 POWER OFF</button></a></p>");
            client.println("</body></html>");
            client.println();

            // The HTTP response ends with another blank line:
            client.println();
            // break out of the while loop:
            break;
          } else {    // if you got a newline, then clear currentLine:
            currentLine = "";
          }
        } else if (c != '\r') {  // if you got anything else but a carriage return character,
          currentLine += c;      // add it to the end of the currentLine
        }

        // Check to see if the client request was "GET /H" or "GET /L":
        if (currentLine.endsWith("GET /on")) {
          digitalWrite(33, LOW);               // GET /H turns the LED on
        }
        if (currentLine.endsWith("GET /off")) {
          digitalWrite(33, HIGH);                // GET /L turns the LED off
        }
        if (currentLine.endsWith("GET /1on")) {
          digitalWrite(21, HIGH);
        }
        if (currentLine.endsWith("GET /1off")) {
          digitalWrite(21, LOW);
        }
        if (currentLine.endsWith("GET /2on")) {
          digitalWrite(32, HIGH);
        }
        if (currentLine.endsWith("GET /2off")) {
          digitalWrite(32, LOW);
        }
      }
    }
    // close the connection:
    client.stop();
    Serial.println("Client Disconnected.");
  }
}

