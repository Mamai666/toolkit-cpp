/*  u-checkloop-data.hpp

    Copyright (c) Glosav 2021
    Author: Kachkovsky Sergey V.
    kasv@list.ru

    Класс для передачи событий от модуля к диспетчеру
*/


#ifndef _U_CheckLoop_Data_HPP
#define _U_CheckLoop_Data_HPP


// имя файла shared memory
#define gtpCheckLoop_Filename    "/dispatcher_checkloop.shm"

// количество элементов в массиве индексов
#define gtpCheckLoop_countMaxPidAndTime     (4 * 1024)


// структура shared memory
class gtpDispatcherShm {
    public:
        long long int       m_Time;

        typedef struct {
            int             m_Pid;
            int             m_Status;
            long long int   m_Time;
        } PidAndTime;

        // массив индексов - pid & время в ms
        PidAndTime mas[gtpCheckLoop_countMaxPidAndTime];
};



#endif
