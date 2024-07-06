/*  u-signals.hpp

    Copyright (c) Glosav 2021
    Author: Kachkovsky Sergey V.
    kasv@list.ru

    Обработка сигналов.
*/


#ifndef _U_SIGNALS_HPP
#define _U_SIGNALS_HPP


extern bool    enaWork;
void    setSignals(bool enaSIGHUP = false);


// Обработчик завершения процесса - вызывается при получении сигнала SIGCHLD (завершение процесса)
// Важно! Эта функция должна быть реализована в программе, использующей setSignals()
extern void    endProcess(int pid);



#endif
