#ifndef TIMER_H
#define TIMER_H

#include <chrono>
#include <ctime>
#include <cmath>
#include <systemd/sd-daemon.h>
#include <iostream>
#include "unistd.h"
#include "loggerpp/include/LoggerPP.h"

class Timer
{
public:
    void start()
    {
        m_StartTime = std::chrono::steady_clock::now();
      //  m_StartTime = std::chrono::high_resolution_clock::now();
        m_bRunning = true;
    }

    void stop()
    {
        m_EndTime = std::chrono::steady_clock::now();
        //m_EndTime = std::chrono::high_resolution_clock::now();
        m_bRunning = false;
    }

    double elapsedMilliseconds()
    {
        //std::chrono::time_point<std::chrono::high_resolution_clock> endTime;
        std::chrono::time_point<std::chrono::steady_clock> endTime;

        if(m_bRunning)
        {
            //endTime = std::chrono::high_resolution_clock::now();
            endTime = std::chrono::steady_clock::now();
        }
        else
        {
            endTime = m_EndTime;
        }

        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - m_StartTime).count();
        return duration;
    }

    double elapsedMicroseconds()
    {
        //std::chrono::time_point<std::chrono::high_resolution_clock> endTime;
        std::chrono::time_point<std::chrono::steady_clock> endTime;

        if(m_bRunning)
        {
            //endTime = std::chrono::high_resolution_clock::now();
            endTime = std::chrono::steady_clock::now();
        }
        else
        {
            endTime = m_EndTime;
        }

        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - m_StartTime).count();
        return duration;
    }

    double elapsedSeconds()
    {
        return elapsedMilliseconds() / 1000.0;
    }

    bool isRunning()
    {
        return m_bRunning;
    }

private:
    //std::chrono::time_point<std::chrono::high_resolution_clock> m_StartTime;
   // std::chrono::time_point<std::chrono::high_resolution_clock> m_EndTime;
    std::chrono::time_point<std::chrono::steady_clock> m_StartTime;
    std::chrono::time_point<std::chrono::steady_clock> m_EndTime;
    bool                                               m_bRunning = false;
};

class SimpleWatchDog
{
public:
    void watchDog_init()
    {
        watchDogDelay = 0;
        if(sd_watchdog_enabled(0, &watchDogTimer) > 0)
        {
            sd_notifyf(0, "READY=1\n"
                        "STATUS=Start processing...\n"
                        "MAINPID=%lu", (unsigned long)getpid());

            watchDogDelay = int(watchDogTimer / 1000) / 2;
            LOG(INFO) << "-> Включён защитный таймер SystemD с интервалом опроса" << watchDogDelay << "миллисекунд.\n";

            sd_notify(0, "WATCHDOG=1");
        }
    }

    void watchDog_start()
    {
        watchDog.start();
    }

    void watchDog_sendNotify()
    {
        if(watchDogDelay > 0 && watchDog.elapsedMilliseconds() > watchDogDelay)
        {
            sd_notify(0, "WATCHDOG=1");
            sd_notify(0, "STATUS=Working...\n");
            watchDog.stop();
            watchDog.start();
        }
    }

private:
    /******* FOR WatchDog***********/
        uint64_t watchDogTimer = 0;
        int watchDogDelay;
        Timer watchDog;
    /********** end **************/
};

#endif // TIMER_H
