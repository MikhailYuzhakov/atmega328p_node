#include <DS3232RTC.h>
#include "Adafruit_HTU21DF.h"
#include <Adafruit_BMP280.h>
#include <Adafruit_ADS1X15.h>
#include <DallasTemperature.h>


#define DS18B20_PIN A0 //пин data датчика ds18b20

OneWire oneWire_in(DS18B20_PIN); //объявляем класс для onewire
DallasTemperature sensor_inhouse(&oneWire_in); //объявляем класс для ds18b20
Adafruit_ADS1115 ads; //class for ADS1115

#define ADDRESS_BMP280 0x76

Adafruit_HTU21DF htu = Adafruit_HTU21DF();
Adafruit_BMP280 bmp; // I2C

class Sensors
{
    private:
        time_t compileTime() { //функция возвращает время компиляции
            const time_t FUDGE(10);    //fudge factor to allow for upload time, etc. (seconds, YMMV)
            const char *compDate = __DATE__, *compTime = __TIME__, *months = "JanFebMarAprMayJunJulAugSepOctNovDec";
            char compMon[3], *m;

            strncpy(compMon, compDate, 3);
            compMon[3] = '\0';
            m = strstr(months, compMon);

            tmElements_t tm;
            tm.Month = ((m - months) / 3 + 1);
            tm.Day = atoi(compDate + 4);
            tm.Year = atoi(compDate + 7) - 1970;
            tm.Hour = atoi(compTime);
            tm.Minute = atoi(compTime + 3);
            tm.Second = atoi(compTime + 6);

            time_t t = makeTime(tm);
            return t + FUDGE;        //add fudge factor to allow for compile time
        }
    public:
        void init(SoftwareSerial &mySerial) {
            if (htu.begin()) 
                mySerial.print("Humidity sensor...ok\n");
            else
                mySerial.print("Humidity sensor...failed\n");

            if (bmp.begin(ADDRESS_BMP280)) {
                mySerial.print("Atm. pressure sensor...ok\n");

                bmp.setSampling(Adafruit_BMP280::MODE_NORMAL,     /* Operating Mode. */
                                Adafruit_BMP280::SAMPLING_X2,     /* Temp. oversampling */
                                Adafruit_BMP280::SAMPLING_X16,    /* Pressure oversampling */
                                Adafruit_BMP280::FILTER_X16,      /* Filtering. */
                                Adafruit_BMP280::STANDBY_MS_500); /* Standby time. */
            } else {
                mySerial.print("Atm. pressure sensor...failed\n");
            }
        }

        /**
         * Считывает атмосферное давление с BMP280 и добавляет его в массив dataOut с индекса pointer
         */
        void readPressure(uint8_t* dataOut, uint8_t &pointer, SoftwareSerial &mySerial) {  // pointer передается по ссылке          
            // Чтение давления с проверкой ошибок
            float pressure = bmp.readPressure() / 100 * 0.750062f;  // Па -> гПа
            
            if (isnan(pressure)) {
                mySerial.println("Error reading pressure");
                return;
            }
            
            // Форматированный вывод
            mySerial.print("bmp_p=");
            mySerial.print(pressure);
            mySerial.print("mmHg");
            
            // Преобразование и проверка диапазона
            uint16_t value = (uint16_t)round(pressure * 10);
            
            // Проверка на переполнение (0-65535 mmHg)
            if (pressure >= 0 && pressure <= 65535) {
                dataOut[pointer++] = (value >> 8) & 0xFF;  // Старший байт
                dataOut[pointer] = value & 0xFF;         // Младший байт
            } else {
                mySerial.print("Out of range!");
            }
        }

