#include <SPI.h>
#include <LoRa.h>
#include "ds3231.h"
#include <Pangodream_18650_CL.h>
//VARIABLES
#include <Wire.h>
#include "img.h"

//LORA TRANSCEIVER
#define SCK 5
#define MISO 19
#define MOSI 27
#define SS 18
#define RST 23
#define DIO0 26


// VARIABLES FOR OUTPUT
boolean buzzer = true;
#define BUTTON_PIN 32
#define BUZZER_PIN 33

//MISC. VARIABLES
String Mymessage = "";
String DecMessage;

//FREQUENCY BAND -433E6 for Asia -866E6 for Europe -915E6 for North America
//THIS IS FOR JAPAN
#define ADC_PIN 35
#define CONV_FACTOR 1.8
#define READS 20

OLED_CLASS_OBJ display(OLED_ADDRESS, OLED_SDA, OLED_SCL);
Pangodream_18650_CL BL(ADC_PIN, CONV_FACTOR, READS);

//battery


void setup() {
  Serial.begin(115200);
  Serial.println("LoRa");
  
  //SPI LoRa pins
  SPI.begin(SCK, MISO, MOSI, SS);

  //setup LoRa transceiver module
  LoRa.setPins(SS, RST, DIO0);

  //STARTS LORA
  if (!LoRa.begin(BAND)) {
    Serial.println("Starting LoRa failed!");
    //while (1)
    ;
  }

if (OLED_RST > 0) {
        pinMode(OLED_RST, OUTPUT);
        digitalWrite(OLED_RST, HIGH);
        delay(100);
        digitalWrite(OLED_RST, LOW);
        delay(100);
        digitalWrite(OLED_RST, HIGH);
    }

    display.init();
    display.flipScreenVertically();
    display.clear();
    display.setFont(ArialMT_Plain_16);
    display.setTextAlignment(TEXT_ALIGN_CENTER);
    display.drawString(display.getWidth() / 2, display.getHeight() / 2, LORA_SENDER ? "LoRa Sender" : "Just Co., Ltd.");
    display.display();
    delay(2000);

  //LORA SETTINGS
  LoRa.setSpreadingFactor(11);
  LoRa.setSignalBandwidth(500E3);
  LoRa.setCodingRate4(5);
  LoRa.setTxPower(10);
  LoRa.setLdoFlag(true);

  Serial.println("LoRa Initializing OK!");

  // put your setup code here, to run once:

}

void loop() {
  
  //checks if there is an incoming message
  int packetSize = LoRa.parsePacket();

//TEST RECEIVE
  if (packetSize) {
    Serial.println(packetSize);
    //TEST DECODE 
    getMessage(packetSize);
  }

  //TEST SEND
  turnOff();
  delay(2000);

delay(10);

}

//LORA FUNCTIONS

//GET MESSAGE PACKETS
void getMessage(int packetSize) {
  // received a packet
  Serial.print("Received packet '");
  Serial.println(packetSize);
  byte packet[packetSize];
  int id = 0;
  Mymessage = "";

  // read packet
  while (LoRa.available()) {
    byte data = LoRa.read();
    Mymessage = data + " ";
    packet[id] = data;
    id++;
    Serial.print("0x");
    Serial.print((char)data, HEX);
    Serial.print(" ");
  }

  // print RSSI of packet
  Serial.print("' with RSSI ");
  Serial.println(LoRa.packetRssi());

  if (!buzzer && DecodeReceived(packet) == "BUZZON") {
    buzzer = true;
    //mainDisp();

  } else if (buzzer && DecodeReceived(packet) == "BUZZOFF") {
    buzzer = false;
    //mainDisp();
  }



}



//DECODING FUNCTIONS

String DecodeReceived(byte data[]) {
  //byte data[] = {0x44, 0x19, 0x0, 0x5, 0xE5, 0xF4, 0xE2, 0xE5, 0xFE };
  //Message Size
  int size = (int)data[0] - 64;
  Serial.print("Size: ");
  Serial.print(size);
  //Channel
  Serial.print(" Channel: ");
  Serial.print(data[1], HEX);
  Serial.println(" No: " + String(data[1]) + " Freq: " + ((int)data[1] + 862) + " Mhz");
  //Address
  Serial.print("Address: ");
  Serial.print(String(data[2]) + " " + String(data[3]));
  //FOR MORE FILTERED MESSAGE ADD IF STATEMENT
  
  //key
  Serial.print(" Key: ");
  Serial.print(getCDEByteKey(data[1]), HEX);
  byte deckey = getCDEByteKey(data[1]);
  //Main Message
  Serial.print(" Message: ");
  String Message = "";
  for (int i = 4; i < (4 + size); i++) {
    Message = Message + (char)(data[i] ^ deckey);
  }
  Serial.print(Message);
  Serial.print(" Checksum: ");
  Serial.println(checksum(data, sizeof(data) - 1) == (uint8_t)data[sizeof(data) - 1] ? "Match" : "Not Match");
  delay(10000);

  return Message;

}

