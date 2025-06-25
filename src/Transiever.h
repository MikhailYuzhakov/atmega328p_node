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
            if (LoRa.begin(868E6)) 
                result += "SX1276 is ok.";
            else result += "SX1276 is failed.";

            LoRa.setSignalBandwidth(bw);
            LoRa.setSpreadingFactor(sf);
            LoRa.setCodingRate4(4);
            LoRa.setTxPower(pwr);
            // txEnable();

            result = result + "\nПараметры SX1276:\nЧастота = " + freq + " Гц\n";
            result = result + "Ширина полосы = " + bw + " Гц\n";
            result = result + "Spreading factor = " + sf + "\n";
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
            String result = "";
            dataOut[pointer] = crc8_bitwise(dataOut, pointer-1);

            LoRa.beginPacket();
            LoRa.write(dataOut, pointer+1);
            LoRa.endPacket();

            result += "Отправлено байт: ";
            result += pointer+1;
            result += " | Данные (HEX): ";
            for (uint8_t i = 0; i < pointer+1; i++) {
                result += String(dataOut[i], HEX);
                result += " ";
            }
                

            result += " CRC8 = ";
            result += String(dataOut[pointer], HEX);
            
            return result;
        }

        /**
         * Принимает пакет и записывает содержимое в переменную dataIn.
         * return сообщения об отладке в строковом типе.
         */
        String receivePacket(uint8_t* dataIn, uint16_t timeout_ms = 2000) {
            String result = "";
            uint16_t startTime = millis(); // Засекаем время начала ожидания

            // Ожидаем пакет в течение timeout_ms миллисекунд
            while (millis() - startTime < timeout_ms) {
                int packetSize = LoRa.parsePacket();
                
                if (packetSize > 0) {
                    result += "Принято байт: ";
                    result += String(packetSize);
                    result += " | Данные (HEX): ";

                    uint8_t i = 0;
                    while (LoRa.available() && i < packetSize) {
                        dataIn[i] = LoRa.read();
                        result += String(dataIn[i], HEX);
                        result += " "; // Разделитель для удобства чтения
                        i++;
                    }
                    uint8_t calculatedCrc8 = crc8_bitwise(dataIn, i-2);
                    if (calculatedCrc8 == dataIn[i-1]) {
                        result += "\nКонтрольная сумма верна";
                    } else {
                        result += "\nКонтрольная сумма неверная";
                        result += "\nКонтрольная сумма пакета: "; 
                        result += String(dataIn[i-1], HEX);
                        result += "\nКонтрольная сумма расчитанная: ";
                        result += String(calculatedCrc8, HEX);
                    }

                    return result; // Пакет успешно принят
                }

                delay(10); // Небольшая задержка, чтобы не нагружать процессор
            }

            return "Ошибка: пакет не получен за " + String(timeout_ms) + " мс";
        }
};