        void readAirTemperature(uint8_t* dataOut, uint8_t &pointer, SoftwareSerial &mySerial) {  // pointer по ссылке           
            // Чтение температур с проверкой ошибок
            float bmpTemperature = bmp.readTemperature();
            float htuTemperature = htu.readTemperature();
            
            // Проверка на ошибки чтения
            if (isnan(bmpTemperature) || isnan(htuTemperature)) {
                mySerial.println("Error reading sensors");
                return;
            }
            
            // Форматированный вывод
            mySerial.print(" bmp_t=");
            mySerial.print(bmpTemperature);
            mySerial.print("°C htu_t=");
            mySerial.print(htuTemperature);
            
            // Преобразование температур (учет отрицательных значений)
            int16_t airTempBmp280 = round(bmpTemperature * 100);
            int16_t airTempHtu21 = round(htuTemperature * 100);
            
            // Запись в массив (big-endian)
            dataOut[pointer++] = (airTempBmp280 >> 8) & 0xFF;
            dataOut[pointer++] = airTempBmp280 & 0xFF;
            dataOut[pointer++] = (airTempHtu21 >> 8) & 0xFF;
            dataOut[pointer] = airTempHtu21 & 0xFF;
        }

<<<<<<< HEAD
        String readAirHumidity(uint8_t* dataOut, uint8_t &pointer) {
            String result = "";

=======
        void readAirHumidity(uint8_t* dataOut, uint8_t &pointer, SoftwareSerial &mySerial) {
>>>>>>> 3c0ef088c30160d6eb31f10c913e1fe4c6f290db
            float airHumidity = htu.readHumidity();

            // Проверка на ошибки чтения
            if (isnan(airHumidity)) {
                mySerial.print("Error reading sensors");
                return;
            }

            mySerial.print(" htu_h=");
            mySerial.print(airHumidity);
            mySerial.print("%\n");

            uint16_t airHumidityHtu21 = (uint16_t)round(airHumidity * 10);
            dataOut[pointer++] = (airHumidityHtu21 >> 8) & 0xFF;
            dataOut[pointer] = airHumidityHtu21 & 0xFF;
        }

        String readSoilHumidity(uint8_t* dataOut, uint8_t &pointer) {
            String result = "";
            
            const float k0 = -0.0085; const float b0 = 164.74;
            const float k1 = -0.0085; const float b1 = 164.74;
            const float k2 = -0.0085; const float b2 = 164.74; 

            ads.setGain(GAIN_ONE);
            if (!ads.begin()) {
                result = "Failed to initialize ADS1115!";
                return result;
            }

            // Выполняем измерения
            int32_t adc0 = 0, adc1 = 0, adc2 = 0;
            const uint16_t n = 500; // объем выборки
            
            for (uint16_t k = 0; k < n; k++) {
                adc0 += ads.readADC_SingleEnded(0);
                adc1 += ads.readADC_SingleEnded(1);
                adc2 += ads.readADC_SingleEnded(2);
            }
            //переводим ads1115 в сон



            // Рассчитываем средние значения
            uint16_t adc0_sr = adc0 / n;
            uint16_t adc1_sr = adc1 / n;
            uint16_t adc2_sr = adc2 / n;
            
            
           // Ограничиваем значения 0-100%
            uint8_t h0 = constrain(uint8_t(k0 * adc0_sr + b0), 0, 100);
            uint8_t h1 = constrain(uint8_t(k1 * adc1_sr + b1), 0, 100);
            uint8_t h2 = constrain(uint8_t(k2 * adc2_sr + b2), 0, 100);
         
            result.reserve(100);
            result += " hum0_15cm=";
            result += String(h0);
            result += "%, hum1_10cm=";
            result += String(h1);
            result += "%, hum2_5cm=";
            result += String(h2);
            result += "%";
            
            dataOut[pointer++] = h0;
            dataOut[pointer++] = h1;
            dataOut[pointer] = h2;
            
              return result;
        }

        String readSoilTemperature(uint8_t* dataOut, uint8_t &pointer) {
            String result = "";
            sensor_inhouse.begin();
            // Адреса датчиков (пронумерованы от 0 до 2 снизу вверх). Трейтий датчик ближе всего к поверхности земли
            const byte ds18b20_0[8] = {0x28, 0x85, 0x69, 0xBE, 0x00, 0x00, 0x00, 0x79};   //номер 18
            const byte ds18b20_1[8] = {0x28, 0x40, 0xE2, 0x56, 0xB5, 0x01, 0x3C, 0xB3};   //номер 17 
            const byte ds18b20_2[8] = {0x28, 0x11, 0x40, 0x75, 0xD0, 0x01, 0x3C, 0x72};   //номер 16

            // Запрос температуры от всех датчиков
            sensor_inhouse.requestTemperatures();
            
            // Чтение температуры и проверка на ошибки
            float temp0 = sensor_inhouse.getTempC(ds18b20_0);
            float temp1 = sensor_inhouse.getTempC(ds18b20_1);
            float temp2 = sensor_inhouse.getTempC(ds18b20_2);

            // if (temp0 == DEVICE_DISCONNECTED_C || 
            //     temp1 == DEVICE_DISCONNECTED_C || 
            //     temp2 == DEVICE_DISCONNECTED_C) {
            //     // result = "Error reading DS18B20 sensors";
            //     // return result;
            // }

            // Преобразование температуры в целые числа (×100)
            uint16_t temp0_int = temp0 * 100;
            uint16_t temp1_int = temp1 * 100;
            uint16_t temp2_int = temp2 * 100;

            // Формирование строки результата
            // result.reserve(100);
            // result += " temp1=";
            // result += String(temp0, 1);
            // result += "°C, temp2=";
            // result += String(temp1, 1);
            // result += "°C, temp3=";
            // result += String(temp2, 1);
            // result += "°C";

            // Запись данных в выходной массив
            dataOut[pointer++] = (temp0_int >> 8) & 0xFF;
            dataOut[pointer++] = temp0_int & 0xFF;
            dataOut[pointer++] = (temp1_int >> 8) & 0xFF;
            dataOut[pointer++] = temp1_int & 0xFF;
            dataOut[pointer++] = (temp2_int >> 8) & 0xFF;
            dataOut[pointer] = temp2_int & 0xFF;

             return result;
        }

};

