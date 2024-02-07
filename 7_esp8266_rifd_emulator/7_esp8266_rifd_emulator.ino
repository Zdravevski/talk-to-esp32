// The script is written by Slavko Zdravevski
// If you want to support my work, you can subscribe to my youtube channel: https://bit.ly/3FG9hpK
// I do a lot of interesting things in my free time, so you might find something of your interest or we can exchange ideas and knowledge

// Pinout declaration
// Output pins
#define RF_PIN D1

// Additional variables needed through the code
unsigned long currentSecounds, previousSecoundsRecord;
String incomingString = "";
String operationDeterminerString = "";
String operationString = "";
static uint8_t data[64];
static uint8_t value[10];
static uint8_t activeId = 1;
uint8_t  vendorActive = 0;
uint32_t idActive = 13264846; // 0013264846, it's just for the start, it will be overwritten through the app

static inline void asm_nop() {
    asm volatile ("nop"); 
}

void setup() {
  Serial.begin(9600);
  pinMode(RF_PIN, INPUT);
}

void loop() {
  operationDeterminer();
  sendGoodConnectionFlag();
  ManchesterEncoder();
}

void operationDeterminer () { // determines which operation needs to be executed
  if (Serial.available()) {
    incomingString = Serial.readString();
    operationDeterminerString = incomingString.substring(0, incomingString.indexOf('|'));
    operationString = incomingString.substring(incomingString.indexOf('|')+1, incomingString.length());

    if (operationString.indexOf('|') == -1) { // if the incomingString is not an empty string
      if (operationDeterminerString == "RFID") {
        uint32_t rfidToPrepare = operationString.toInt();
        Serial.println(String(rfidToPrepare));
        prepare_data(rfidToPrepare, vendorActive);
      }
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

void prepare_data(uint32_t ID, uint32_t VENDOR) { // prepares the data to be sent according to the protocol
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

void ManchesterEncoder() { // encodes the data 
  int i=0, j=0;
  //Manchester
  if (activeId) {
    for (i=0; i<15; i++) {
      for (j=0; j<64; j++) {
        data[j]? (GPE |= (1<<RF_PIN)):(GPE &= ~(1<<RF_PIN));
        delayMicroseconds(255);
        for (int k=0; k<14; k++) asm_nop(); //fine tuning

        data[j]? (GPE &= ~(1<<RF_PIN)):(GPE |= (1<<RF_PIN));
        delayMicroseconds(255);
        for (int k=0; k<13; k++) asm_nop(); //fine tuning
      }
    }
  }
}
