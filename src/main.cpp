#include <Arduino.h>
#include <SoftwareSerial.h>
#include <SPI.h>
#include <Wire.h>
#include <OneWire.h>
#include <EEPROM.h>
#include <Sleep_n0m1.h>
#include "Sensors.h"
#include "Transiever.h"
#include "RealTimeClock.h"

#define SD_CS 9 //Chip select для карты памяти
#define NSS 10 //пин Chip select (slave select) lora
#define RST 3 //пин сброса lora
#define DIO0 4 // цифровой пин для lora
#define TXEN 8 //режим на передачу
#define RXEN 7 //режим на прием
#define DS18B20_PIN A0 //пин data датчика ds18b20
#define INT0_PIN 2 //пин для пробуждения
#define MAX_PACKET_SIZE 100
#define DATETIME_PACKET_SIZE 6
#define ANSWER_TIMEOUT 10000

SoftwareSerial mySerial(5, 6); // RX, TX
OneWire oneWire(DS18B20_PIN);
Sensors sensors;
Transiever sx1276;
RealTimeClock rtc;
Sleep deepSleep;


uint8_t dataOut[MAX_PACKET_SIZE];
uint8_t dataIn[MAX_PACKET_SIZE];
uint8_t dateTime[6];
uint8_t pointer = 0;
uint8_t n = 0;
uint8_t crc;
uint64_t uuid;

uint64_t readID() {
  uint64_t id = 0;
  for (uint8_t i = 0; i < 8; i++) {
    id |= (uint64_t)EEPROM.read(7 - i) << (8 * i);  // Читаем от старшего к младшему
    mySerial.print(EEPROM.read(i), HEX);
  }
  return id;
}

void getDateTimeFromPacket(uint8_t* dateTime) {
    for (uint8_t i = 9; i < 15; i++)
      dateTime[i - 9] = dataIn[i];
}

/*
Конвертирует uint64_t в 8 элементов массива типа uint8_t
*/
String uint64_to_array(uint64_t value, uint8_t* array) {
    String result = "";
    for (int i = 0; i < 8; ++i) {
        array[i] = (value >> (8 * (7 - i))) & 0xFF;
        result += String(array[i], HEX);
    }
    return result;
}

void clearInputOutputBuffer() {
  //очищаем буффер
  for (uint8_t i = 0; i < MAX_PACKET_SIZE; i++) {
    dataIn[i] = 0;
    dataOut[i] = 0;
  }
}

//a3 f5 e2 d8 1c 9b 4e 7f
void setup() {
  mySerial.begin(115200);

  mySerial.print("\nAgroprobe ID: ");
  uuid = readID();
  mySerial.printf("\nAlarm period: %uh:%um\n", EEPROM.read(8), EEPROM.read(9));

  rtc.init(mySerial);
  sensors.init(mySerial);
  sx1276.init(mySerial);

  sx1276.sendHandshakePacket(uuid, mySerial);
  sx1276.receivePacket(dataIn, mySerial, ANSWER_TIMEOUT);

  uint8_t dateTime[6];
  getDateTimeFromPacket(dateTime);
  rtc.setRTCDateTime(dateTime);
}

void loop() {
  clearInputOutputBuffer();

  dataOut[pointer++] = 0x7F; // записываем в 0 байт протокол
  uint64_to_array(uuid, &dataOut[1]); // записываем ID  зонда с 1 по 8 байт
  pointer = 9;
  uint8_t* dateTime = rtc.getCurrentDateTime();
  for (uint8_t i = 0; i < 6; i++)
    dataOut[pointer++] = dateTime[i]; // записываем с 9 по 14 байт дату и время

  sensors.readPressure(dataOut, pointer, mySerial); // 15 и 16 байт атмосферное давление
  pointer++; //явно увеличиваем указатель в выходном буффере

  sensors.readAirTemperature(dataOut, pointer, mySerial); // 17-18, 19-20 байт температура с bmp и c htu
  pointer++;

  sensors.readAirHumidity(dataOut, pointer, mySerial); // 21-22 байт влажность с htu
  
  sx1276.sendDataPacket(dataOut, pointer, mySerial);
  sx1276.receivePacket(dataIn, mySerial, ANSWER_TIMEOUT);

  mySerial.println(rtc.setRTCDateTime(&dataIn[9])); // синхронизируем время

  pointer = 0;
  
  mySerial.println(rtc.setAlarm(1, 0));
  mySerial.println();
  deepSleep.pwrDownMode();
  deepSleep.sleepPinInterrupt(INT0_PIN, FALLING);
}

