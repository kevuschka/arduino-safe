#include "TM1637.h"
#include "LowPower.h"
#include <Servo.h>
#include <I2CKeyPad.h>
#include <Wire.h>
#include <EEPROM.h>

// Password Object
struct Password {
  bool set;
  int8_t secret[4];
};

// Konstanten definieren
#define KeyPad_I2C_ADDR 0x21

// Variablen definieren
int sequenceNum = 0;
bool keyPressed = false; // um ein Debounce zu verhindern
bool turnedOn = true; // nach verlassen des Energiesparmodus, sollen einmalig einige Einstellungen ausgeführt werden
bool doorOpen = true; // um einmalig die Energiesparmodus-Einstelllugen festzulegen
bool passwordIsSet = false; // gibt an, ob ein Passwort im Speicher hinterlegt ist
int8_t passwordTemp[4]; // hält das Passwort aus dem Speicher, um damit zu arbeiten
unsigned long onTime = 0; // Zeit in Millisekunden, seit dem der Arduino an ist
unsigned long onTime_p = 0; // letzte Zeit in Millisec., in der ein Input geschah
unsigned long dispTime = 0; // Zeit in Millisekunden fuer das Display
unsigned long dispTime_p  = 0; // abgleich der verstrichenen Zeit
unsigned long keyTime = 0; // Zeit in Millisekunden fuer das Keypad
unsigned long keyTime_p  = 0; // abgleich der verstrichenen Zeit
unsigned long buzTime = 0;// Zeit in Millisekunden fuer den Buzzer
unsigned long buzTime_p  = 0; // abgleich der verstrichenen Zeit
bool wrongInput = false; // Variable fuer falsche Eingabe
unsigned long buzTimeWrongInput = 0; // Zeit in Millisekunden fuer den Buzzer bei falscher Eingabe
unsigned long buzTimeWrongInput_p = 0; // abgleich der verstrichenen Zeit
bool unlocked = false; // Variable, falls der Safe entsperrt wurde
bool locked = false;  // Variable, falls der Safe gesperrt wurde
int afkTime = 20000; // Zeit in Millisec., nach der Energiesparmodus beginnt
bool tryAgain = false; // sobald die Eingabe von Anfang beginnen soll
int degree = 0; // Servomotor-Riegel Winkelstellung in Grad 

// LED Pin Belegung
unsigned long openingLED = 0;
unsigned long openingLED_p = 0;
unsigned long wrongPinLED = 0;
unsigned long wrongPinLED_p = 0;
unsigned long closingLED = 0;
unsigned long closingLED_p = 0;

// PINs definieren
const int ServoPin = 12;
const int BUZ = 11;
const int Disp = 5; 
const int Door = 2; // Interrupt fähiger PIN
const int keyPadInt = 3; // Interrupt fähiger PIN
const int greenLED = 8;
const int redLED = 9;
const int yellowLED = 10;

// I2C Keypad
I2CKeyPad keypad(KeyPad_I2C_ADDR);
char keyMap[19] = "123 456 789  0# NF";

// 7 Segment 4 Digit Display
int8_t pinCode[4];
const int Disp_CLK = 6;
const int Disp_DIO = 7;
TM1637 tm1637(Disp_CLK, Disp_DIO);

// Servo Motor (Türriegel)
Servo Servomotor;

void wakeUp() 
{
  // Just a handler for the pin interrupt.  
}

// Speichert das Passwort in den EEPROM Speicher
void setPassword(int8_t* secret) {
  Password password;
  password.set = true;
  for (int i = 0; i < 4; i++) {
    password.secret[i] = secret[i];  
  }

  EEPROM.put(0, password);
}

// Holt sich die Info aus dem Speicher, ob dort ein Passwort gespeichert ist.
bool getPassword(int8_t* tempPassword) {
  Password myPassword;
  EEPROM.get(0, myPassword); 
  
  if (!myPassword.set) {
    return false;
  }
  
  for (int i = 0; i < 4; i++) {
    tempPassword[i] = myPassword.secret[i];
  }

  return true;
}

