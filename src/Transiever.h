#include <Arduino.h>
#include <EEPROM.h>
#include <LoRa.h>

#define NSS 10 //пин Chip select (slave select) lora
#define RST 3 //пин сброса lora
#define DIO0 4 // цифровой пин для lora
#define TXEN 8 //режим на передачу
#define RXEN 7 //режим на прием
#define HS_PACKET_SIZE 10

class Transiever
{
    private:
        void txEnable(){
            digitalWrite(TXEN, HIGH);
            digitalWrite(RXEN, LOW);
        }

        void rxEnable(){
            digitalWrite(TXEN, LOW);
            digitalWrite(RXEN, HIGH);
        }
        
        /*
        Конвертирует uint64_t в 8 элементов массива типа uint8_t
        */
        void uint64_to_array(uint64_t value, uint8_t* array) {
            for (int i = 0; i < 8; ++i) {
                array[i] = (value >> (8 * (7 - i))) & 0xFF;
            }
        }
        /*
        Считает контрольную сумму CRC-8 побитовым сдвигом и полиномом 0x1D
        */
        uint8_t crc8_bitwise(const uint8_t *data, size_t length) {
            uint8_t crc = 0x00;  // Начальное значение 
            const uint8_t poly = 0x1D;  // Полином CRC-8 (SAE J1850)

            for (size_t i = 0; i < length; ++i) {
            crc ^= data[i];  // XOR с текущим байтом

                for (uint8_t bit = 0; bit < 8; ++bit) {
                    if (crc & 0x80) {  // Если старший бит = 1
                        crc = (crc << 1) ^ poly;
                    } else {
                        crc <<= 1;
                    }
                }
            }
            return crc;
        }

    public:
        void init(SoftwareSerial &mySerial) {
            uint32_t freq, bw;
            uint8_t sf, pwr, cr;
            cr = 4;

            freq = EEPROM.read(11);
            freq |= freq << 8;
            freq |= EEPROM.read(10);
            freq = freq - 1;
            freq = freq * 100000;
            bw = EEPROM.read(12);
            bw = bw * 1000;
            sf = EEPROM.read(13);
            pwr = EEPROM.read(14);

            LoRa.setPins(NSS, RST, DIO0);
<<<<<<< HEAD
            if (LoRa.begin(868E6)) {}
                // result += "SX1276 is ok.";
            else result += "SX1276 is failed.";
=======
            if (LoRa.begin(freq)) 
                mySerial.print("Transiever...ok\n");
            else 
                mySerial.print("Transiever...failed\n");
>>>>>>> 3c0ef088c30160d6eb31f10c913e1fe4c6f290db

            LoRa.setSignalBandwidth(bw);
            LoRa.setSpreadingFactor(sf);
            LoRa.setCodingRate4(cr);
            LoRa.setTxPower(pwr);

<<<<<<< HEAD
            // result = result + "\nПараметры SX1276:\nЧастота = " + freq + " Гц\n";
            // result = result + "Ширина полосы = " + bw + " Гц\n";
            // result = result + "Spreading factor = " + sf + "\n";
                        // result = result + "Мощность = " + pwr + " дБм\n";       
            return result;
=======
            mySerial.println("Transiever config:");
            mySerial.printf("Frequency = %lu Hz\n", freq);
            mySerial.printf("Bandwidth = %lu Hz\n", bw);
            mySerial.printf("Spreading factor = %u\n", sf);
            mySerial.printf("Coding rate = %u\n", cr);
            mySerial.printf("Power = %d dBm\n", pwr);
>>>>>>> 3c0ef088c30160d6eb31f10c913e1fe4c6f290db
        }

