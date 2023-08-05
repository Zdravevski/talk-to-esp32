// The script is written by Slavko Zdravevski
// If you want to support my work, you can subscribe to my youtube channel: https://bit.ly/3FG9hpK
// I do a lot of interesting things in my free time, so you might find something of your interest or we can exchange ideas and knowledge

// Libraries, I also put them on github
#include "BluetoothSerial.h"
#include <RCSwitch.h>

// Pinout declaration
int ledPin = 13;
int rfReceiverPin = 2;
int rfTransmitterPin = 4; 

// Additional variables needed through the code
unsigned long currentSecounds, previousSecoundsRecord;
String receivedCommand;
String commandToSend;

unsigned long receivedCodes[100];
int receivedProtocols[100];
int receiverCounter = 0;
bool canStoreCurrentCode = true;
unsigned long previousMillisLed = 0;
bool blinkLed = false;
int ledCounter = 0;

BluetoothSerial SerialBT;
RCSwitch mySwitch = RCSwitch();

void setup() {
  Serial.begin(115200);
  SerialBT.begin("ESP32"); // If you want to use my app, leave this line as it is, otherwise, it won't work
  mySwitch.enableReceive(rfReceiverPin);
  mySwitch.enableTransmit(rfTransmitterPin);
  pinMode(ledPin, OUTPUT);
}

void loop() {
  currentSecounds = millis() / 1000;
  receiveBtData();
  decodeRfSignals();
  checkLedState();
  
  if ((currentSecounds - previousSecoundsRecord) >= 4) {
    previousSecoundsRecord = currentSecounds;
    SerialBT.println("Good connection");
  }
}

// Method declarations
void receiveBtData() { // Receives Bluetooth data sent from the Android device and acts accordingly
  if (SerialBT.available()) {
    receivedCommand += String((char)SerialBT.read());
  } else {
    receivedCommand.toLowerCase();
    if (receivedCommand != "") {
      Serial.println(receivedCommand);
      if (receivedCommand == "refresh data") {
        sendCurrentRfData();
      } else {
        int codeForSend = receivedCommand.substring(0,receivedCommand.indexOf("|")).toInt();
        int protocolForSend = receivedCommand.substring(receivedCommand.indexOf("|")+1, receivedCommand.length()).toInt();
        Serial.println(codeForSend);
        Serial.println(protocolForSend);
        sendCodeOverRfModule(codeForSend, protocolForSend);
      }
      receivedCommand = "";
    }
  }
}

void sendCurrentRfData() { // Sends the currently stored data in the microcontroller's memory to the phone over Bluetooth
  if (receiverCounter != 0) {
    for (int i=0; i<receiverCounter; i++) {
      commandToSend = String(receivedCodes[i])+"|"+String(receivedProtocols[i]);
      SerialBT.println(commandToSend);
      delay(200);
    }
  } else {
    SerialBT.println("empty memory");
  }
}

void sendCodeOverRfModule(int code, int protocol) { // Sends the code got from the parameter over the radio module
  blinkLed = true;
  // Optional set protocol (default is 1, will work for most outlets)
  mySwitch.setProtocol(protocol);
  mySwitch.send(code, 24);
}

void decodeRfSignals() { // Decodes the 433/315 signals received by the module, and stores them into local arrays
  if (mySwitch.available()) {
    blinkLed = true;
    for (int i=0; i<receiverCounter; i++) {
      if (receivedCodes[i] == mySwitch.getReceivedValue()) {
        canStoreCurrentCode = false;
        break;
      }
    }

    if (canStoreCurrentCode) {
      receivedCodes[receiverCounter] = mySwitch.getReceivedValue();
      receivedProtocols[receiverCounter] = mySwitch.getReceivedProtocol();

      Serial.println(mySwitch.getReceivedValue());
      Serial.println(receiverCounter);
      receiverCounter++;
    }

    canStoreCurrentCode = true;

    mySwitch.resetAvailable();
  }
}

void checkLedState() {
  unsigned long currentMillisLed = millis();  
  if (blinkLed) {
    if ((unsigned long)(currentMillisLed - previousMillisLed) >= 100) {
      if (ledCounter == 0) {
        digitalWrite(ledPin, HIGH);
        ledCounter = 1;
      } else if (ledCounter == 1) {
        digitalWrite(ledPin, LOW);
        ledCounter = 2;
      } else if (ledCounter == 2) {
        digitalWrite(ledPin, HIGH);
        ledCounter = 3;
      } else if (ledCounter == 3) {
        digitalWrite(ledPin, LOW);
        ledCounter = 0;
        blinkLed = false;
      }
      previousMillisLed = currentMillisLed;
    }
  }
}
