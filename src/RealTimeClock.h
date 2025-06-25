#include <DS3232RTC.h>
#include <time.h>

DS3232RTC myRTC;

class RealTimeClock
{
    private:
        /* data */
    public:
        String init() {
            myRTC.begin();
            return "RTC init successfully.";
        }

        /**
         * Устанавливает время в DS3231 на основе массива uint8_t[6]
         * Формат массива:
         * [0] - день (1-31)
         * [1] - месяц (1-12)
         * [2] - год (текущий год - 2000)
         * [3] - часы (0-23)
         * [4] - минуты (0-59)
         * [5] - секунды (0-59)
         */
        void setRTCDateTime(const uint8_t* datetime) {
            tmElements_t tm;
            
            // Заполняем структуру tmElements_t из массива
            tm.Day = datetime[0];          // День (1-31)
            tm.Month = datetime[1];        // Месяц (1-12)
            tm.Year = datetime[2] + 30;    // Год (текущий год - 2000 + 30 = год с 1970)
            tm.Hour = datetime[3];         // Часы (0-23)
            tm.Minute = datetime[4];       // Минуты (0-59)
            tm.Second = datetime[5];       // Секунды (0-59)

            // Преобразуем в time_t (UNIX-время)
            time_t t = makeTime(tm);
            
            // Устанавливаем время в DS3231
            myRTC.set(t);
        }

        uint8_t* getCurrentDateTime() {
            static uint8_t datetime[6]; 
            // Получаем текущее время
            time_t t = myRTC.get();

            // Преобразуем в структуру tmElements_t
            tmElements_t tm;
            breakTime(t, tm);

            // Заполняем массив
            datetime[0] = tm.Day;            // День (1-31)
            datetime[1] = tm.Month;          // Месяц (1-12)
            datetime[2] = tm.Year - 30;      // Год (год-1970 → год-2000)
            datetime[3] = tm.Hour;           // Часы (0-23)
            datetime[4] = tm.Minute;         // Минуты (0-59)
            datetime[5] = tm.Second;         // Секунды (0-59)
            return datetime;
        }
        
};
