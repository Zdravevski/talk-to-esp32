// The script is written by Slavko Zdravevski
// If you want to support my work, you can subscribe to my youtube channel: https://bit.ly/3FG9hpK
// I do a lot of interesting things in my free time, so you might find something of your interest or we can exchange ideas and knowledge

// Libraries, I also put them on github
#include <SoftwareSerial.h>

// Pinout declaration
int ledPin = D5;
int rfCoilPin = D1;

// Additional variables needed through the code
const int rfidBufferSize = 14; // RFID DATA FRAME FORMAT: 1byte head (value: 2), 10byte data (2byte version + 8byte tag), 2byte checksum, 1byte tail (value: 3)
const int rfidDataSize = 10; // 10byte data (2byte version + 8byte tag)
const int rfidDataVersionSize = 2; // 2byte version (actual meaning of these two bytes may vary)
const int rfidDataTagSize = 8; // 8byte tag
const int rfidChecksumSize = 2; // 2byte checksum

unsigned long receivedRfidCodes[100]; // I'm planning to store 100 rfid codes in the microcontroller, you can change the maximum number
int receiverRfidCounter = 0;
bool canStoreRfidCurrentCode = true;
uint8_t rfidBuffer[rfidBufferSize]; // used to store an rfid incoming data frame 
int rfidBufferIndex = 0;

unsigned long currentSecounds, previousSecoundsRecord;
String receivedCommand;
String commandToSend;

unsigned long previousMillisLed = 0;
bool blinkLed = false;
int ledCounter = 0;

static uint8_t data[64];
static uint8_t value[10];
static uint8_t activeId = 0;
uint8_t  vendorActive = 0;
uint32_t idActive = 13264846; // 0013264846, it's just for the start, it will be overwritten through the app
int manchesterCounter = 0;

static inline void asm_nop() {
    asm volatile ("nop"); 
}

// Objects
SoftwareSerial ssrfid = SoftwareSerial(D6,D8); 

void setup() {
 Serial.begin(9600);
 ssrfid.begin(9600);
 ssrfid.listen(); 

 pinMode(ledPin, OUTPUT);
 pinMode(rfCoilPin, INPUT);
}

void loop() {
  sendGoodConnectionFlag();
  receiveBtData();
  checkAndDecodeTag();
  checkLedState();
  manchesterEncoder();
}

void checkAndDecodeTag() { // checks whether a tag is present near the coil and decodes it
  if (ssrfid.available() > 0) {
    bool callExtractTag = false;
    
    int ssvalue = ssrfid.read(); // read 
    if (ssvalue == -1) { // no data was read
      return;
    }

    if (ssvalue == 2) { // RDM630/RDM6300 found a tag => tag incoming 
      rfidBufferIndex = 0;
    } else if (ssvalue == 3) { // tag has been fully transmitted       
      callExtractTag = true; // extract tag at the end of the function call
    }

    if (rfidBufferIndex >= rfidBufferSize) { // checking for a rfidBuffer overflow (It's very unlikely that an rfidBuffer overflow comes up!)
      Serial.println("Error: rfidBuffer overflow detected!");
      return;
    }
    
    rfidBuffer[rfidBufferIndex++] = ssvalue; // everything is alright => copy current value to rfidBuffer

    if (callExtractTag == true) {
      if (rfidBufferIndex == rfidBufferSize) {
        unsigned long tag = (unsigned long )extractTag();
        checkTagToStore(tag);
      } else { // something is wrong... start again looking for preamble (value: 2)
        rfidBufferIndex = 0;
        return;
      }
    }    
  }
}

unsigned extractTag() { // extracts the tag value
    uint8_t *msgData = rfidBuffer + 1; // 10 byte => data contains 2byte version + 8byte tag
    uint8_t *msgDataTag = msgData + 2;
    uint8_t *msgChecksum = rfidBuffer + 11; // 2 byte

    long tag = hexStrToValue(msgDataTag, rfidDataTagSize);

    return tag;
}

