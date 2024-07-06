#include <chrono>
#include <cstdlib>
#include <cstring>
#include "mqueue-client-worker.h"

#ifndef USE_FOREIGH_LOGGER
#include "logger_Wolh/logger.h"
#endif

void GtpCheckloopWorker::setItWorks(bool &newItWorks)
{
    m_itWorks = newItWorks;
}

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
    m_sendOne = false;

    while(m_itWorks || m_sendOne)
    {
        switch(m_outputStatus)
        {
        case gtpCheckLoopClient::Statuses::Status_OK:
            m_cli.createMsg("OK");
            break;
        case gtpCheckLoopClient::Statuses::Status_Startup:
            m_cli.createMsg("Module Start");
            break;
        case gtpCheckLoopClient::Statuses::Status_Off:
            m_cli.createMsg("Module Stop");
            break;
        case gtpCheckLoopClient::Statuses::Status_ErrStreamOpening:
            m_cli.createMsg("ERROR: Stream Opening");
            break;
        case gtpCheckLoopClient::Statuses::Status_ErrtStreamInfo:
            m_cli.createMsg("ERROR: Stream Info");
            break;
        case gtpCheckLoopClient::Statuses::Status_ErrStreamLookup:
            m_cli.createMsg("ERROR: Stream Lookup");
            break;
        case gtpCheckLoopClient::Statuses::Status_ErrFrameReceiving:
            m_cli.createMsg("ERROR: Frame Receiving");
            break;
        case gtpCheckLoopClient::Statuses::Status_ErrDiskWrite:
            m_cli.createMsg("FATAL: Disk Write");
            break;
        case gtpCheckLoopClient::Statuses::Status_ErrFrameReceivingTimeout:
            m_cli.createMsg("ERROR: Frame Receiving Timeout");
            break;
        case gtpCheckLoopClient::Statuses::Status_ErrFrameReceivingTimeoutFIFO:
            m_cli.createMsg("FATAL: Frame Receiving Timeout");
            break;
        case gtpCheckLoopClient::Statuses::Status_ErrOutputReconnect:
            m_cli.createMsg("FATAL: Output Reconnect");
            break;
        case gtpCheckLoopClient::Statuses::Status_ErrOutputHeaderSend:
            m_cli.createMsg("FATAL: Output Header Send");
            break;
        case gtpCheckLoopClient::Statuses::Status_ErrOutputFrameSend:
            m_cli.createMsg("FATAL: Output Frame Send");
            break;
        case gtpCheckLoopClient::Statuses::Status_ErrAsyncTimeOut:
            m_cli.createMsg("FATAL: Async Timeout");
            break;
        case gtpCheckLoopClient::Statuses::Status_ErrBadFrameHeadLength:
            m_cli.createMsg("FATAL: Invalid input frame header length");
            break;
        }

        m_sendOne = false; // Сообщение отправлено, сброс состояния
        bool retOK = m_cli.sendMsg(m_lastError);
        if(retOK)
        {
            gtpCheckLoopClient::Statuses f = m_outputStatus;
            m_lastDebugInfo = "Сигнал диспетчеру "+std::to_string((uint32_t)f)+" послан!"
                              " , Задержка обновления: " + std::to_string(elapsed());
    #if !defined(USE_FOREIGH_LOGGER) && defined(DEBUG_BUILD)
            D_pLogDebug("-- Сигнал диспетчеру %u послан! (Задержка обновления %d)", (uint32_t)f, elapsed());
    #endif
        }
        else
        {
            // Do something ..
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(m_delay));

        // Если не было обновлений больше двух интервалов, установить ошибку
        if(m_outputStatus != gtpCheckLoopClient::Statuses::Status_Off && (elapsed() > m_delay * 2))
        {
            m_outputStatus = gtpCheckLoopClient::Statuses::Status_ErrAsyncTimeOut;
            m_lastError = "Не было обновлений больше двух интервалов: Status_ErrAsyncTimeOut";
        }
    }
}

GtpCheckloopWorker::GtpCheckloopWorker():
    m_lastUpdate(std::chrono::steady_clock::now())
{
    m_delay = 1000;
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
    m_sendOne = true;
    updateTime();
}

bool GtpCheckloopWorker::start()
{
#ifndef USE_FOREIGH_LOGGER
    D_pLogDebugNA("-- Запуск вещания диспетчеру");
#endif
    bool ret = m_cli.openMq(m_lastError);
    updateTime();

    if(ret)
    {
        notify(gtpCheckLoopClient::Statuses::Status_Startup);
        m_worker = std::thread(&GtpCheckloopWorker::worker, this);
        isWasOpened = true;
    }

    return ret;
}

void GtpCheckloopWorker::stop()
{
    if(!isWasOpened) return;
#ifndef USE_FOREIGH_LOGGER
    D_pLogDebugNA("-- Отключение вещания диспетчеру");
#endif
    if(m_outputStatus != gtpCheckLoopClient::Statuses::Status_OK)
        m_outputStatus = gtpCheckLoopClient::Statuses::Status_Off;
    m_itWorks = false;
    if(m_worker.joinable())
        m_worker.join();

    m_cli.closeMq(m_lastError);
}

std::string GtpCheckloopWorker::lastError() const
{
    return m_lastError;
}

std::string GtpCheckloopWorker::lastDebugInfo() const
{
    return m_lastDebugInfo;
}
