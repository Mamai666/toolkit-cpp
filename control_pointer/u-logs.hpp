/*  u-logs.hpp

    Copyright (c) Glosav 2021
    Author: Kachkovsky Sergey V.
    kasv@list.ru

    Запись в лог
*/


#ifndef _U_LOGS_HPP
#define _U_LOGS_HPP


// Задать путь, где создавать лог ошибок (по умолчанию будет использован текущий путь)
void    logSetDir(const char* logDir = nullptr, int days = 10);
void    logSetScriptName(const char* name);

// Удаляет старые логи
void    logClean(void);

// Управление выдачей ошибок в stdout
void    logStderr(bool enaStderr);

// Добавляет запись в лог ошибок
void    logError(const char* file, int line, const char* fmt, ...);
void    logSignal(const char* file, int line, int sig, bool wrDbgPoints, const char* fmt, ...);
void    logSignalBlk(void);
void    logException(const char* file, int line, const char* fmt, ...);



// Создать контрольную точку
int    logDbgPoint_New(const char* loopName, int idx = -1);

// Удалить контрольную точку
void    logDbgPoint_Delete(int idx);

// Задать контрольную точку (например, перед вызовом функции или метода)
void    logDbgPoint_Set(int idx, const char* file, int line);

// Очистить контрольную точку (например, когда поток переходит в ожидание)
void    logDbgPoint_Clear(int idx);



// Добавляет запись в лог отладки
void    logDebug(const char* file, int line, const char* fmt, ...);



// Краткая форма для записи в лог ошибок
#define LOG_ERR(...)    logError(__FILE__, __LINE__, __VA_ARGS__)

// Краткая форма для регистрации исключений
#define LOG_EXEPTION(...)    logException(__FILE__, __LINE__, __VA_ARGS__)

// Краткая форма для регистрации исключений
#define LOG_SIGNAL(sig, m, ...)    logSignal(__FILE__, __LINE__, sig, m, __VA_ARGS__)

// Краткая запись для формирования контрольной точки
#define dbg(i)           logDbgPoint_Set(i, __FILE__,__LINE__)
// Краткая запись для очистки контрольной точки
#define dbgClear(i)      logDbgPoint_Clear(i) 

// Краткая форма для записи в лог отладки
#define LOG_DBG(e, ...) if (e) logDebug(__FILE__, __LINE__, __VA_ARGS__)



#endif
