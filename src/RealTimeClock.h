#include <DS3232RTC.h>
#include <time.h>

DS3232RTC myRTC;

class RealTimeClock
{
    private:
        uint8_t bcd2dec(uint8_t n)
        {
            return n - 6 * (n >> 4);
        }

        String printAlarmDate() {
            uint8_t value[2];
            String result = "Alarm time: ";
            myRTC.readRTC(0x08, value, 2);


            if (bcd2dec(value[1]) < 10)
                result += "0";
            result += bcd2dec(value[1]);
            result += ":";

            if (bcd2dec(value[0]) < 10)
                result += 0;
            result += bcd2dec(value[0]);
            return result;
        }

    public:
        void init(SoftwareSerial &mySerial) {
            myRTC.begin();
            myRTC.squareWave(myRTC.SQWAVE_NONE); //конфигурируем пин SQW (DS3231) на прерывания
            myRTC.alarmInterrupt(myRTC.ALARM_1, true); //разрешаем прерывания по 1 будильнику
            mySerial.print("RTC...ok\n");
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
        String setRTCDateTime(const uint8_t* datetime) {
            tmElements_t tm;
            String currentDateTime = "Date and time are synchronized: ";
            
            // Заполняем структуру tmElements_t из массива
            tm.Day = datetime[0];          // День (1-31)
            tm.Month = datetime[1];        // Месяц (1-12)
            tm.Year = datetime[2] + 30;    // Год (текущий год - 2000 + 30 = год с 1970)
            tm.Hour = datetime[3];         // Часы (0-23)
            tm.Minute = datetime[4];       // Минуты (0-59)
            tm.Second = datetime[5];       // Секунды (0-59)
            
            if (datetime[0] < 10)
                currentDateTime += "0";
            currentDateTime += String(datetime[0]);
            currentDateTime += ".";

            if (datetime[1] < 10)
                currentDateTime += "0";
            currentDateTime += String(datetime[1]);
            currentDateTime += ".";

            currentDateTime += "20";
            currentDateTime += String(datetime[2]);
            currentDateTime += " ";

            if (datetime[3] < 10)
                currentDateTime += "0";
            currentDateTime += String(datetime[3]);
            currentDateTime += ":";

            if (datetime[4] < 10)
                currentDateTime += "0";
            currentDateTime += String(datetime[4]);
            currentDateTime += ":";

            if (datetime[5] < 10)
                currentDateTime += "0";
            currentDateTime += String(datetime[5]);

            // Преобразуем в time_t (UNIX-время)
            time_t t = makeTime(tm);
            
            // Устанавливаем время в DS3231
            myRTC.set(t);
            return currentDateTime;
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

        String setAlarm(uint8_t hour, uint8_t min) {
            String result = "";
            uint8_t* datetime = getCurrentDateTime();

            if (datetime[4] + min > 59) {
                min = datetime[4] + min - 60;
                hour++;
            } else min = datetime[4] + min;

            if (datetime[3] + hour > 23) hour = 0 + datetime[3] + hour - 24;
            else hour = datetime[3] + hour;

            uint8_t sec = datetime[5];

            myRTC.setAlarm(myRTC.ALM1_MATCH_HOURS, sec, min, hour, 0);
            myRTC.alarm(myRTC.ALARM_1); //очищаем флаг будильника ALARM_1

            return printAlarmDate();
        }


        
};
