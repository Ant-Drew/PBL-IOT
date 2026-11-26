// Libraries 
#include <Arduino.h>
#include <SPI.h> 
#include <Wire.h>
#include <WiFi.h>
#include <WebServer.h>
#include <MFRC522.h> 
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_BMP280.h>

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

//========== From Lab 6 ===========
Adafruit_BMP280 bmp;

//=====From Lab 8 ======
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &SPI, OLED_DC, OLED_RESET, OLED_CS);


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


const unsigned long SENSOR_TIME = 2000;              
unsigned long lastSensor = 0;
const unsigned long DISPLAY_TIME = 250;
unsigned long lastDisplay = 0;

//======== From lab 11 ============

const int NUM_SAMPLES = 10;                          
float temps[NUM_SAMPLES];
int sampleIndex = 0;
int sampleCount = 0;
float avgTemp = 0.0;

float setPoint = 24.0;

//==== from lab 12 & 2 ======= 

const unsigned long POT_TIME = 100;
unsigned long lastPot = 0;

const int PWM_FREQ = 5000;
const int PWM_RES = 8;
const int FAN_MIN_DUTY = 90; 


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

//============== From Lab 6 & 8 =====

void updateDisplay()
{
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);

  display.setTextSize(2);
  display.setCursor(0, 0);
  if ((statusByte & MASK_OCCUPIED) != 0)
  {
    display.println("OCCUPIED");
  }
  else
  {
    display.println("FREE");
  }

  display.setTextSize(1);
  display.setCursor(0, 24);
  display.print("Avg: ");
  if (sampleCount > 0)
  {
    display.print(avgTemp, 1);
    display.println(" C");
  }
  else
  {
    display.println("--");
  }
  display.print("Set: ");
  display.print(setPoint, 1);
  display.println(" C");

  if ((statusByte & MASK_HELP) != 0)
  {
    display.println("HELP REQUESTED");
  }
  else if ((statusByte & MASK_TOO_HOT) != 0)
  {
    display.println("Fan ON (too hot)");
  }
  else
  {
    display.println("Fan off");
  }
  display.display();
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

//======= From lab 11 ====== 

float getAverage()
{
  float sum = 0;
  for (int i = 0; i < sampleCount; i++)
  {
    sum = sum + temps[i];
  }
  return sum / sampleCount;
}

void readSetPoint()
{
  int potValue = analogRead(POT_PIN);
  setPoint = map(potValue, 0, 4095, 180, 320) / 10.0;
}

void updateFan()
{
  int duty = 0;
  if ((statusByte & MASK_TOO_HOT) != 0)
  {
    int over = (int)((avgTemp - setPoint) * 10);
    if (over > 50)
    {
      over = 50;
    }
    if (over < 0)
    {
      over = 0;
    }
    duty = map(over, 0, 50, FAN_MIN_DUTY, 255);
  }
  ledcWrite(FAN_PIN, duty);
}

//===== Other ========= 

void storeSample(float t)
{
  temps[sampleIndex] = t;
  sampleIndex++;
  if (sampleIndex >= NUM_SAMPLES)
  {
    sampleIndex = 0;
  }
  if (sampleCount < NUM_SAMPLES)
  {
    sampleCount++;
  }
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
   if ((statusByte & MASK_OCCUPIED) != 0 && sampleCount > 0 && avgTemp > setPoint)
  {
    statusByte |= MASK_TOO_HOT;
  }
  else
  {
    statusByte &= ~MASK_TOO_HOT;
  }
  
  updateFan();

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
  
  // ====== Bmp code from lab 6&7 ======
  Wire.begin();                 
  if (bmp.begin(0x76))
  {
    Serial.println("BMP280 Found at 0x76");
  }
  else if (bmp.begin(0x77))
  {
    Serial.println("BMP280 Found at 0x77");
  }
  else
  {
    Serial.println("BMP280 Not Found");
  }

  // ============== Lab 8 Oled code =====
  display.begin(SSD1306_SWITCHCAPVCC);
  Serial.println("OLED Started");

  lastSensor = millis() - SENSOR_TIME;

 //========== Lab 12 ============

  analogReadResolution(12);       
  analogSetAttenuation(ADC_11db);
  ledcAttach(FAN_PIN, PWM_FREQ, PWM_RES);
  ledcWrite(FAN_PIN, 0);



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

  //========= Lab 6&10 =============
  if (millis() - lastSensor >= SENSOR_TIME) // temp measurment time
  {
    lastSensor = millis();
    storeSample(bmp.readTemperature());
    avgTemp = getAverage();
  }


  
  if (millis() - lastDisplay >= DISPLAY_TIME) // Oled refresh
  {
    lastDisplay = millis();
    updateDisplay();
  }

  //====== Fan control with pot ========
  if (millis() - lastPot >= POT_TIME)
  {
    lastPot = millis();
    readSetPoint();
  }  

  updateOutputs();
  checkStatusChange();
}