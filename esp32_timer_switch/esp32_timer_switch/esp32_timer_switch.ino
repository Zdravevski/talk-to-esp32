// The script is written by Slavko Zdravevski
// If you want to support my work, you can subscribe to my youtube channel: https://bit.ly/3Qyv8pp
// I do a lot of interesting things in my free time, so you might find something of your interest or we can exchange ideas and knowledge

// Libraries
#include "BluetoothSerial.h"
#include <ArduinoJson.h>
#include <ThreeWire.h>  
#include <RtcDS1302.h>
#include <EEPROM.h>

// Pinout declaration
int relayPin = 23;
int optocouplerRtcPin = 12;
int batteryIndicationLedPin = 14;

// Additional variables needed through the code
String receivedCommand;
unsigned long currentSeconds, previousSecondsRecord;
String jsonBlInput = "";

int t1Hours[7];
int t1Minutes[7];
int t1Seconds[7];
int t2Hours[7];
int t2Minutes[7];
int t2Seconds[7];

bool canDeviceWork = true;

BluetoothSerial SerialBT;
DynamicJsonDocument doc(2048); 
ThreeWire myWire(4,5,2);
RtcDS1302<ThreeWire> Rtc(myWire);
RtcDateTime now;

// The main program starts here
void setup() {
  EEPROM.begin(28);
  Serial.begin(9600);
  initializeRtcModule();

  if (canDeviceWork) {
    SerialBT.begin("ESP32"); // If you want to use my app, leave this line as it is, otherwise, it won't work
  }

  pinInitialization();
  readDataFromEeprom();
}

void loop() {
  if (canDeviceWork) {
    // We can connect the ESP32 VCC to the RTC module through the optocoupler, which we had previously disabled, just to check the battery
    digitalWrite(optocouplerRtcPin, HIGH);
    digitalWrite(batteryIndicationLedPin, LOW);

    checkBluetoothSerial();
    pingBluetoothDevice();
    executeTimeConditions();
  } else {
    digitalWrite(batteryIndicationLedPin, HIGH);
    digitalWrite(relayPin, LOW); // The relay is turned on
  }
}

// Method declarations
void executeTimeConditions() { // Checks and decides whether we have to turn on or off the relay
  RtcDateTime now = Rtc.GetDateTime();
  int currentHour = (now.Hour());
  int currentMinute = (now.Minute());
  // I don't use seconds in my case, if you need a more precise solution, you can add them too
  //  int currentSecond = (now.Second());
  int currentWeekday = weekday((now.Year()), (now.Month()), (now.Day()));

  if (currentWeekday == 0) { // I need this change because the JSON data are ordered differently, there 0 means Monday
    currentWeekday = 6;
  } else {
    currentWeekday -= 1;
  }

  int t1HourStored = t1Hours[currentWeekday];
  int t1MinuteStored = t1Minutes[currentWeekday];
  int t2HourStored = t2Hours[currentWeekday];
  int t2MinuteStored = t2Minutes[currentWeekday];

  String currentComparationTime = String(currentHour) + String(currentMinute);
  String storedComparationT1Time = String(t1HourStored) + String(t1MinuteStored);
  String storedComparationT2Time = String(t2HourStored) + String(t2MinuteStored);

  if (currentComparationTime.toInt() >= storedComparationT1Time.toInt() &&
      currentComparationTime.toInt() < storedComparationT2Time.toInt()) {
      digitalWrite(relayPin, LOW);
    } else {
      digitalWrite(relayPin, HIGH);
    }
}

void pinInitialization() { // Initializing the pinouts
  pinMode(relayPin, OUTPUT); digitalWrite(relayPin, HIGH); // When I send LOW to my relay it is active, change it if yours doesn't work this way
  pinMode(optocouplerRtcPin, OUTPUT); digitalWrite(optocouplerRtcPin, LOW);
  pinMode(batteryIndicationLedPin, OUTPUT); digitalWrite(batteryIndicationLedPin, LOW);
}

void pingBluetoothDevice() { // Info needed for the app, to confirm that the device is connected to the app
  currentSeconds = millis() / 1000;
  if ((currentSeconds - previousSecondsRecord) >= 4) {
    previousSecondsRecord = currentSeconds;
    SerialBT.println("Good connection");
  }
}

void checkBluetoothSerial() { // Checks whether we have Bluetooth input, and saves the data in a parameter
  if (SerialBT.available()) {
    receivedCommand += String((char)SerialBT.read());
  } else {
    receivedCommand.toLowerCase();
    if (receivedCommand != "") {
      deserializeJsonInput(receivedCommand);
      receivedCommand = "";
    }
  }
}

