#include <DS3232RTC.h>
#include "Adafruit_HTU21DF.h"
#include <Adafruit_BMP280.h>

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

        void readAirHumidity(uint8_t* dataOut, uint8_t &pointer, SoftwareSerial &mySerial) {
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
};

