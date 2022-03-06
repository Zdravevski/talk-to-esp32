// The script is written by Slavko Zdravevski
// If you want to support my work, you can subscribe to my youtube channel: https://bit.ly/3FG9hpK
// I do a lot of interesting things in my free time, so you might find something of your interest or we can exchange ideas and knowledge

#include "BluetoothSerial.h"
#include "max6675.h"

int thermoDO = 12;
int thermoCS = 14;
int thermoCLK = 27;

String receivedCommand;
unsigned long currentSecounds, previousSecoundsRecord;
float temperature;

BluetoothSerial SerialBT;
MAX6675 thermocouple(thermoCLK, thermoCS, thermoDO);

void setup() {
  Serial.begin(115200);
  SerialBT.begin("ESP32"); // If you want to use my app, leave this line as it is, otherwise, it won't work
}

void loop() {
  currentSecounds = millis() / 1000;

  temperature = thermocouple.readCelsius(); // You can use readFahrenheit() also
  Serial.println(temperature);
  if (temperature > 100) {
    SerialBT.println("The temperature is too high, check the system");
    delay(5000);
  }
  
  if ((currentSecounds - previousSecoundsRecord) >= 4) {
    previousSecoundsRecord = currentSecounds;
    SerialBT.println("Good connection");
  }

  // For the MAX6675 to update, you must delay AT LEAST 250ms between reads!
  delay(250);
}
