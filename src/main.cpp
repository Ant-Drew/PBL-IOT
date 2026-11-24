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


// Global vriables  
byte statusByte = 0b00000000;
byte lastStatus = 0b11111111; 


//======== From Lab 9 ===============  
MFRC522 rfid(RFID_SS, RFID_RST);                     
String authorizedUIDs[] = {"85 EF 2C 06"};
int numAuthorized = 1;
String lastUID = "";
unsigned long lastScanTime = 0;
unsigned long buzzerStart = 0;                
unsigned long buzzerTime = 0;
bool buzzerOn = false;

//======From lab 10 ===========================
const unsigned long BLINK_TIME = 150;
unsigned long lastBlink = 0;
int blinkToggles = 0;
bool redState = false;
volatile bool helpFlag = false;
volatile unsigned long lastInterruptHelp = 0;





// functions 

// ============ From Lab 2 ============== 
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
//======== from lab 9 ============================ 
String getUID()
{
  String uid = "";
  for (byte i = 0; i < rfid.uid.size; i++)
  {
    if (rfid.uid.uidByte[i] < 0x10)
    {
      uid += "0";
    }
    uid += String(rfid.uid.uidByte[i], HEX);
    if (i < rfid.uid.size - 1)
    {
      uid += " ";
    }
  }
  uid.toUpperCase();
  return uid;
}

bool isAuthorized(String uid)
{
  for (int i = 0; i < numAuthorized; i++)
  {
    if (uid == authorizedUIDs[i])
    {
      return true;
    }
  }
  return false;
}

void startBuzzer(unsigned long duration)
{
  digitalWrite(BUZZER, HIGH);
  buzzerStart = millis();
  buzzerTime = duration;
  buzzerOn = true;
}

void startRedFlash()
{
  redState = true;
  digitalWrite(RED_LED, HIGH);
  lastBlink = millis();
  blinkToggles = 5;
}

void onAccessGranted()
{
  Serial.println("ACCESS GRANTED");
  statusByte ^= MASK_OCCUPIED;
  statusByte &= ~MASK_HELP;
}
void onAccessDenied()
{
  Serial.println("ACCESS DENIED");
  startRedFlash();
  startBuzzer(1000);
}


//=======From lab 10 ==========================
void IRAM_ATTR helpButton_ISR()
{
  if (millis() - lastInterruptHelp > 200)
  {
    lastInterruptHelp = millis();
    helpFlag = true;
  }
}

void onCardScanned(String uid)
{
  if (uid == lastUID && millis() - lastScanTime < 2000)
  {
    Serial.println("Repeat ignored");
    return;
  }
  lastUID = uid;
  lastScanTime = millis();
  Serial.print("Card UID: ");
  Serial.println(uid);
  if (isAuthorized(uid) == true)
  {
    onAccessGranted();
  }
  else
  {
    onAccessDenied();
  }
}

void onHelpPressed()
{
  Serial.println("HELP requested");
  statusByte |= MASK_HELP;
  startBuzzer(1000);
}

void updateOutputs()
{
  if ((statusByte & MASK_OCCUPIED) != 0)
  {
    digitalWrite(GREEN_LED, HIGH);
  }
  else
  {
    digitalWrite(GREEN_LED, LOW);
  }
}


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
 
  attachInterrupt(digitalPinToInterrupt(HELP_BUTTON), helpButton_ISR, FALLING);// from lab 10

  //======== RFID code from lab 9 ======
  SPI.begin();                          
  rfid.PCD_Init();
  rfid.PCD_DumpVersionToSerial();


} 

 

void loop() { 

 //======== From Lab 9 =======
  if (rfid.PICC_IsNewCardPresent() == true && rfid.PICC_ReadCardSerial() == true) //RFID
  {
    String uid = getUID();
    onCardScanned(uid);
    rfid.PICC_HaltA();
    rfid.PCD_StopCrypto1();
  }

  if (buzzerOn == true && millis() - buzzerStart >= buzzerTime) //Buzzer
  {
    digitalWrite(BUZZER, LOW);
    buzzerOn = false;
  }

  //======= Frpm Lab 10 ======== 
  if (helpFlag == true)
  {
    helpFlag = false;
    onHelpPressed();
  }

  if (blinkToggles > 0 && millis() - lastBlink >= BLINK_TIME) //Red light flash
  {
    lastBlink = millis();
    redState = !redState;
    digitalWrite(RED_LED, redState);
    blinkToggles--;
  }

  updateOutputs();
  checkStatusChange();
}