// Speicher löschen bzw. überschreibt jede Stelle mit Nullen
void clearStorage() {
  for (int i = 0 ; i < EEPROM.length() ; i++) {
    EEPROM.write(i, 0);
  }  
}

// Vergleiche das gespeicherte Passwort mit der Eingabe und gebe ein true/false zurück
bool inputMatchesPassword(int8_t* input, int8_t* password) {
  for (int i = 0; i < 4; i++) {
    if (input[i] != password[i]) {
      return false; 
    }  
  }  

  return true;
}

void applyPowerSaving() {
  digitalWrite(Disp, LOW); // Display ausschalten (Energiesparmaßnahme)
  
  // Enter power down state with ADC and BOD module disabled.
  // Wake up when wake up pin is low.
  doorOpen = false;
  LowPower.powerDown(SLEEP_FOREVER, ADC_OFF, BOD_OFF);
   
  // der Interrupt führt den Code hier weiter aus..
  turnedOn = true; // um das Aufwachen zu registrieren
}

// Nachdem der Arduino den Energiesparmodus verlässt
void setRunningParameter() {
  onTime_p = onTime;
  turnedOn = false;
  detachInterrupt(digitalPinToInterrupt(Door)); // Vorher gesetzten Interrupt löschen
  detachInterrupt(digitalPinToInterrupt(keyPadInt)); // Vorher gesetzten Interrupt löschen
  memset(pinCode, 0, sizeof(pinCode)); // um das Display mit Nullen füllen -> erneute Eingabe
  digitalWrite(Disp, HIGH);
  doorOpen = true;
  sequenceNum = 0;
}

void setup() {   
  // PIN-Modi definieren                          
  pinMode(BUZ, OUTPUT);
  pinMode(Door, INPUT);
  pinMode(Disp, OUTPUT);
  pinMode(keyPadInt, INPUT);
  pinMode(greenLED, OUTPUT);
  pinMode(redLED, OUTPUT);
  pinMode(yellowLED, OUTPUT);

  // Servomotor
  Servomotor.attach(ServoPin);
  
  // Keypad
  keypad.loadKeyMap(keyMap);
  
  // Display
  tm1637.init();
  tm1637.set(BRIGHT_TYPICAL);//BRIGHT_TYPICAL = 2,BRIGHT_DARKEST = 0,BRIGHTEST = 7;

  Serial.begin(9600);
  keypad.begin();
  Wire.begin();
  delay(500);
   //clearStorage(); // Falls der Safe gesperrt wurde und der Pin vergessen wird

  // Rückstellung bzw. Kallibrierung des Servomotor-Stellung
  if (!getPassword(passwordTemp)) {
    Servomotor.write(degree);
  }

  delay(500);
}