void deserializeJsonInput(String json) { // Deserializes the JSON data sent through Bluetooth by the smartphone, and stores the values in local variables
  jsonBlInput = json;
  deserializeJson(doc, json);
  Serial.println(jsonBlInput);

  for (int i=0; i<7; i++) {
    t1Hours[i] = (int)doc["t1"][i]["h"];
    t1Minutes[i] = (int)doc["t1"][i]["m"];
    t1Seconds[i] = (int)doc["t1"][i]["s"];

    t2Hours[i] = (int)doc["t2"][i]["h"];
    t2Minutes[i] = (int)doc["t2"][i]["m"];
    t2Seconds[i] = (int)doc["t2"][i]["s"];
  }

  int localTimeYear = (int)doc["lt"]["y"];
  int localTimeMonth = (int)doc["lt"]["mo"];
  int localTimeDay = (int)doc["lt"]["d"];
  int localTimeHour = (int)doc["lt"]["h"];
  int localTimeMinute = (int)doc["lt"]["m"];
  int localTimeSeconds = (int)doc["lt"]["s"];

  // Year, Month, Day, Hour, Minute, Seconds
  Rtc.SetDateTime(RtcDateTime(localTimeYear, 
                              localTimeMonth, 
                              localTimeDay, 
                              localTimeHour, 
                              localTimeMinute, 
                              localTimeSeconds));
  saveDataToEeprom();
}

void saveDataToEeprom() { // Stores the hours and minutes sent through Bluetooth from the phone in the EEPROM memory
  int localCounter = 0;
  for (int i=0; i<28; i++) { 
    if (i < 7) {
      EEPROM.write(i, t1Hours[localCounter]);
      EEPROM.commit();
    } else if (i >= 7 && i < 14) {
      EEPROM.write(i, t1Minutes[localCounter]);
      EEPROM.commit();
    } else if (i >= 14 && i < 21) {
      EEPROM.write(i, t2Hours[localCounter]);
      EEPROM.commit();
    } else if (i >= 21 && i < 28) {
      EEPROM.write(i, t2Minutes[localCounter]);
      EEPROM.commit();
    }
    // I don't store the seconds in the EEPROM memory, if you need a more precise solution, you can add them too

    localCounter++;
    if (localCounter > 6) {
      localCounter = 0;
    }
  }
  Serial.println("The data has been stored in the EEPROM memory of the controller");
}

void readDataFromEeprom() { // Reads the EEPROM data, and adds the data to the local variables
  int localCounter = 0;
  for (int i=0; i<28; i++) {
    if (i < 7) {
      t1Hours[localCounter] = EEPROM.read(i);
    } else if (i >= 7 && i < 14) {
      t1Minutes[localCounter] = EEPROM.read(i);
    } else if (i >= 14 && i < 21) {
      t2Hours[localCounter] = EEPROM.read(i);
    } else if (i >= 21 && i < 28) {
      t2Minutes[localCounter] = EEPROM.read(i);
    }

    // I don't store the seconds in the EEPROM memory, if you need a more precise solution, you can add them too
    
    localCounter++;
    if (localCounter > 6) {
      localCounter = 0;
    }
  }
}

void initializeRtcModule() { // Initializes the RTC module
  Rtc.Begin();
  RtcDateTime compiled = RtcDateTime(__DATE__, __TIME__);
  
  if (!Rtc.IsDateTimeValid()) {
      // Common Causes:
      //    1) first time you ran and the device wasn't running yet
      //    2) the battery on the device is low or even missing
      Serial.println("RTC lost confidence in the DateTime!");
      canDeviceWork = false;
      Rtc.SetDateTime(compiled);
  }
  
  if (Rtc.GetIsWriteProtected()) {
      Serial.println("RTC was write protected, enabling writing now");
      Rtc.SetIsWriteProtected(false);
  }
  
  if (!Rtc.GetIsRunning()) {
      Serial.println("RTC was not actively running, starting now");
      Rtc.SetIsRunning(true);
  }
  
  now = Rtc.GetDateTime();
  if (now < compiled) {
      Serial.println("RTC is older than compile time!  (Updating DateTime)");
      Rtc.SetDateTime(compiled);
  }
  else if (now > compiled) {
      Serial.println("RTC is newer than compile time. (this is expected)");
  }
  else if (now == compiled) {
      Serial.println("RTC is the same as compile time! (not expected but all is fine)");
  }
}

int weekday(int year, int month, int day) { // Calculates day of week in proleptic Gregorian calendar. Sunday == 0.
  int adjustment, mm, yy;
  if (year<2000) year+=2000;
  adjustment = (14 - month) / 12;
  mm = month + 12 * adjustment - 2;
  yy = year - adjustment;
  return (day + (13 * mm - 1) / 5 +
    yy + yy / 4 - yy / 100 + yy / 400) % 7;
}
