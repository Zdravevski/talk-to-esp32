// The script is written by Slavko Zdravevski
// If you want to support my work, you can subscribe to my youtube channel: https://bit.ly/3FG9hpK
// I do a lot of interesting things in my free time, so you might find something of your interest or we can exchange ideas and knowledge

#include "BluetoothSerial.h"

String receivedCommand;
unsigned long currentSecounds, previousSecoundsRecord;
int redLed = 0;
int yellowLed = 4;
int blueLed = 16;

BluetoothSerial SerialBT;

void setup() {
  Serial.begin(115200);
  SerialBT.begin("ESP32"); // If you want to use my app, leave this line as it is, otherwise, it won't work

  pinMode(redLed, OUTPUT);
  pinMode(yellowLed, OUTPUT);
  pinMode(blueLed, OUTPUT);
}

void loop() {
  currentSecounds = millis() / 1000;
  
  if (SerialBT.available()) {
    receivedCommand += String((char)SerialBT.read());
  } else {
    receivedCommand.toLowerCase();
    if (receivedCommand != "") {
      Serial.println(receivedCommand);
      ledColor(receivedCommand);
      receivedCommand = "";
    }
  }

  if ((currentSecounds - previousSecoundsRecord) >= 4) {
    previousSecoundsRecord = currentSecounds;
    SerialBT.println("Good connection");
  }
}

void ledColor(String color) {
  if (color == "red") {
    digitalWrite(redLed, HIGH);
    digitalWrite(yellowLed, LOW);
    digitalWrite(blueLed, LOW);
  } else if (color == "yellow") {
    digitalWrite(redLed, LOW);
    digitalWrite(yellowLed, HIGH);
    digitalWrite(blueLed, LOW);
  } else if (color == "blue") {
    digitalWrite(redLed, LOW);
    digitalWrite(yellowLed, LOW);
    digitalWrite(blueLed, HIGH);
  } else if (color == "turn on") {
    digitalWrite(redLed, HIGH);
    digitalWrite(yellowLed, HIGH);
    digitalWrite(blueLed, HIGH);
  } else {
    digitalWrite(redLed, LOW);
    digitalWrite(yellowLed, LOW);
    digitalWrite(blueLed, LOW);
  }
}