void loop() {
  onTime = millis();
  buzTime = millis();
  openingLED = millis();
  wrongPinLED = millis();
  closingLED = millis();
  buzTimeWrongInput = millis();
  
  // Bei falscher Pin Eingabe
  if (wrongInput) {
    if (buzTimeWrongInput - buzTimeWrongInput_p >= 1050) {
      digitalWrite(BUZ, LOW);
      digitalWrite(redLED, LOW);
      buzTimeWrongInput_p = buzTimeWrongInput;
      wrongInput = false;
    }
  }

  // Nach Tastendruck soll der Buzzer nach 35 Millisekunden aus gehen
  if (!wrongInput && keyPressed && (buzTime - buzTime_p >= 35)) {
    buzTime_p = buzTime;
    digitalWrite(BUZ, LOW);
    keyPressed = false;
  }

  // Falls der Safe entsperrt wurde
  if (unlocked && (openingLED - openingLED_p >= 300)) {
    memset(pinCode, 0, sizeof(pinCode)); // um das Display mit Nullen füllen
    if (openingLED - openingLED_p >= 700) {
      unlocked = false;
      clearStorage(); // ..braucht Zeit!
      digitalWrite(greenLED, LOW);
    }
  }

  if (locked && closingLED - closingLED_p >= 700) {
    if (closingLED - closingLED_p >= 900) {
      digitalWrite(greenLED, LOW);
    }

    if (closingLED - closingLED_p >= 1100) {
      digitalWrite(yellowLED, LOW);
    }

    if (closingLED - closingLED_p >= 1300) {
      digitalWrite(redLED, LOW);
      locked = false;
    }
  }

  // Zuruecksetzen der Zahlen, für erneute Eingabe bei vorhandenem Passwort
  if (tryAgain && !wrongInput && !unlocked && !locked) {
    sequenceNum = 0;
    tryAgain = false;  
    
    memset(pinCode, 0, sizeof(pinCode)); // um das Display mit Nullen füllen
  }

  
  int doorClosed = digitalRead(Door);
  if (doorClosed) {
    tm1637.display(pinCode); // Zahl auf dem Bildschirm ausgeben

    // Nach der 'away-from-keyboard' Zeit, soll in den Energiesparmodus gewechselt werden
    if (onTime - onTime_p >= afkTime) {
      attachInterrupt(digitalPinToInterrupt(keyPadInt), wakeUp, CHANGE); // Interrupt definieren
      applyPowerSaving();
    }
    
    // Wenn der Arduino direkt zuvor im Energiesparmodus oder ausgeschaltet war
    if (turnedOn) {
      setRunningParameter();
    }

    // Hier wird die Eingabe registriert und bearbeitet
    keyTime = millis();
    char keyAsChar = keypad.getChar();
    if ((keyTime - keyTime_p >= 650) && keyAsChar != ' ' && keyAsChar != 'F') { // Wenn die deaktivierten Tasten gedrückt werden oder zwei Tasten gleichzeitig.  
      if (keyAsChar != 'N') {
       
        onTime_p = onTime; // resetten der Zeit, bis der Energiesparmodus eintritt 
        keyTime_p = keyTime; // resetten der Zeit, bis ein Tastendruck möglich ist
        
        digitalWrite(BUZ, HIGH);

        if (keyAsChar != '#') {
          char keyAsAscii = keyAsChar - '0'; //Um bei der Umwandlung von char zu int die korrekten Zahlen zu erhalten
          
          if (sequenceNum < 4) {
            pinCode[sequenceNum] = (int8_t)keyAsAscii;
            sequenceNum++;
            tm1637.display(pinCode); // Zahl auf dem Bildschirm ausgeben
          }
          
          if (sequenceNum == 4) {
            if (!getPassword(passwordTemp)) {
              Servomotor.write(degree);
              
            } else {
              if (inputMatchesPassword(pinCode, passwordTemp)) {
                Serial.println("richtiges Passwort!");
                digitalWrite(greenLED, HIGH);
                openingLED_p = openingLED;
                Servomotor.write(degree);
                unlocked = true;
                
                
              } else {
                Serial.println("FALSCH!!");
                digitalWrite(redLED, HIGH);
                digitalWrite(BUZ, HIGH);
                wrongPinLED_p = wrongPinLED;
                buzTimeWrongInput_p = buzTimeWrongInput;
                wrongInput = true;
              }

              tryAgain = true;
            } 
          }
        } else if (keyAsChar == '#' && sequenceNum == 4 && !getPassword(passwordTemp)) {
          digitalWrite(yellowLED, HIGH);
          digitalWrite(redLED, HIGH);
          digitalWrite(greenLED, HIGH);
          closingLED_p = closingLED;
          Servomotor.write(degree + 92); // Schliessen
          setPassword(pinCode);
          // attachInterrupt(digitalPinToInterrupt(keyPadInt), wakeUp, CHANGE); // Interrupt definieren
          tryAgain = true;
          locked = true;
        }
        
        keyPressed = true;
      }
    }
  } else if (doorOpen) {
    delay(1000);
    attachInterrupt(digitalPinToInterrupt(Door), wakeUp, CHANGE); // Interrupt definieren
    applyPowerSaving();
  } 
}
