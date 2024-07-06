/*  u-checkloop-client.hpp

    Copyright (c) Glosav 2021
    Author: Kachkovsky Sergey V.
    kasv@list.ru

    Клиентский класс для работы с диспетчером с целью передачи статуса и контроля зацикливания модуля.
    Не создаются потоки
    Не используются мьютексы
    Не используются исключения
    Высокое быстродействие при передаче статуса
*/


#ifndef _U_CheckLoop_CLIENT_HPP
#define _U_CheckLoop_CLIENT_HPP


class gtpCheckLoopClient {
    public:
        // подключиться к диспетчеру - использовать один раз в начале работы программы, относительно затратная операция
        bool    Open(void);

        // отключиться от диспетчера - после отключения от диспетчера повторное подключение невозможно, отключаться перед завершением программы
        void    Close(void);

        // возможные статусы модуля для передачи в диспетчер
        enum class Statuses {
            // временно остановить в диспетчере контроль зацикливания
            Status_Off = 0,
            // статус успешного функционирования
            Status_OK = 1,

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
            // ошибка переоткрытия исходящего канала
            Status_ErrOutputReconnect,
            // ошибка отправки исходящего заголовка
            Status_ErrOutputHeaderSend,
            // ошибка отправки исходящего кадра
            Status_ErrOutputFrameSend,
            // ошибка - долго не обновляется статус в асинхронной обёртке
            Status_ErrAsyncTimeOut,
        };

        // передать статус в диспетчер. По-умолчанию передается Status_OK
        void    Notify(Statuses StatusCode = Statuses::Status_OK);
};


#endif




/* --- Example ---


#include <u-checkloop-client.hpp>
#include <unistd.h>
#include <stdio.h>


bool enaWork = true;


int main()
{
    gtpCheckLoopClient dispatcherStatus;

    if (!dispatcherStatus.Open()) {
        fprintf(stderr, "Не удалось подключиться к диспетчеру");
    }

    while (enaWork) {
        dispatcherStatus.Notify();
        sleep(1);
    }
    dispatcherStatus.Close();
}


*/

