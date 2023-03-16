#include <EEPROM.h>
void setup() {
  Serial.begin(9600);
  EEPROM.begin(28);

  // for (int i = 0 ; i < 7 ; i++) {
  //   EEPROM.write(i, 5);
  //   EEPROM.commit();
  // }

  for (int i = 0 ; i < 28 ; i++) {
    Serial.println(EEPROM.read(i));
  }

}

void loop() {
  // put your main code here, to run repeatedly:

}