        void sendHandshakePacket(uint64_t uuid, SoftwareSerial &mySerial) {
            uint8_t dataOut[HS_PACKET_SIZE];
            char buf[4]; // Буфер для HEX-значений

            dataOut[0] = 0x8F; //байт для рукопожатия
            uint64_to_array(uuid, &dataOut[1]); //записываем UUID в dataOut с 1 по 8 байт включительно
            dataOut[9] = crc8_bitwise(dataOut, HS_PACKET_SIZE-1);

            LoRa.beginPacket();
            LoRa.write(dataOut, sizeof(dataOut));
            LoRa.endPacket();

            mySerial.print("Send bytes (handshake): " + String(HS_PACKET_SIZE) +  "| Data (HEX):");
            for (uint8_t i = 0; i < sizeof(dataOut); i++) {
                sprintf(buf, "%02X ", dataOut[i]); // Форматируем с ведущим нулём
                mySerial.print(buf);
            }

            mySerial.print("CRC8 = ");
            sprintf(buf, "%02X", dataOut[HS_PACKET_SIZE-1]);
            mySerial.print(buf);
            mySerial.println();
        }

        void sendDataPacket(uint8_t* dataOut, uint8_t &pointer, SoftwareSerial &mySerial) {
            pointer++;
            dataOut[pointer] = crc8_bitwise(dataOut, pointer - 1);
            
            LoRa.beginPacket();
            LoRa.write(dataOut, pointer + 1);
            LoRa.endPacket();

            // Оптимизированное формирование строки
            mySerial.print("Send bytes: ");
            mySerial.print(pointer+1);
            mySerial.print(" | Data (HEX): ");
            
            char buf[4]; // Буфер для HEX-значений
            for (uint8_t i = 0; i < pointer+1; i++) {
                sprintf(buf, "%02X ", dataOut[i]); // Форматируем с ведущим нулём
                mySerial.print(buf);
            }
            
            mySerial.print("CRC8 = ");
            sprintf(buf, "%02X", dataOut[pointer]);
            mySerial.print(buf);
            mySerial.println();
        }

        /**
         * Принимает пакет и записывает содержимое в переменную dataIn.
         * return сообщения об отладке в строковом типе.
         */
        void receivePacket(uint8_t* dataIn, SoftwareSerial &mySerial, uint16_t timeout_ms) {          
            long startTime = millis();
            bool packetReceived = false;
            uint8_t i = 0;
            uint8_t packetSize = 0;

            // Ожидание пакета
            while (millis() - startTime < timeout_ms) {
                int pSize = LoRa.parsePacket();
                if (pSize > 0) {
                    packetSize = pSize;
                    packetReceived = true;
                    break;
                }
                delay(10);
            }

            if (!packetReceived) {
                mySerial.print("Error timeout ");
                mySerial.print(timeout_ms);
                mySerial.print(" мс\n");
            }

            // Чтение данных
            char hexBuf[4]; // Буфер для HEX-значений
            mySerial.print("Recieve bytes: " + String(packetSize) + " | Data (HEX): ");

            while (LoRa.available() && i < packetSize) {
                dataIn[i] = LoRa.read();
                
                // Форматирование в HEX с ведущим нулем
                snprintf(hexBuf, sizeof(hexBuf), "%02X ", dataIn[i]);
                mySerial.print(hexBuf);
                i++;
            }

            // Проверка CRC
            uint8_t calculatedCrc = crc8_bitwise(dataIn, i-2);
            uint8_t receivedCrc = dataIn[i-1];

<<<<<<< HEAD
            // result += "\nCRC: Пакет=";
            // snprintf(hexBuf, sizeof(hexBuf), "%02X", receivedCrc);
            // result += hexBuf;
            // result += " Вычислено=";
            // snprintf(hexBuf, sizeof(hexBuf), "%02X", calculatedCrc);
            // result += hexBuf;
=======
            snprintf(hexBuf, sizeof(hexBuf), "%02X", receivedCrc);
            mySerial.print(" CRC8: in packet = 0x" + String(hexBuf));
            snprintf(hexBuf, sizeof(hexBuf), "%02X", calculatedCrc);
            mySerial.print(" calculated = 0x" + String(hexBuf));
>>>>>>> 3c0ef088c30160d6eb31f10c913e1fe4c6f290db

            if (calculatedCrc == receivedCrc) {
                mySerial.print(" correct\n");
            } else {
                mySerial.print(" uncorrect\n");
            }
        }
};

