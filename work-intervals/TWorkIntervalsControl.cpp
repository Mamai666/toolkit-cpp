#include "TWorkIntervalsControl.h"

time_t  getCurrTime(void)
{
    time_t t;
    time(&t);
    return t;
}

// возвращает true, если детектор должен быть в работе, 0 в ожидании
bool TWorkIntervals::isRun()
{
    // если интервалы не заданы или заданы неверно - разрешить работу по умолчанию
    if (!m_Ena) {
        return 1;
    }

    if (m_Intervals.size()>0) {
        int currTime = getCurrTimeIntervals();

        for (auto& e : m_Intervals) {
            if (e.isRun(currTime)) {
                return m_Run;
            }
        }
    }
    return !m_Run;
}

int TWorkIntervals::getCurrTimeIntervals()
{
    time_t t = getCurrTime();

    struct tm T;
    localtime_r(&t, &T);

    char s[10];
    sprintf(s, "%02d%02d%02d", T.tm_hour, T.tm_min, T.tm_sec);

    return atoi(s);
}

// ------------------------------------------------------------------------------------------------
// очистка данных
void    TWorkIntervals::clear()
{
    m_Ena = 0;
    m_Run = 0;
    m_Intervals.clear();
}

// ------------------------------------------------------------------------------------------------
int     TWorkIntervals::stringToTime(std::string s)
{
    if (s.size() > 0) {
        int dHour = -1;
        int dMin = -1;
        int dSec = -1;
        if (sscanf(s.c_str(), "%d:%d:%d", &dHour, &dMin, &dSec) == 3) {
            if (dHour >= 0 && dHour <= 24 && dMin >= 0 && dMin < 60) {
                int d = (dHour * 100 + dMin) * 100 + dSec;
                if (d >= 0 && d <= 240000)
                    return d;
            }
        }
        else {
            LOG(ERROR) << "TWorkIntervals: Неверный формат времени, должен быть hh:mm:ss";
        }
    }
    return -1;
}

// ------------------------------------------------------------------------------------------------
// обновляет список интервалов при изменении конфигурации
// возвращает 1 при успешной проверке значений
bool    TWorkIntervals::update(TWorkIntervalsList workIntervListConf)
{
    clear();

    if (workIntervListConf.m_Intervals.size() > 0) {
        m_Run = workIntervListConf.m_Run;

        bool isFirst = 1;
        int timeFrom, timeTo;
        for (auto& e : workIntervListConf.m_Intervals) {
            timeFrom = stringToTime(e.begin);
            timeTo = stringToTime(e.end);

            if (isFirst) {
                isFirst = 0;
                if (timeFrom < 0) return false;
            }
            else {
                if (timeFrom <= 0) return false;
            }

            if (timeTo <= 0) return false;
            if (timeTo <= timeFrom) return false;

            m_Intervals.push_back({ timeFrom, timeTo });
        }

        m_Ena = 1;

        LOG(INFO) << "Заданы рабочие интервалы времени: ";
        for (auto& e : m_Intervals) {
            LOG(INFO) << "\tFrom " << e.m_timeFrom << " To " << e.m_timeTo;
        }

        return true;
    }
    else {
        LOG(WARNING) << "Не заданы рабочие интервалы времени!";
    }

    return false;
}

// ------------------------------------------------------------------------------------------------
void    TWorkIntervals::makeTime(char* s, int t)
{
    int sec = t % 100;
    int min = (t / 100) % 100;
    int hour = t / 10000;
    sprintf(s, "%02d:%02d:%02d", hour, min, sec);
}
