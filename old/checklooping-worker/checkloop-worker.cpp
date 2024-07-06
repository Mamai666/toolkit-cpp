#include <chrono>
#include "checkloop-worker.h"

#ifndef USE_FOREIGH_LOGGER
#include <logger.h>
#endif

void GtpCheckloopWorker::updateTime()
{
    m_lastUpdate = std::chrono::steady_clock::now();
}

int GtpCheckloopWorker::elapsed()
{
    using std::chrono::milliseconds;
    using std::chrono::duration_cast;
    std::chrono::steady_clock::time_point t = m_lastUpdate;
    return static_cast<int>(duration_cast<milliseconds>(std::chrono::steady_clock::now() - t).count());
}

void GtpCheckloopWorker::worker()
{
    m_itWorks = true;
    while(m_itWorks)
    {
        // КОСТЫЛЬ: Слать диспетчеру только 0 или 1
        if(m_outputStatus <= gtpCheckLoopClient::Statuses::Status_OK)
            m_cli.Notify(m_outputStatus);

    #ifndef USE_FOREIGH_LOGGER
        gtpCheckLoopClient::Statuses f = m_outputStatus;
        D_pLogDebug("-- Сигнал диспетчеру %u послан! (Задержка обновления %d)", (uint32_t)f, elapsed());
    #endif
        std::this_thread::sleep_for(std::chrono::milliseconds(m_delay));

        // Если не было обновлений больше двух интервалов, установить ошибку
        if(m_outputStatus != gtpCheckLoopClient::Statuses::Status_Off && (elapsed() > m_delay * 2))
            m_outputStatus = gtpCheckLoopClient::Statuses::Status_ErrAsyncTimeOut;
    }
}

GtpCheckloopWorker::GtpCheckloopWorker():
    m_lastUpdate(std::chrono::steady_clock::now())
{
    m_delay = 500;
    m_outputStatus = gtpCheckLoopClient::Statuses::Status_Off;
}

GtpCheckloopWorker::~GtpCheckloopWorker()
{
    stop();
}

void GtpCheckloopWorker::setDelay(int ms)
{
    m_delay = ms;
}

void GtpCheckloopWorker::notify(gtpCheckLoopClient::Statuses out)
{
    m_outputStatus = out;
    updateTime();
}

bool GtpCheckloopWorker::start()
{
#ifndef USE_FOREIGH_LOGGER
    D_pLogDebugNA("-- Запуск вещания диспетчеру");
#endif
    bool ret = m_cli.Open();
    updateTime();
    m_worker = std::thread(&GtpCheckloopWorker::worker, this);
    return ret;
}

void GtpCheckloopWorker::stop()
{
#ifndef USE_FOREIGH_LOGGER
    D_pLogDebugNA("-- Отключение вещания диспетчеру");
#endif
    m_outputStatus = gtpCheckLoopClient::Statuses::Status_Off;
    m_itWorks = false;
    if(m_worker.joinable())
        m_worker.join();

    m_cli.Notify(gtpCheckLoopClient::Statuses::Status_Off);
}
