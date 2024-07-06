/*  u-signals.cpp

    Copyright (c) Glosav 2021
    Author: Kachkovsky Sergey V.
    kasv@list.ru

    Обработка сигналов.
*/


#include <u-signals.hpp>
#include <u-logs.hpp>
#include <signal.h>
#include <stdlib.h>


#ifdef _WIN32
    static char* strsignal(int sig) { return (char*)""; }
    static int wait(int* status) { return 0; }
    static int waitpid(int pid, int* status, int options) { return 0; }
    #define WNOHANG	1 
#else
    #include <sys/wait.h>
#endif




bool    enaWork = 1;

static bool g_SignalFatal = false;


static void	onSignalFatal(int sig)
{
    if (!g_SignalFatal) {
        g_SignalFatal = true;
        enaWork = false;

        LOG_SIGNAL(sig, 1, "Error! Signal fatal! Вызов abort()");
        abort();
    }
}

static void	onSignalAbort(int sig)
{
    if (!g_SignalFatal) {
        g_SignalFatal = 1;

        LOG_SIGNAL(sig, 1, "Error! Signal Abort!");
    }
}


static void	onSignalQuit(int sig)
{
    signal(sig, SIG_IGN);

    if (!enaWork) {
        LOG_SIGNAL(sig, 1, "Debug! Signal quit! Вызов exit()");
        exit(0);
    }
    enaWork = false;

    LOG_SIGNAL(sig, 0, "Debug! Signal quit! Присвоено enaWork = false для завершения программы");
    signal(sig, onSignalQuit);
}

static void	onSignalIgnore(int sig)
{
    signal(sig, SIG_IGN);

    LOG_SIGNAL(sig, 0, "Debug! Signal ignore");
    signal(sig, onSignalIgnore);
}

static void     onSignalReapChild(int sig)
{
    int wait_status;
    int pid;
    while ((pid = waitpid(-1, &wait_status, WNOHANG)) > 0) {
        endProcess(pid);
        LOG_SIGNAL(sig, 0, "Debug! Signal Reap Child! Завершен процесс pid = %d;", pid);
    }

    signal(sig, onSignalReapChild);
}



void    setSignals(bool enaSIGHUP)
{
    //----------------------------------------------
    // signals fatal
    //----------------------------------------------
    // (A) Завершение с дампом памяти. Недопустимая инструкция процессора
    signal(SIGILL, onSignalFatal);
    // (A) Завершение с дампом памяти. Ошибочная арифметическая операция
    signal(SIGFPE, onSignalFatal);
    // (A) Завершение с дампом памяти. Нарушение при обращении в память
    signal(SIGSEGV, onSignalFatal);
#ifndef _WIN32
    // (A) Завершение с дампом памяти. Неправильное обращение в физическую память
    signal(SIGBUS, onSignalFatal);
    // (A) Завершение с дампом памяти. Ловушка трассировки или брейкпоинт
    signal(SIGTRAP, onSignalFatal);
    // Перехват ввода-вывода
    signal(SIGIOT, onSignalFatal);
#ifdef SIGEMT		// this is not defined under Linux
    // Перехват эмуляции
    signal(SIGEMT, onSignalFatal);
#endif
#ifdef SIGSTKFLT	// is not defined in BSD, OSX
    // Нарушение стека сопроцессора
    signal(SIGSTKFLT, onSignalFatal);
#endif
#ifdef SIGPWR		// is not defined in BSD, OSX
    // Перезапуск после проблемы с питанием
    signal(SIGPWR, onSignalFatal);
#endif
    // (A) Bad system call
    signal(SIGSYS, onSignalFatal);
#endif

    //----------------------------------------------
    // signals quit
    //----------------------------------------------
    // (T) Завершение. Сигнал прерывания (Ctrl-C) с терминала
    signal(SIGINT, onSignalQuit);
    // (T) Завершение. Сигнал завершения (сигнал по умолчанию для утилиты kill)
    signal(SIGTERM, onSignalQuit);
    // (A) Завершение с дампом памяти. Сигнал посылаемый функцией abort()
    signal(SIGABRT, onSignalAbort);
#ifndef _WIN32
    // (T) Завершение. Посылается процессу для уведомления о потере соединения с управляющим терминалом пользователя
    signal(SIGHUP, enaSIGHUP ? onSignalQuit : onSignalIgnore);
    // (A) Завершение с дампом памяти. Сигнал «Quit» с терминала (Ctrl-\)
    signal(SIGQUIT, onSignalQuit);

    // (T) Завершение. Запись в разорванное соединение (пайп, сокет)
    signal(SIGPIPE, onSignalIgnore);
#endif

    //----------------------------------------------
    // signals ignore
    //----------------------------------------------
#ifndef _WIN32
    // (T) Завершение. Пользовательский сигнал № 1
    signal(SIGUSR1, onSignalIgnore);
    // (T) Завершение. Пользовательский сигнал № 2
    signal(SIGUSR2, onSignalIgnore);
    // (T) Завершение. Сигнал истечения времени, заданного alarm()
    signal(SIGALRM, onSignalIgnore);
    // (S) Остановка процесса. Сигнал остановки с терминала (Ctrl-Z)
    signal(SIGTSTP, onSignalIgnore);
    // (S) Остановка процесса. Попытка чтения с терминала фоновым процессом
    signal(SIGTTIN, onSignalIgnore);
    // (S) Остановка процесса. Попытка записи на терминал фоновым процессом
    signal(SIGTTOU, onSignalIgnore);
    // (I) Игнорируется. На сокете получены срочные данные
    signal(SIGURG, onSignalIgnore);
    // (A) Завершение с дампом памяти. Процесс превысил лимит процессорного времени
    signal(SIGXCPU, onSignalIgnore);
    // (A) Завершение с дампом памяти. Процесс превысил допустимый размер файла
    signal(SIGXFSZ, onSignalIgnore);
    // (T) Завершение. Истечение «виртуального таймера»
    signal(SIGVTALRM, onSignalIgnore);
    // (T) Завершение. Истечение таймера профилирования
    signal(SIGPROF, onSignalIgnore);
    // Этот сигнал отправляется, когда файловый дескриптор готов выполнить ввод или вывод
    signal(SIGIO, onSignalIgnore);
    // (C) Продолжить выполнение. Продолжить выполнение ранее остановленного процесса
    signal(SIGCONT, onSignalIgnore);

    // (I) Игнорируется. Посылается при изменении статуса дочернего процесса (завершён, приостановлен или возобновлен)
    signal(SIGCHLD, onSignalReapChild);
#endif
}
