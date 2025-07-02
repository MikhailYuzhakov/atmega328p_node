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
        String uint64_to_array(uint64_t value, uint8_t* array) {
            String result = "";
            for (int i = 0; i < 8; ++i) {
                array[i] = (value >> (8 * (7 - i))) & 0xFF;
                result += String(array[i], HEX);
            }
            return result;
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
        String init()
        {
            uint32_t freq, bw;
            uint8_t sf, pwr;
            String result = "----------------------------------------------------------\n";

            freq = EEPROM.read(11);
            freq |= freq << 8;
            freq |= EEPROM.read(10);
            freq = freq - 1;
            freq = freq * 100000;
            bw = EEPROM.read(12);
            bw = bw * 1000;
            sf = EEPROM.read(13);
            pwr = EEPROM.read(14);
            // pinMode(TXEN, OUTPUT);
            // pinMode(RXEN, OUTPUT);

            LoRa.setPins(NSS, RST, DIO0);
            if (LoRa.begin(868E6)) {}
                // result += "SX1276 is ok.";
            else result += "SX1276 is failed.";

            LoRa.setSignalBandwidth(bw);
            LoRa.setSpreadingFactor(sf);
            LoRa.setCodingRate4(4);
            LoRa.setTxPower(pwr);
            // txEnable();

            // result = result + "\nПараметры SX1276:\nЧастота = " + freq + " Гц\n";
            // result = result + "Ширина полосы = " + bw + " Гц\n";
            // result = result + "Spreading factor = " + sf + "\n";
                        // result = result + "Мощность = " + pwr + " дБм\n";       
            return result;
        }

        String sendHandshakePacket(uint64_t uuid) {
            uint8_t dataOut[HS_PACKET_SIZE];
            String res1 = "ID = ";
        

            dataOut[0] = 0x8F; //байт для рукопожатия
            res1 += uint64_to_array(uuid, &dataOut[1]); //записываем UUID в dataOut с 1 по 8 байт включительно
            dataOut[9] = crc8_bitwise(dataOut, HS_PACKET_SIZE-1);

            LoRa.beginPacket();
            LoRa.write(dataOut, sizeof(dataOut));
            LoRa.endPacket();

            res1 += "\nHandshake packet is sent = ";
            for (uint8_t i = 0; i < sizeof(dataOut); i++)
                res1 += String(dataOut[i], HEX);

            res1 += " CRC8 = ";
            res1 += String(dataOut[HS_PACKET_SIZE-1], HEX);
            
            return res1;
        }

        String sendDataPacket(uint8_t* dataOut, uint8_t pointer) {
            pointer++;
            dataOut[pointer] = crc8_bitwise(dataOut, pointer - 1);
            
            LoRa.beginPacket();
            LoRa.write(dataOut, pointer + 1);
            LoRa.endPacket();

            // Оптимизированное формирование строки
            String result = "Отправлено байт: " + String(pointer + 1) + " | Данные (HEX): ";
            
            char buf[4]; // Буфер для HEX-значений
            for (uint8_t i = 0; i < pointer+1; i++) {
                sprintf(buf, "%02X ", dataOut[i]); // Форматируем с ведущим нулём
                result += buf;
            }
            
            result += "CRC8 = ";
            sprintf(buf, "%02X", dataOut[pointer]);
            result += buf;
            
            return result;
        }

        /**
         * Принимает пакет и записывает содержимое в переменную dataIn.
         * return сообщения об отладке в строковом типе.
         */
        String receivePacket(uint8_t* dataIn, uint16_t timeout_ms = 2000) {
            String result = "";
            result.reserve(128); // Заранее резервируем память
            
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
                return "Ошибка: пакет не получен за " + String(timeout_ms) + " мс";
            }

            // Чтение данных
            char hexBuf[4]; // Буфер для HEX-значений
            result = "Принято байт: ";
            result += String(packetSize);
            result += " | Данные (HEX): ";

            while (LoRa.available() && i < packetSize) {
                dataIn[i] = LoRa.read();
                
                // Форматирование в HEX с ведущим нулем
                snprintf(hexBuf, sizeof(hexBuf), "%02X ", dataIn[i]);
                result += hexBuf;
                i++;
            }

            // Проверка CRC
            uint8_t calculatedCrc = crc8_bitwise(dataIn, i-2);
            uint8_t receivedCrc = dataIn[i-1];

            // result += "\nCRC: Пакет=";
            // snprintf(hexBuf, sizeof(hexBuf), "%02X", receivedCrc);
            // result += hexBuf;
            // result += " Вычислено=";
            // snprintf(hexBuf, sizeof(hexBuf), "%02X", calculatedCrc);
            // result += hexBuf;

            if (calculatedCrc == receivedCrc) {
                result += " (Верно)";
            } else {
                result += " (Ошибка)";
            }

            return result;
        }
};

