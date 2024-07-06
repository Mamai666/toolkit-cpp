#ifndef WATCHDOGWORKER_H
#define WATCHDOGWORKER_H

#include <atomic>
#include <thread>
#include <string>

#define use_WATCHDOG

#include "IWatchDogClient.h"

// Для обратной совместимости
namespace WatchDog
{
enum class Statuses
{
    // временно остановить в диспетчере контроль зацикливания
    Status_Off = 0,
    // статус успешного функционирования
    Status_OK = 1,
    // Модуль успешно запущен
    Status_Startup,

    // ошибка открытия входящего потока на старте (не удалось подключиться)
    Status_ErrStreamOpening,
    // ошибка поиска информации о потоке (поток не содержит видео, или содержит недействительные или неактуальные данные)
    Status_ErrtStreamInfo,
    // ошибка поиска потока (не удалось найти поток)
    Status_ErrStreamLookup,
    // ошибка получения кадра (видео закрылось на стороне сервера либо обрыв сети)
    Status_ErrFrameReceiving,
    // ошибка записи на диск (отвалилось что-то, отсутствуют права, диск переполнился)
    Status_ErrDiskWrite,
    // ошибка - долго нет кадра от захватчика
    Status_ErrFrameReceivingTimeout,
    // ошибка - долго нет кадра от захватчика
    Status_ErrFrameReceivingTimeoutFIFO,
    // ошибка переоткрытия исходящего канала
    Status_ErrOutputReconnect,
    // ошибка отправки исходящего заголовка
    Status_ErrOutputHeaderSend,
    // ошибка отправки исходящего кадра
    Status_ErrOutputFrameSend,
    // ошибка - долго не обновляется статус в асинхронной обёртке
    Status_ErrAsyncTimeOut,
    // ошибка - Неправильная длина заголовка входящего пакета
    Status_ErrBadFrameHeadLength,
};
}

/*!
 * \brief Асинхронная надстройка для периодического вещания сигнала диспетчеру
 */
class WatchDogWorker
{
    IWatchDogClient  *p_Cli = nullptr;
    bool             *p_isWorking = nullptr;

    bool                m_localWorks;
    //! Гарантия того, что сообщение дойдёт, даже если запрошено завершение
    std::atomic_bool    m_sendOne;
    std::atomic<WatchDog::Statuses> m_outputStatus;
    typedef std::chrono::nanoseconds TimeT;
    std::atomic<std::chrono::steady_clock::time_point> m_lastUpdate;
    std::thread         m_worker;
    std::atomic_int     m_delay;

    void updateTime();
    int  elapsed();

    void worker();

public:
    WatchDogWorker(IWatchDogClient *cli);
    ~WatchDogWorker();

    /*!
     * \brief Установить интервал отправки статуса диспетчеру
     * \param ms Время в миллисекундах
     */
    void setDelay(int ms);

    /*!
     * \brief Записать статус на отправку
     * \param out Значение статуса
     */
    void notify(WatchDog::Statuses out = WatchDog::Statuses::Status_OK);

    /*!
     * \brief Запустить вещание статуса
     * \return Успешный запуск, или не завелось
     */
    bool start();

    /*!
     * \brief Остановить вещание статуса
     */
    bool stop();

    std::string lastError(bool clean = false);
    std::string lastDebugInfo(bool clean = false);

    void setItWorks(bool *newItWorks);

private:
    std::string m_lastError;
    std::string m_lastDebugInfo;
};

#endif // WATCHDOGWORKER_H
