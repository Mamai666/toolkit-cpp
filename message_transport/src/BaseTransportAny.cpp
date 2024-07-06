#include "BaseTransportAny.h"

BaseTransportAny::BaseTransportAny(std::string credentials, std::string instanceName, std::initializer_list<void *> *other)
{
    m_credentials = credentials;
    m_localIsWork = true;
}

BaseTransportAny::~BaseTransportAny()
{

}

void BaseTransportAny::setP_GlobalIsWork(bool *globalIsWork)
{
    p_GlobalIsWork = globalIsWork;
}

void BaseTransportAny::setIsKeepConnect(bool newIsKeepConnect)
{
    m_isKeepConnect = newIsKeepConnect;
}

TransportState BaseTransportAny::getStatus()
{
    if(elapsed() > m_delayStatus*2)
    {
        m_currStatus.status = TransportState::STATUS_ERROR_UPDATE;
    }
    return m_currStatus.status;
}

std::string BaseTransportAny::credentials()
{
    return m_credentials;
}

bool BaseTransportAny::localIsWork() const
{
    return m_localIsWork && (p_GlobalIsWork == nullptr ? 1 : *p_GlobalIsWork);
}

void BaseTransportAny::setLocalIsWork(bool newLocalIsWork)
{
    m_localIsWork = newLocalIsWork;
}

uint32_t BaseTransportAny::delayMks() const
{
    return m_delayMks;
}

// void BaseTransportAny::setDelayMks(uint32_t newCDelayMks)
// {
//     m_delayMks = newCDelayMks;
// }

uint32_t BaseTransportAny::waitOpenMks() const
{
    return m_waitOpenMks;
}

void BaseTransportAny::setWaitOpenMks(uint32_t newWaitOpenMks)
{
    m_waitOpenMks = newWaitOpenMks;
}

void BaseTransportAny::changeStatus(TransportState newCurrStatus)
{
    m_currStatus.status = newCurrStatus;
    updateStatus();
}

void BaseTransportAny::updateStatus()
{
    m_currStatus.updateTime.store(std::chrono::steady_clock::now());
}

int BaseTransportAny::elapsed()
{
    using std::chrono::milliseconds;
    using std::chrono::duration_cast;
    TimePoint t = m_currStatus.updateTime.load();
    return static_cast<int>(duration_cast<milliseconds>(std::chrono::steady_clock::now() - t).count());
}
