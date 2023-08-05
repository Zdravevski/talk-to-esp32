// The script is written by Slavko Zdravevski
// If you want to support my work, you can subscribe to my youtube channel: https://bit.ly/3FG9hpK
// I do a lot of interesting things in my free time, so you might find something of your interest or we can exchange ideas and knowledge

#include "BluetoothSerial.h"

String receivedCommand;
int pwmValue;
unsigned long currentSecounds, previousSecoundsRecord;
const int ledPin = 32; 
const int freq = 5000;
const int ledChannel = 0;
const int resolution = 8;

BluetoothSerial SerialBT;

void setup() {
  Serial.begin(115200);
  SerialBT.begin("ESP32"); // If you want to use my app, leave this line as it is, otherwise, it won't work

  ledcSetup(ledChannel, freq, resolution);
  ledcAttachPin(ledPin, ledChannel);
}

void loop() {
  confirmGoodConnection();
  decodeReceivedValues();
  ledcWrite(ledChannel, pwmValue);
}

void decodeReceivedValues() {
  if (SerialBT.available()) {
    receivedCommand += String((char)SerialBT.read());
  } else {
    receivedCommand.toLowerCase();
    if (receivedCommand != "") {
      int receivedCommandValue = receivedCommand.toInt();
      if (receivedCommandValue >= 10 && receivedCommandValue <= 99) { // I'm sending these values through my app, let them like they are, if you want to use my app
        pwmValue = map(receivedCommandValue, 10, 99, 0, 255); // Mapping the values received from the phone's app, into values required for the PWM signal
      }
      receivedCommand = "";
    }
  }
}

void confirmGoodConnection() {
  currentSecounds = millis() / 1000;

  if ((currentSecounds - previousSecoundsRecord) >= 4) {
    previousSecoundsRecord = currentSecounds;
    SerialBT.println("Good connection");
  }
}
