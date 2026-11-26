#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BMP280.h>
#include <WiFi.h>
#include <WebServer.h>
#include <SPI.h>
#include <SD.h>

Adafruit_BMP280 bmp;


float temperature;

const char* ssid = "IOT-PBL-AC";
const char* password = "password123";

const int chipSelect = 10;
const int potPin = A0;
int sensorValue = 0;
unsigned long currentTime = 0

const int ledPin = 23;

File dataFile;

  int position = 0;
 
WebServer server(80);
// Paste the webpage code here	from lab 7
const char webpage[] PROGMEM = R"rawliteral(

<!DOCTYPE html>
<html>

<head>

<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">

<meta http-equiv="refresh" content="5">

<title>ESP32 Environmental Monitor</title>

<style>

body{
    font-family:Arial,sans-serif;
    background:linear-gradient(
        135deg,
        #1E3C72,
        #2A5298
    );
    color:white;
    text-align:center;
    margin:0;
    padding:20px;
}

.container{
    max-width:800px;
    margin:auto;
}

.card{
    background:rgba(
        255,
        255,
        255,
        0.15
    );

    padding:25px;
    border-radius:15px;

    box-shadow:
    0px 4px 12px rgba(
        0,
        0,
        0,
        0.3
    );
}

.value{
    font-size:42px;
    font-weight:bold;
    color:#FFD54F;
}

button{

    width:180px;
    height:60px;

    margin:10px;

    font-size:20px;

    border:none;

    border-radius:10px;

    cursor:pointer;
}

.on{
    background:#4CAF50;
    color:white;
}

.off{
    background:#F44336;
    color:white;
}

.info{
    margin-top:20px;
}

</style>

</head>

<body>

<div class="container">

<h1>🌎 ESP32 Environmental Monitor</h1>

<div class="card">

<h2>Temperature</h2>

<p class="value">
TEMP_PLACEHOLDER
</p>

</div>

<br>

<button class="on"
onclick="location.href='/on'">
LED ON
</button>

<button class="off"
onclick="location.href='/off'">
LED OFF
</button>

<div class="info">

<p>
SSID: ESP32_Lab7
</p>

<p>
IP Address: 192.168.4.1
</p>

</div>

</div>

</body>

</html>

)rawliteral";

void handleRoot()
{
  temperature = bmp.readTemperature();
  String page = webpage;
  page.replace("TEMP_PLACEHOLDER", String(temperature));
  server.send(200, "text/html", page);
}

void handleLEDOn()
{ 
  Serial.println("LED ON Route Accessed");
	  digitalWrite(ledPin, HIGH);
	    server.sendHeader("Location", "/" );
	    server.send(303);
}
void handleLEDOff()
{
  Serial.println("LED OFF Route Accessed");
	  digitalWrite( ledPin, LOW );
	    server.sendHeader("Location", "/" );
	    server.send(303);
}
// missing fan level & byte as JSON


void setup(){

   void setup() {
    Serial.begin(9600);
    Serial.println("Program Started");
    Serial.println("Initializing SD Card...");

      if(SD.begin(chipSelect)) {
      Serial.println("SD Card Found");
    }
      else {
      Serial.println("SD Card Not Found");
      }
 }

  Serial.begin(115200);
  Serial.println("Program Started");
  
  Wire.begin();
 
    if(bmp.begin(0x76))
    {
      Serial.println("BMP280 Found");
      }
    else
    {
      Serial.println("BMP280 Not Found");
   }

  WiFi.softAP(ssid, password);
    Serial.print("IP Address: ");
    Serial.println(WiFi.softAPIP());

  server.on( "/", handleRoot );
  server.on("/on", handleLEDOn);
  server.on("/off", handleLEDOff);

  server.begin();
    Serial.println("Web Server Started");

  pinMode( ledPin, OUTPUT);

  // BMP280 code
	// LED code
	// Wi-Fi code
	// Routes
	// Server start
}
void loop(){

  Serial.println(position);
    delay(1000);

  sensorValue = analogRead(potPin);
    Serial.print("Value = ");
    Serial.println(sensorValue);
      delay(500);

  currentTime = millis()/1000;
    Serial.print("Current Time: ");
    Serial.println(currentTime);
      delay(1000);

  // dataFile = SD.open("hello.txt", FILE_WRITE);
  // dataFile.println("Hello Dawson");
  // dataFile.close();

  // dataFile = SD.open("sensor.txt", FILE_WRITE);
  // dataFile.println(sensorValue);
  // dataFile.close();

  dataFile = SD.open("sensor.csv", FILE_WRITE);
  dataFile.println("Time,Value");
  dataFile.close();

   server.handleClient();
   // Handles client requests
}