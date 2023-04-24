#include <HCSR04.h>

UltraSonicDistanceSensor distanceSensor(2, 3);  // Initialize sensor that uses digital pins 13 and 12.

//YWROBOT
//Compatible with the Arduino IDE 1.0
//Library version:1.1
#include <Wire.h> 
#include <LiquidCrystal_I2C.h>
#include "DFRobotDFPlayerMini.h"
#include <DS3231.h>
#include <MFRC522.h>

#define SS_PIN 48
#define RST_PIN 5
 
MFRC522 rfid(SS_PIN, RST_PIN); // Instance of the class

MFRC522::MIFARE_Key key; 

// Init array that will store new NUID 
byte nuidPICC[4];

DS3231 myRTC;
bool century = false;
bool h12Flag;
bool pmFlag;
int minuti;
int oldminuti;
int n = 0;
int m = 0;
String giorniset[7] = {"do", "lu", "ma", "me", "gi", "ve", "sa"};

// Create the Player object
DFRobotDFPlayerMini player;

unsigned long previousMillis = 0;
const long interval = 10000;

LiquidCrystal_I2C lcd(0x3F,16,2);  // set the LCD address to 0x27 for a 16 chars and 2 line display
float distance = 999;
boolean newcard;

void setup() {
  Wire.begin();
  SPI.begin(); // Init SPI bus
  // Init USB serial port for debugging
  Serial1.begin(9600);
  Serial.begin(9600);
  // Start communication with DFPlayer Mini
  if (player.begin(Serial1)) {
    Serial.println("OK");

    // Set volume to maximum (0 to 30).
    player.volume(30);
  } else {
    Serial.println("Connecting to DFPlayer Mini failed!");
  }
  lcd.init();                      // initialize the lcd 
  lcd.init();
  // Print a message to the LCD.
  lcd.backlight();
  lcd.print("sul lettore");
  rfid.PCD_Init(); // Init MFRC522 

  for (byte i = 0; i < 6; i++) {
    key.keyByte[i] = 0xFF;
  }

  Serial.println(F("This code scan the MIFARE Classsic NUID."));
  Serial.print(F("Using the following key:"));
  printHex(key.keyByte, MFRC522::MF_KEY_SIZE);
}


void loop() {
  while(distance >= 30 || distance == -1 ) {
    distance = distanceSensor.measureDistanceCm();
    Serial.println(distance);
    minuti = myRTC.getMinute();
    if (minuti != oldminuti) {
      scrividataora();
      oldminuti = minuti;
    } 
    if (n == 0 && m == 0) {
      lcd.setCursor(0,1);
      lcd.print("In attesa       ");
    }
    if (n == 1 && m == 0) {
      lcd.setCursor(0,1);
      lcd.print("di leggere      ");
    }
    if (n == 2 && m == 0) {
      lcd.setCursor(0,1);
      lcd.print("la chiavetta    ");
    }
    delay(250);
    m = m + 1;
    if (m == 7) {
      n = n + 1;
      m = 0;
    }
    if (n == 3) n = 0;
  }
  player.play(1);
  lcd.setCursor(0,0);
  lcd.print("Poni la chiave  ");
  lcd.setCursor(0,1);
  lcd.print("sul lettore     ");
  previousMillis = millis();
  while (millis() - previousMillis < interval) {
    readRFID();
    if (newcard) {
      exit;
    }
  }
  distance = 999;
  oldminuti = 99;
  Serial.println("finito");
}

void scrividataora() {
  lcd.setCursor(0,0);
  char buffer[11]; //buffer size defined
  int ngs = myRTC.getDoW();
  String gs = giorniset[ngs - 1];
  Serial.println(gs);
  lcd.print(gs);
  int anno = myRTC.getYear();
  int mese = myRTC.getMonth(century);
  int giorno = myRTC.getDate();
  sprintf(buffer, "%02d/%02d/%02d", giorno, mese, anno);
  lcd.setCursor(2,0);
  lcd.print(buffer);
  lcd.print("-");
  buffer[5]; //buffer size defined
  // Finally the hour, minute, and second
  int ore = myRTC.getHour(h12Flag, pmFlag);
  sprintf(buffer, "%02d:%02d", ore, minuti);
  lcd.setCursor(11,0);
  lcd.print(buffer);
}

void printHex(byte *buffer, byte bufferSize) {
  for (byte i = 0; i < bufferSize; i++) {
    Serial.print(buffer[i] < 0x10 ? "0" : "");
    Serial.print(buffer[i], HEX);
  }
}

void readRFID() {
  // Reset the loop if no new card present on the sensor/reader. This saves the entire process when idle.
  if ( ! rfid.PICC_IsNewCardPresent())
    return;

  // Verify if the NUID has been readed
  if ( ! rfid.PICC_ReadCardSerial())
    return;

  Serial.print(F("PICC type: "));
  MFRC522::PICC_Type piccType = rfid.PICC_GetType(rfid.uid.sak);
  Serial.println(rfid.PICC_GetTypeName(piccType));

  // Check is the PICC of Classic MIFARE type
  if (piccType != MFRC522::PICC_TYPE_MIFARE_MINI &&  
    piccType != MFRC522::PICC_TYPE_MIFARE_1K &&
    piccType != MFRC522::PICC_TYPE_MIFARE_4K) {
    Serial.println(F("Your tag is not of type MIFARE Classic."));
    return;
  }

  // Store NUID into nuidPICC array
  for (byte i = 0; i < 4; i++) {
    nuidPICC[i] = rfid.uid.uidByte[i];
  }
   
  Serial.println(F("The NUID tag is:"));
  Serial.print(F("In hex: "));
  printHex(rfid.uid.uidByte, rfid.uid.size);
  Serial.println();
  newcard = true;

  // Halt PICC
  rfid.PICC_HaltA();

  // Stop encryption on PCD
  rfid.PCD_StopCrypto1();
}
