#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>
#include <passwordStuff.h>
#include <Adafruit_GFX.h>    // Core graphics library
#include <Adafruit_ST7789.h> // Hardware-specific library for ST7789
//#include <WiFi.h>
#include <Adafruit_AHTX0.h>

//HAN Notes - change aht to something descriptive to the brief
Adafruit_AHTX0 aht;


////////////////////////////////////
const char SSID[]     = SECRET_SSID;
const char PASSWORD[] = SECRET_PASS;

WiFiServer server(80);

Adafruit_ST7789 tftScreen = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_RST);

const byte LEDPIN    = 13;
const byte SENSORPIN = A5; //HAN Notes - shouldn't need this anymore as using aht sensor

void initWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(SSID, PASSWORD);
  Serial.print("Connecting to WiFi...");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print('.');
    delay(1000);
  }

  //-------tft--------
  tftScreen.setCursor(0, 0);
  tftScreen.setTextColor(ST77XX_BLACK);
  tftScreen.setTextSize(1);
  tftScreen.setTextWrap(true);
  tftScreen.print("Connected to ");
  tftScreen.println(SSID);

  tftScreen.print("Use http://");
  tftScreen.println(WiFi.localIP());
}

/*********************
 * Setup stuff
 *********************/
void setup() {
  pinMode(LEDPIN, OUTPUT);
  pinMode(SENSORPIN, INPUT); //HAN Notes - shouldn't need this anymore as using aht sensor

  Serial.begin(115200);
  delay(5000);

  //------------the screen setup--------------
  pinMode(TFT_BACKLITE, OUTPUT);
  pinMode(TFT_I2C_POWER, OUTPUT);

  digitalWrite(TFT_BACKLITE, HIGH);
  digitalWrite(TFT_I2C_POWER, HIGH);
  delay(10);

  // initialize TFT
  tftScreen.init(135, 240);
  tftScreen.setRotation(3);
  tftScreen.fillScreen(ST77XX_WHITE);

  //------------WiFi setup-----------
  initWiFi();
//HAN Notes - will want to begin the aht sensor as well as the server
  
  server.begin();
}

// loop stuff
void loop() {
  WiFiClient client = server.available();
  if (client) {
    Serial.println("");
    String request = "";
    while (client.connected()) {
      if (client.available()) {
        char c = client.read();
        request += c;
        if (c == '\n') {
          // Check request
          if (request.indexOf("GET /fanOn") >= 0) {
            digitalWrite(LEDPIN, HIGH);
          }
          if (request.indexOf("GET /fanOff") >= 0) {
            digitalWrite(LEDPIN, LOW);
          }

          // Serve JSON data
          if (request.indexOf("GET /data") >= 0) {
            int sensorReading = analogRead(SENSORPIN);
            byte LEDReading = digitalRead(LEDPIN);

            client.println("HTTP/1.1 200 OK");
            client.println("Content-Type: application/json");
            client.println("Connection: close");
            client.println();
            client.print("{\"sensor\":");
            client.print(sensorReading);
            client.print(",\"led\":");
            client.print(LEDReading);
            client.println("}");
            break;
          }

          // Serve main page
          if (request.indexOf("GET / ") >= 0) {
            client.println("HTTP/1.1 200 OK");
            client.println("Content-Type: text/html");
            client.println("Connection: close");
            client.println();
            client.println("<!DOCTYPE HTML>");
            client.println("<html><head><style>");
            client.println("html{font-family: Arial; text-align: center;}");
            client.println("button{padding: 10px 20px; font-size: 16px; margin: 10px;}");
            client.println("</style></head><body>");

            client.println("<h1>Sensor + LED Control</h1>");
            client.println("<p><a href=\"/fanOn\"><button>Turn Fan ON</button></a></p>");
            client.println("<p><a href=\"/fanOff\"><button>Turn Fan OFF</button></a></p>");

            client.println("<p>Sensor value: <span id=\"sensor\">---</span></p>");
            client.println("<p>LED is: <span id=\"led\">---</span></p>");

            // JavaScript to update values without refreshing
            client.println("<script>");
            client.println("setInterval(function(){");
            client.println("fetch('/data').then(res=>res.json()).then(data=>{");
            client.println("document.getElementById('sensor').innerHTML=data.sensor;");
            client.println("document.getElementById('led').innerHTML=(data.led==1?'ON':'OFF');");
            client.println("});");
            client.println("}, 1000);"); // update every 1s
            client.println("</script>");

            client.println("</body></html>");
            break;
          }
          request = "";
        }
      }
    }
    client.stop();
    Serial.println("Client disconnected");
  }
}
