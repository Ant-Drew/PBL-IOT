// Libraries 
#include <Arduino.h>
#include <SPI.h> 
#include <Wire.h>
#include <WiFi.h>
#include <WebServer.h>
#include <MFRC522.h> 
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

//pinout (inspired by labs)
#define RFID_SS      5
#define RFID_RST     4
#define OLED_DC      16
#define OLED_RESET   17
#define OLED_CS      14
#define GREEN_LED    25
#define RED_LED      26
#define BUZZER       27
#define HELP_BUTTON  33
#define POT_PIN      34
#define FAN_PIN      13

//OLED size(from lab 8)
#define SCREEN_WIDTH  128
#define SCREEN_HEIGHT 64

//Masks (From lab 2)
#define MASK_OCCUPIED  0b00000001
#define MASK_TOO_HOT   0b00000010
#define MASK_HELP      0b00000100

byte statusByte = 0b00000000;
byte lastStatus = 0b11111111; 

// ============ Lab 2 ============== 
void printBinary8(byte value)
{
  for (int i = 7; i >= 0; i--)
  {
    Serial.print((value >> i) & 1);
  }
}

void checkStatusChange()
{
  if (statusByte != lastStatus)
  {
    lastStatus = statusByte;
    Serial.print("Status: ");
    printBinary8(statusByte);
    Serial.println();
  }
} 
//=====================================




void setup() { 
 Serial.begin(115200);
 Serial.println("Program Started");
// Pin modes 
  pinMode(GREEN_LED, OUTPUT);
  pinMode(RED_LED, OUTPUT);
  pinMode(BUZZER, OUTPUT);
  pinMode(HELP_BUTTON, INPUT_PULLUP);
  digitalWrite(GREEN_LED, LOW);
  digitalWrite(RED_LED, LOW);
  digitalWrite(BUZZER, LOW);
 

} 

 

void loop() { 
  checkStatusChange();
}