static uint8_t getCDEByteKey(const uint8_t _channelNo) {
  const uint8_t keyLUT[] = {0x0A, 0x09, 0x08, 0x0F, 0x0E, 0x0D, 0x0C, 0x03, 0x02, 0x01, 0x00, 0x07, 0x06, 0x05, 0x04, 0x0B};
  return ((_channelNo + 0x99) & 0xF0) |  keyLUT[_channelNo & 0x0F];
}

uint8_t checksum(uint8_t* buf, uint8_t s) {
  uint32_t sum = 0;
  for (uint8_t i = 0; i < s; ++i) {
    sum += buf[i];
  }
  sum = 0x100 - (sum & 0xFF);
  return (uint8_t)sum;
}

//SENDING MESSAGE FUNCTION
void turnOff() {
  //CREATE THE MESSAGE
  uint8_t     radioBuffer[12]   = {0};
  radioBuffer[0] = 0x40 + 7;  //Message size
  radioBuffer[1] = 0x3D;      //Channel
  radioBuffer[2] = 0xff;      //Address
  radioBuffer[3] = 0xff;      //Address
  sprintf((char*)&radioBuffer[4], "BUZZOFF");
  uint8_t key = getCDEByteKey(radioBuffer[1]);
  //ENCRYPT THE MESSAGE PART
  //encode data with the key
  for (uint8_t i = 4; i < 11; ++i) {
    radioBuffer[i] = radioBuffer[i] ^ key;
  }
  //compute checksum
  radioBuffer[11] = checksum(radioBuffer, 11);
  //PRINTS THE PACKET DATA
  for (uint8_t i = 0; i < 12; ++i) {
    Serial.print("0x");
    if (radioBuffer[i] < 0x10)  Serial.print("0");
    Serial.print(radioBuffer[i], HEX);
    Serial.print(" ");
  }
  Serial.println();

  //SENDS THE DATA
  LoRa.beginPacket();
  LoRa.write(radioBuffer, 12);
  LoRa.endPacket();
}

//BATTERY INFO FUNCTIONS
void battery_info(void *arg)
{
  while (1) {
    Serial.println("");
    Serial.print("Average value from pin: ");
    Serial.println(BL.pinRead());
    Serial.print("Volts: ");
    Serial.println(BL.getBatteryVolts());
    Serial.print("Charge level: ");
    Serial.println(BL.getBatteryChargeLevel());
    Serial.print("Reading: ");
    Serial.println(analogRead(35));

    if (BL.getBatteryVolts() >= 3.2) {
      clearLine(1);      
      printDisplay(WHITE, 1, 100,1, String(BL.getBatteryChargeLevel())+"%");
      display.drawBitmap(80, 0, BATTERY_BOX0, 128, 64, BLACK);
      display.drawBitmap(80, 0, BATTERY_BOX3, 128, 64, WHITE);
      display.display();
      vTaskDelay(500);
      display.drawBitmap(80, 0, BATTERY_BOX3, 128, 64, BLACK);
      display.drawBitmap(80, 0, BATTERY_BOX2, 128, 64, WHITE);
      display.display();
      vTaskDelay(500);
      display.drawBitmap(80, 0, BATTERY_BOX2, 128, 64, BLACK);
      display.drawBitmap(80, 0, BATTERY_BOX1, 128, 64, WHITE);
      display.display();
      vTaskDelay(500);
      display.drawBitmap(80, 0, BATTERY_BOX1, 128, 64, BLACK);
      display.drawBitmap(80, 0, BATTERY_BOX0, 128, 64, WHITE);
      display.display();
      vTaskDelay(500);

    } else {
      displayBatInfo();
      vTaskDelay(1000);
    }

  }

}
void displayBatInfo() {
  clearLine(1);
  int batteryLevel = BL.getBatteryChargeLevel();
  if (batteryLevel >= 67) {
    display.drawBitmap(80, 0, BATTERY_BOX1, 128, 64, WHITE);
  } else if (batteryLevel < 66 && batteryLevel >= 33 ) {
    display.drawBitmap(80, 0, BATTERY_BOX2, 128, 64, WHITE);
  } else if (batteryLevel < 32 && batteryLevel >= 10 ) {
    display.drawBitmap(80, 0, BATTERY_BOX3, 128, 64, WHITE);
  } else {
    display.drawBitmap(80, 0, BATTERY_BOX0, 128, 64, WHITE);
  }
  printDisplay(WHITE, 1, 100,1, String(BL.getBatteryChargeLevel())+"%");
  display.display();
}

