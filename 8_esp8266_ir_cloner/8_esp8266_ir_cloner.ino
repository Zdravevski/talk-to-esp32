// The script is written by Slavko Zdravevski
// If you want to support my work, you can subscribe to my youtube channel: https://bit.ly/3FG9hpK
// I do a lot of interesting things in my free time, so you might find something of your interest or we can exchange ideas and knowledge

// Libraries
#include <Arduino.h>
#include "PinDefinitionsAndMore.h" // defines macros for input and output pin etc.

#define MARK_EXCESS_MICROS 20    // Adapt it to your IR receiver module. 20 is recommended for the cheap VS1838 modules.

#if !defined(RAW_BUFFER_LENGTH)
#  if RAMEND <= 0x4FF || RAMSIZE < 0x4FF
#define RAW_BUFFER_LENGTH  120
#  elif RAMEND <= 0xAFF || RAMSIZE < 0xAFF // 0xAFF for LEONARDO
#define RAW_BUFFER_LENGTH  400 // 600 is too much here, because we have additional uint8_t rawCode[RAW_BUFFER_LENGTH];
#  else
#define RAW_BUFFER_LENGTH  750
#  endif
#endif

#include <IRremote.hpp>
#include <SoftwareSerial.h>

// Pinout declaration
int ledPin = D0;

// Additional variables needed through the code
struct storedIRDataStruct {
  uint8_t rawCode[RAW_BUFFER_LENGTH];
  uint8_t rawCodeLength;
} storedIRData;
  
bool triggerIrEmittingProcedure = false;
int irEmittingCnt = 0;
unsigned long currentEmittingMillis, lastEmittingMillis;
bool resumeIrDecodingProcess = true;

unsigned long currentSecounds, previousSecoundsRecord;
String receivedCommand;
String commandToSend;

unsigned long previousMillisLed = 0;
bool blinkLed = false;
int ledCounter = 0;

void setup() {
  pinMode(ledPin, OUTPUT); digitalWrite(ledPin, LOW);

  Serial.begin(9600);

  IrReceiver.begin(IR_RECEIVE_PIN, ENABLE_LED_FEEDBACK);
  IrSender.begin();
}

void loop() {
  sendGoodConnectionFlag();
  receiveBtData();
  checkIrEmittingProcedure();
  checkIrDecodingProcess();
  checkLedState();
}

// Method declarations
void storeIrCode() { // stores the IR raw data into local variables
  storedIRData.rawCodeLength = IrReceiver.decodedIRData.rawDataPtr->rawlen - 1;
  IrReceiver.compensateAndStoreIRResultInArray(storedIRData.rawCode);
  // IrReceiver.printIRResultRawFormatted(&Serial, true); // Output the results in RAW format
}

void sendIrCode() { // sends the raw data, through the IR transmitter, modulated at a frequency of 38Khz
  IrSender.sendRaw(storedIRData.rawCode, storedIRData.rawCodeLength, 38);
}

void sendGoodConnectionFlag() { // sends a good connection flag, so the app knows that it has a connection with the board
  currentSecounds = millis() / 1000;

  if ((unsigned long)(currentSecounds - previousSecoundsRecord) >= 4) {
    previousSecoundsRecord = currentSecounds;
    Serial.println("Good connection");
  }
}

void receiveBtData() { // determines which operation needs to be executed
  String currentIrTiming = "";
  int incomingCodeCnt = 0;

  if (Serial.available()) {
    String incomingString = Serial.readString();
    if (incomingString == "refresh data") {
      sendCurrentIRData();
    } else {
      for (int i=0; i<incomingString.length(); i++) {
        if ((incomingString[i] != ',') && (incomingString[i] != '|')) {
          currentIrTiming += incomingString[i];
        } else {
          storedIRData.rawCode[incomingCodeCnt] = currentIrTiming.toInt();
          currentIrTiming = "";
          incomingCodeCnt++;
        }
      }

      storedIRData.rawCodeLength = incomingCodeCnt;
      triggerIrEmittingProcedure = true;
      resumeIrDecodingProcess = false;
      irEmittingCnt = 0;
      blinkLed = true;
    }
  }
}

void sendCurrentIRData() { // sends IR data, chucked into groups, because sometimes the Bluetooth communication fails if we try to send big commands
  String currentBtData;
  int currentDataCnt = 0;
  int currentRawCodeLength = storedIRData.rawCodeLength;
  
  if (currentRawCodeLength == 0) {
    Serial.println("empty memory");
  } else {
    for (int i=0; i<storedIRData.rawCodeLength; i++) {
      currentDataCnt++;
      
      if (i == storedIRData.rawCodeLength-1) {
        currentBtData += ((String((int)storedIRData.rawCode[i]))+String('|'));
        Serial.println(currentBtData);
      } else if (currentDataCnt <= 5) {
        currentBtData += ((String((int)storedIRData.rawCode[i]))+String(','));
      } else {
        currentBtData += ((String((int)storedIRData.rawCode[i]))+String(','));
        Serial.println(currentBtData);
        currentBtData = "";
        currentDataCnt = 0;
        delay(100);
      }
    }
  }
}

void checkIrEmittingProcedure() {
  if (triggerIrEmittingProcedure) {
    currentEmittingMillis = millis();
    if ((unsigned long)(currentEmittingMillis - lastEmittingMillis) > 1000) {
      if (irEmittingCnt == 0) {
        IrReceiver.stop();
        Serial.flush();
        sendIrCode();
      } else if (irEmittingCnt == 1){
        IrReceiver.start();
      } else if (irEmittingCnt == 2) {
          triggerIrEmittingProcedure = false;
          resumeIrDecodingProcess = true;
      }

      lastEmittingMillis = currentEmittingMillis;
      irEmittingCnt++;
    }
  }
}

void checkIrDecodingProcess () {
  if (resumeIrDecodingProcess) {
    if (IrReceiver.decode()) {
      blinkLed = true;
      storeIrCode();
      IrReceiver.resume();
    }
  }
}

void checkLedState() { // Checks whether the led should blink
  unsigned long currentMillisLed = millis();  
  if (blinkLed) {
    if ((unsigned long)(currentMillisLed - previousMillisLed) >= 100) {
      if (ledCounter%2 == 0) {
        digitalWrite(ledPin, LOW);
      } else {
        digitalWrite(ledPin, HIGH);
      }
      
      if (ledCounter >= 4) {
        ledCounter = 0;
        blinkLed = false;
      }

      ledCounter++;
      previousMillisLed = currentMillisLed;
    }
  }
}




