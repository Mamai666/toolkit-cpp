#include "WatchDogWorker.h"

void WatchDogWorker::setItWorks(bool *newItWorks)
{
    p_isWorking = newItWorks;
}

void WatchDogWorker::updateTime()
{
    m_lastUpdate = std::chrono::steady_clock::now();
}

int WatchDogWorker::elapsed()
{
    using std::chrono::milliseconds;
    using std::chrono::duration_cast;
    std::chrono::steady_clock::time_point t = m_lastUpdate;
    return static_cast<int>(duration_cast<milliseconds>(std::chrono::steady_clock::now() - t).count());
}

void WatchDogWorker::worker()
{
    m_localWorks = true;
    m_sendOne    = false;

    if(p_isWorking == nullptr)
        p_isWorking = &m_localWorks;

    while((*p_isWorking && m_localWorks) || m_sendOne)
    {
        switch(m_outputStatus)
        {
        case WatchDog::Statuses::Status_OK:
            p_Cli->createMsg("OK");
            break;
//        case WatchDog::Statuses::Status_Startup:
//            p_Cli->createMsg("Module Start");
//            break;
//        case WatchDog::Statuses::Status_Off:
//            p_Cli->createMsg("Module Stop");
//            break;
        case WatchDog::Statuses::Status_ErrStreamOpening:
            p_Cli->createMsg("ERROR: Stream Opening");
            break;
        case WatchDog::Statuses::Status_ErrtStreamInfo:
            p_Cli->createMsg("ERROR: Stream Info");
            break;
        case WatchDog::Statuses::Status_ErrStreamLookup:
            p_Cli->createMsg("ERROR: Stream Lookup");
            break;
        case WatchDog::Statuses::Status_ErrFrameReceiving:
            p_Cli->createMsg("ERROR: Frame Receiving");
            break;
        case WatchDog::Statuses::Status_ErrDiskWrite:
            p_Cli->createMsg("FATAL: Disk Write");
            break;
        case WatchDog::Statuses::Status_ErrFrameReceivingTimeout:
            p_Cli->createMsg("ERROR: Frame Receiving Timeout");
            break;
        case WatchDog::Statuses::Status_ErrFrameReceivingTimeoutFIFO:
            p_Cli->createMsg("FATAL: Frame Receiving Timeout");
            break;
        case WatchDog::Statuses::Status_ErrOutputReconnect:
            p_Cli->createMsg("FATAL: Output Reconnect");
            break;
        case WatchDog::Statuses::Status_ErrOutputHeaderSend:
            p_Cli->createMsg("FATAL: Output Header Send");
            break;
        case WatchDog::Statuses::Status_ErrOutputFrameSend:
            p_Cli->createMsg("FATAL: Output Frame Send");
            break;
        case WatchDog::Statuses::Status_ErrAsyncTimeOut:
            p_Cli->createMsg("ERROR: Async Timeout");
            break;
        case WatchDog::Statuses::Status_ErrBadFrameHeadLength:
            p_Cli->createMsg("FATAL: Invalid input frame header length");
            break;
//        case WatchDog::Statuses::Status_ErrAsyncTimeOut:
//            break;
        default: break;
        }

        m_sendOne = false; // Сообщение отправлено, сброс состояния
        bool retOK = p_Cli->sendMsg(m_lastError);
        if(retOK)
        {
            WatchDog::Statuses f = m_outputStatus;
            m_lastDebugInfo = "Сигнал диспетчеру "+std::to_string((uint32_t)f)+" послан!"
                              " , Задержка обновления: " + std::to_string(elapsed());

            //D_pLogDebug("-- Сигнал диспетчеру %u послан! (Задержка обновления %d)", (uint32_t)f, elapsed());
        }
        else
        {
            // Do something ..
        }

        if(m_delay % 100 == 0)
        {
            for(int i = 0; (i < 100) && *p_isWorking && m_localWorks; ++i)
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(m_delay/100));
            }
        }
        else
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(m_delay));
        }

        // Если не было обновлений больше двух интервалов, установить ошибку
        if(m_outputStatus != WatchDog::Statuses::Status_Off && (elapsed() > m_delay * 2))
        {
            m_outputStatus = WatchDog::Statuses::Status_ErrAsyncTimeOut;
            m_lastError = "Не было обновлений больше двух интервалов: Status_ErrAsyncTimeOut";
        }
    }

    fprintf(stderr, "Worker ended..\n");
}

WatchDogWorker::WatchDogWorker(IWatchDogClient *cli):
    m_lastUpdate(std::chrono::steady_clock::now())
{
    m_delay = 1000;
    m_outputStatus = WatchDog::Statuses::Status_Off;
    p_Cli = cli;
    if(p_Cli)
    {
        setDelay(p_Cli->wdSets().delaySendMessageMs);
        //LOG(INFO) << "Будет использоваться watchDog от " << m_cli->nameWatcher();
    }
}

WatchDogWorker::~WatchDogWorker()
{
}

void WatchDogWorker::setDelay(int ms)
{
    m_delay = ms;
}

void WatchDogWorker::notify(WatchDog::Statuses out)
{
    if(m_delay != p_Cli->wdSets().delaySendMessageMs)
        setDelay(p_Cli->wdSets().delaySendMessageMs);

    m_outputStatus = out;
    m_sendOne = true;
    updateTime();
}

bool WatchDogWorker::start()
{
    if(!p_Cli)
    {
        return false;
    }

    //D_pLogDebugNA("-- Запуск вещания диспетчеру");

    bool ret = p_Cli->open(m_lastError);
    updateTime();

    if(ret)
    {
        p_Cli->createMsg("Module Start");
        p_Cli->sendMsg(m_lastError);
        notify(WatchDog::Statuses::Status_OK);
        m_worker = std::thread(&WatchDogWorker::worker, this);
    }

    return ret;
}

bool WatchDogWorker::stop()
{
    if(!p_Cli)
    {
        return false;
    }

    // D_pLogDebugNA("-- Отключение вещания диспетчеру");

    //if(m_outputStatus != WatchDog::Statuses::Status_OK)
    m_localWorks = false;

    if(m_worker.joinable())
        m_worker.join();

    p_Cli->createMsg("Module Stop");
    bool retOK = p_Cli->sendMsg(m_lastError);
    if(retOK)
    {
        WatchDog::Statuses f = m_outputStatus;
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

    p_Cli->close(m_lastError);

    return retOK;
}

std::string WatchDogWorker::lastError(bool clean)
{
    std::string lastError = m_lastError;
    if(clean) m_lastError.clear();
    return lastError;
}

std::string WatchDogWorker::lastDebugInfo(bool clean)
{
    std::string lastDebugInfo = m_lastDebugInfo;
    if(clean) m_lastDebugInfo.clear();
    return lastDebugInfo;
}
