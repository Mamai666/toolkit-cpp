#ifndef CHECKLOOP_WORKER_HHH
#define CHECKLOOP_WORKER_HHH

#include <atomic>
#include <thread>
#include <u-checkloop-client.hpp>

/*!
 * \brief Асинхронная надстройка для периодического вещания сигнала диспетчеру
 */
class GtpCheckloopWorker
{
    gtpCheckLoopClient  m_cli;
    std::atomic_bool    m_itWorks;
    std::atomic<gtpCheckLoopClient::Statuses> m_outputStatus;
    typedef std::chrono::nanoseconds TimeT;
    std::atomic<std::chrono::steady_clock::time_point> m_lastUpdate;
    std::thread         m_worker;
    std::atomic_int     m_delay;

    void updateTime();
    int  elapsed();

    void worker();

public:
    GtpCheckloopWorker();
    ~GtpCheckloopWorker();

    /*!
     * \brief Установить интервал отправки статуса диспетчеру
     * \param ms Время в миллисекундах
     */
    void setDelay(int ms);

    /*!
     * \brief Записать статус на отправку
     * \param out Значение статуса
     */
    void notify(gtpCheckLoopClient::Statuses out = gtpCheckLoopClient::Statuses::Status_OK);

    /*!
     * \brief Запустить вещание статуса
     * \return Успешный запуск, или не завелось
     */
    bool start();

    /*!
     * \brief Остановить вещание статуса
     */
    void stop();
};

#endif // CHECKLOOP_WORKER_HHH
