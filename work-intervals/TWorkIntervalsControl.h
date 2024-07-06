#ifndef TWORKINTERVALSCONTROL_H
#define TWORKINTERVALSCONTROL_H

#include <vector>
#include <string>
#include "include_logger.h"

struct Interval_t
{
    std::string begin = "";
    std::string end   = "";

    void operator=(const Interval_t& r_val) {
        this->begin = r_val.begin;
        this->end   = r_val.end;
    }

    void operator=(const Interval_t* r_val)     {
        this->begin = r_val->begin;
        this->end   = r_val->end;
    }

    bool operator== (const Interval_t& rl) const{
        return (this->begin == rl.begin)
                && (this->end == rl.end);
    }

    bool operator!= (const Interval_t& rl) const{
        return (this->begin != rl.begin)
                || (this->end != rl.end);
    }
};

class TWorkIntervalsList {
    public:
        TWorkIntervalsList() {}
        ~TWorkIntervalsList() {}

        // 1 при успешной проверке содержимого json
        bool    m_Ena = 0;

        // 1 если заданы интервалы на включение, 0 - на выключение
        bool    m_Run = 0;

        // список интервалов
        std::vector<Interval_t> m_Intervals;

        void    Clear() {
            m_Ena = 0;
            m_Intervals.clear();
            m_Run = 0;
        }

        TWorkIntervalsList& operator=(const TWorkIntervalsList& r_val)
        {
            //Interval_t::operator=(r_val);
            this->m_Ena = r_val.m_Ena;
            this->m_Run   = r_val.m_Run;
            this->m_Intervals = r_val.m_Intervals;

            return *this;
        }

        bool operator== (const TWorkIntervalsList& rl) {
            //if (&rl == this) return true;
            return (this->m_Ena == rl.m_Ena)
                    && (this->m_Run == rl.m_Run)
                    && (this->m_Intervals == rl.m_Intervals);
        }

        bool operator!= (const TWorkIntervalsList& rl) {
           // if (&rl == this) return true;
            return (this->m_Ena != rl.m_Ena)
                    || (this->m_Run != rl.m_Run)
                    || (this->m_Intervals != rl.m_Intervals);
        }
};


class TWorkIntervals {
    public:
        TWorkIntervals() {}
        ~TWorkIntervals() {}

        // возвращает true, если детектор должен быть в работе, 0 в ожидании
        bool    isRun();

        // обновляет список интервалов при изменении конфигурации
        // возвращает 1 при успешной проверке значений
        bool    update(TWorkIntervalsList workIntervListConf);

        // очистка данных
        void    clear();

       // friend void     addJSON_WorkIntervalsList(AddJSON& addJSON);

    private:

        class WorkInterval {
            public:
                WorkInterval(int timeFrom, int timeTo) : m_timeFrom(timeFrom), m_timeTo(timeTo) {}
                ~WorkInterval() {}

                // возвращает true, если текущее время попадает в этот интервал
                bool    isRun(int currTime) { return (currTime >= m_timeFrom && currTime < m_timeTo); }

            friend TWorkIntervals;
            //friend void     addJSON_WorkIntervalsList(AddJSON& addJSON);
            private:
                // время начала интервала = hour*100+minutes
                const int     m_timeFrom;
                // время завершения интервала = hour*100+minutes
                const int     m_timeTo;
        };

        // на всех интервалах требуется:
        // true - включить, false - выключить
        bool m_Run = 0;

        // список интервалов
        std::vector<WorkInterval> m_Intervals;

        // признак, что данные успешно загружены и проверены
        bool m_Ena = 0;

        int     getCurrTimeIntervals();
        int     stringToTime(std::string);
        void    makeTime(char* s, int t);
};

#endif // TWORKINTERVALSCONTROL_H
