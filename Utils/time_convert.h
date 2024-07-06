#include <cstring>
#include <iomanip>
#include <string>
#include <chrono>
#include "mstrings.h"

using namespace std::chrono_literals;

namespace MTime
{

static std::string utcToHuman(long timeUTCMks, std::string dateFormat = "%Y-%m-%d %H:%M:%S.zzz")
{
    auto sss = std::chrono::microseconds(timeUTCMks);
    auto tp = std::chrono::system_clock::time_point(sss);
    auto seconds = std::chrono::time_point_cast<std::chrono::seconds>(tp);
    auto fraction = tp - seconds;
    auto microseconds = std::chrono::duration_cast<std::chrono::microseconds>(fraction);

    auto in_time_t = std::chrono::system_clock::to_time_t(tp);
    char out_time[24], out[35];
    std::memset(out, 0, sizeof(out));
    std::memset(out_time, 0, sizeof(out_time));
    std::string timestampHuman = "1970-01-01 23:59:59";

    std::string dateFormatNoMks = Strings::stdv_split(dateFormat, ".zz").front();

    if(0 < strftime(out_time, sizeof(out_time), dateFormatNoMks.c_str(), std::localtime(&in_time_t)))
    {
        if(dateFormat.rfind(".zzz") != std::string::npos)
        {
            std::snprintf(out, sizeof(out), "%s.%06ld", out_time, microseconds.count());
        }
        else
        {
            std::snprintf(out, sizeof(out), "%s", out_time);
        }
        timestampHuman = out;
    }

    return timestampHuman;
}

// Функция для конвертации строки формата человека в метку времени UTC в микросекундах
static long utcFromHuman(const std::string& timestamp, const std::string& dateFormat = "%Y-%m-%d %H:%M:%S.zzz")
{
    struct tm tm = {0};
    std::istringstream ss(timestamp);

    // Извлечение части формата даты без микросекунд
    std::string dateFormatNoMks = dateFormat.substr(0, dateFormat.find(".zzz"));
    ss >> std::get_time(&tm, dateFormatNoMks.c_str());

    // Проверка на успешность парсинга даты
    if (ss.fail())
    {
        return 0; // Возврат метки времени для 1970-01-01
    }

    // Парсинг микросекунд, если они присутствуют в формате
    std::chrono::microseconds microseconds(0);
    if (dateFormat.find(".zzz") != std::string::npos)
    {
        char dot;
        int us;
        if ((ss >> dot >> us) && dot == '.')
        {
            microseconds = std::chrono::microseconds(us);
        }
        else
        {
            return 0; // Возврат метки времени для 1970-01-01
        }
    }

    // Преобразование tm в time_t
    auto time_c = std::mktime(&tm);
    if (time_c == -1)
    {
        return 0; // Возврат метки времени для 1970-01-01
    }

    // Преобразование time_t в std::chrono::system_clock::time_point
    auto time_point = std::chrono::system_clock::from_time_t(time_c);
    time_point += microseconds;

    // Преобразование time_point в микросекунды с начала эпохи
    auto epoch = std::chrono::system_clock::time_point{};
    auto time_since_epoch = std::chrono::duration_cast<std::chrono::microseconds>(time_point - epoch);

    return time_since_epoch.count();
}

static long nowUTC(bool steadyClock = false)
{
    std::chrono::microseconds t ;

    if(steadyClock)
    {
        t = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now().time_since_epoch());
    }
    else {
        t = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now().time_since_epoch());
    }
    return t.count();
}

static std::string nowTimeStamp(std::string dateFormat = "%Y-%m-%dT%H:%M:%S.zzz")
{
    return utcToHuman(nowUTC(), dateFormat);
}

}