long hexStrToValue(unsigned char *str, unsigned int length) { // converts a hexadecimal value (encoded as ASCII string) to a numeric value
  char* copy = (char*)malloc((sizeof(char) * length) + 1); 
  memcpy(copy, str, sizeof(char) * length);
  copy[length] = '\0'; 
  // the variable "copy" is a copy of the parameter "str". "copy" has an additional '\0' element to make sure that "str" is null-terminated.
  long value = strtol(copy, NULL, 16);  // strtol converts a null-terminated string to a long value
  free(copy); // clean up 
  return value;
}

void checkTagToStore(unsigned long tagForCheck) { // checks whether the current tag has been previously received
    blinkLed = true;
    for (int i=0; i<receiverRfidCounter; i++) { // If you are using more tags in the microcontroller's memory, use a better search mechanism
      if (receivedRfidCodes[i] == tagForCheck) {
        canStoreRfidCurrentCode = false;
        break;
      }
    }

    if (canStoreRfidCurrentCode) {
      receivedRfidCodes[receiverRfidCounter] = tagForCheck;
      // Serial.println(receivedRfidCodes[receiverRfidCounter]);
      receiverRfidCounter++;
    }

    canStoreRfidCurrentCode = true;
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

void sendGoodConnectionFlag() { // sends a good connection flag, so the app knows that it has a connection with the board
  currentSecounds = millis() / 1000;

  if ((unsigned long)(currentSecounds - previousSecoundsRecord) >= 4) {
    previousSecoundsRecord = currentSecounds;
    Serial.println("Good connection");
  }
}

void receiveBtData () { // determines which operation needs to be executed
  if (Serial.available()) {
    String incomingString = Serial.readString();
    if (incomingString == "refresh data") {
      sendCurrentRfidData();
    } else {
        activeId = 1;
        uint32_t rfidToPrepare = incomingString.toInt();
        prepareRfidData(rfidToPrepare, vendorActive);
    }
  }
}

void sendCurrentRfidData() { // sends the currently stored data in the microcontroller's memory to the phone over Bluetooth
  if (receiverRfidCounter != 0) {
    for (int i=0; i<receiverRfidCounter; i++) {
      commandToSend = String(receivedRfidCodes[i]);
      Serial.println(commandToSend);
      delay(200);
    }
  } else {
    Serial.println("empty memory");
  }
}

void prepareRfidData(uint32_t ID, uint32_t VENDOR) { // prepares rfid data to be sent according to the protocol
  value[0] = (VENDOR>>4) & 0XF;
  value[1] = VENDOR & 0XF;
  for (int i=1; i<8; i++) {
    value[i+2] = (ID>>(28-i*4)) &0xF;
  }

  for (int i=0; i<9; i++) data[i]=1; //header
  for (int i=0; i<10; i++) {         //data
    for (int j=0; j<4; j++) {
      data[9 + i*5 +j] = value[i] >> (3-j) & 1;
    }
    data[9 + i*5 + 4] = ( data[9 + i*5 + 0]
                        + data[9 + i*5 + 1]
                        + data[9 + i*5 + 2]
                        + data[9 + i*5 + 3]) % 2;
  }
  for (int i=0; i<4; i++) {          //checksum
    int checksum=0;
    for (int j=0; j<10; j++) {
      checksum += data[9 + i + j*5];
    }
    data[i+59] = checksum%2;
  }
  data[63] = 0;               
}

void manchesterEncoder() { // encodes the rfid data for the emulation process
  int i=0, j=0;
  //Manchester
  if (activeId) {
    for (i=0; i<15; i++) {
      for (j=0; j<64; j++) {
        data[j]? (GPE |= (1<<rfCoilPin)):(GPE &= ~(1<<rfCoilPin));
        delayMicroseconds(255);
        for (int k=0; k<14; k++) asm_nop(); //fine tuning

        data[j]? (GPE &= ~(1<<rfCoilPin)):(GPE |= (1<<rfCoilPin));
        delayMicroseconds(255);
        for (int k=0; k<13; k++) asm_nop(); //fine tuning
      }
    }
    manchesterCounter++;
    if (manchesterCounter == 15) {
      manchesterCounter = 0;
      activeId = 0;
    }
  }
}
