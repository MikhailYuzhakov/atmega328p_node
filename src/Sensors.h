#include <DS3232RTC.h>
#include "Adafruit_HTU21DF.h"
#include <Adafruit_BMP280.h>

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
        String init() {
            String result = "";

            if (htu.begin()) 
                result += "HTU21 is ok.\n";
            else result += "HTU21 is failed.\n";
            
            if (bmp.begin(0x76)) {
                result += "BMP280 is ok.";

                bmp.setSampling(Adafruit_BMP280::MODE_NORMAL,     /* Operating Mode. */
                                Adafruit_BMP280::SAMPLING_X2,     /* Temp. oversampling */
                                Adafruit_BMP280::SAMPLING_X16,    /* Pressure oversampling */
                                Adafruit_BMP280::FILTER_X16,      /* Filtering. */
                                Adafruit_BMP280::STANDBY_MS_500); /* Standby time. */
            } else {
                result += "BMP280 is failed.";
            }
            
            return result;
        }

        /**
         * Считывает атмосферное давление с BMP280 и добавляет его в массив dataOut с индекса pointer
         */
        String readPressure(uint8_t* dataOut, uint8_t &pointer) {  // pointer передается по ссылке
            String result = "";
            
            // Чтение давления с проверкой ошибок
            float pressure = bmp.readPressure() / 100 * 0.750062f;  // Па -> гПа
            
            if (isnan(pressure)) {
                result = "Error reading pressure";
                return result;
            }
            
            // Форматированный вывод
            result.reserve(20);  // Оптимизация выделения памяти
            result += "bmp_p=";
            result += String(pressure, 1);  // 1 знак после запятой
            result += "mmHg";
            
            // Преобразование и проверка диапазона
            uint16_t value = (uint16_t)round(pressure * 10);
            
            // Проверка на переполнение (0-65535 mmHg)
            if (pressure >= 0 && pressure <= 65535) {
                dataOut[pointer++] = (value >> 8) & 0xFF;  // Старший байт
                dataOut[pointer++] = value & 0xFF;         // Младший байт
            } else {
                result += " (OUT OF RANGE)";
            }
            
            return result;
        }

        String readAirTemperature(uint8_t* dataOut, uint8_t &pointer) {  // pointer по ссылке
            String result = "";
            
            // Чтение температур с проверкой ошибок
            float bmpTemperature = bmp.readTemperature();
            float htuTemperature = htu.readTemperature();
            
            // Проверка на ошибки чтения
            if (isnan(bmpTemperature) || isnan(htuTemperature)) {
                result = "Error reading sensors";
                return result;
            }
            
            // Форматированный вывод
            result.reserve(50);  // Предварительное выделение памяти
            result += " bmp_t=";
            result += String(bmpTemperature, 1);
            result += "°C htu_t=";
            result += String(htuTemperature, 1);
            result += "°C";
            
            // Преобразование температур (учет отрицательных значений)
            int16_t airTempBmp280 = round(bmpTemperature * 100);
            int16_t airTempHtu21 = round(htuTemperature * 100);
            
            // Запись в массив (big-endian)
            dataOut[pointer++] = (airTempBmp280 >> 8) & 0xFF;
            dataOut[pointer++] = airTempBmp280 & 0xFF;
            dataOut[pointer++] = (airTempHtu21 >> 8) & 0xFF;
            dataOut[pointer++] = airTempHtu21 & 0xFF;
            
            return result;
        }

        String readAirHumidity(uint8_t* dataOut, uint8_t pointer) {
            String result = "";

            float airHumidity = htu.readHumidity();

            // Проверка на ошибки чтения
            if (isnan(airHumidity)) {
                result = "Error reading sensors";
                return result;
            }

            result.reserve(50);  // Предварительное выделение памяти
            result += " htu_h=";
            result += String(airHumidity, 1);
            result += "%";

            uint16_t airHumidityHtu21 = (uint16_t)round(airHumidity * 100);
            dataOut[pointer++] = (airHumidityHtu21 >> 8) & 0xFF;
            dataOut[pointer++] = airHumidityHtu21 & 0xFF;

            return result;
        }
};

