/*  u-checkloop-client.cpp

    Copyright (c) Glosav 2021
    Author: Kachkovsky Sergey V.
    kasv@list.ru

    Классы для передачи контрольных точек в диспетчер
*/


#include <u-checkloop-client.hpp>
#include <u-checkloop-data.hpp>
#include <time.h>
#include <fcntl.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>



#if _WIN32 == 1
    #include <io.h>
    #include <windows.h>
    #include <process.h>

    int shm_open(const char* name, int oflag, int mode);
    int shm_unlink(const char* name);

    void* mmap(void* start, size_t length, int prot, int flags, int fd, off_t offset);
    int munmap(void* start, size_t length);
    #define unlink _unlink
    #define close _close
    #define PROT_READ   0
    #define PROT_WRITE   0
    #define MAP_SHARED   0
    #define getpid  _getpid
#else
    #include <sys/time.h>
    #include <sys/mman.h>
    #include <unistd.h>
#endif 



// ------------------------------------------------------------------------------------------------
static class gtpDispatcherCheckLoop {
    public:
        ~gtpDispatcherCheckLoop(void);

        // подключиться к диспетчеру
        bool    Enable(void);

        // отключиться от диспетчера
        void    Disable(void);

        // учесть очередную итерацию цикла - записать событие и его время в shm
        void    Notify(gtpCheckLoopClient::Statuses status);


    private:
        // shared memory dispatcher, если не NULL, то подключена
        gtpDispatcherShm* m_DispatcherShm = nullptr;

        // файловый дескриптор общей памяти
        int m_shm_fd = -1;

        // pid текущего процесса
        int m_pid = 0;

        // индекс текущего процесса в shm
        int m_shm_idx = -1;

        // очистить данные процесса перед завершением
        void    Clear(void);
} s_CheckLoop;



// ------------------------------------------------------------------------------------------------
gtpDispatcherCheckLoop::~gtpDispatcherCheckLoop(void)
{
    Disable();
}


// ------------------------------------------------------------------------------------------------
// подключиться к расширенной памяти диспетчера
bool    gtpDispatcherCheckLoop::Enable(void)
{
    m_pid = getpid();
    if (m_pid <= 0) {
        // при нормально работе этой ошибки быть не должно
        return 0;
    }

    if (m_DispatcherShm) {
        // shared memory уже открыта
        return true;
    }

    m_shm_fd = shm_open(gtpCheckLoop_Filename, O_RDWR, 0);
    if (m_shm_fd < 0) {
        // Ошибка открытия shared memory: shm_open()
        return 0;
    }

    struct stat st;
    if (fstat(m_shm_fd, &st)) {
        // Ошибка получения размера shared memory: fstat()
        return 0;
    }

    if (st.st_size != sizeof(gtpDispatcherShm)) {
        //Ошибка - не совпадает размер shared memory - скорее всего не соответствует версия структуры gtpDispatcherShm
        Disable();
        return 0;
    }


    m_DispatcherShm = (gtpDispatcherShm*)mmap(
        NULL,                      // подсказка
        sizeof(gtpDispatcherShm),  // segment size,
        PROT_READ | PROT_WRITE,    // protection flags
        MAP_SHARED,                // sharing flags
        m_shm_fd,                  // handle to map object
        0);                        // offset

    if (!m_DispatcherShm) {
        // Ошибка подключения к shared memory: mmap()
        Disable();
        return 0;
    }

    // найти свой индекс
    for (int i = 0; i < gtpCheckLoop_countMaxPidAndTime; i++) {
        if (m_DispatcherShm->mas[i].m_Pid == m_pid) {
            m_shm_idx = i;
            Notify(gtpCheckLoopClient::Statuses::Status_Off);
            return 1;
        }
    }

    // Не найден свой pid - нельзя использовать этот функционал
    return 0;
}


// ------------------------------------------------------------------------------------------------
// отключиться от диспетчера
void    gtpDispatcherCheckLoop::Disable(void)
{
    if (m_DispatcherShm) {
        // очистить данные процесса перед завершением
        Clear();

        munmap(m_DispatcherShm, sizeof(gtpDispatcherShm));
        m_DispatcherShm = nullptr;
    }

    if (m_shm_fd >= 0) {
        close(m_shm_fd);
        m_shm_fd = -1;
    }

    m_shm_idx = -1;
}


// ------------------------------------------------------------------------------------------------
// учесть очередную итерацию цикла - записать событие и его время в shm
void    gtpDispatcherCheckLoop::Notify(gtpCheckLoopClient::Statuses status)
{
    if (m_shm_idx>=0 && 
        m_shm_idx < gtpCheckLoop_countMaxPidAndTime &&
        m_DispatcherShm && 
        m_pid > 0 && 
        m_DispatcherShm->mas[m_shm_idx].m_Pid == m_pid) {
            // записать статус и время модуля
            m_DispatcherShm->mas[m_shm_idx].m_Status = (int)status;
            m_DispatcherShm->mas[m_shm_idx].m_Time = m_DispatcherShm->m_Time;
    }
}


// ------------------------------------------------------------------------------------------------
// очистить данные процесса перед завершением
void    gtpDispatcherCheckLoop::Clear(void)
{
    if (m_shm_idx >= 0 &&
        m_shm_idx < gtpCheckLoop_countMaxPidAndTime &&
        m_DispatcherShm &&
        m_pid > 0 &&
        m_DispatcherShm->mas[m_shm_idx].m_Pid == m_pid) {
            m_DispatcherShm->mas[m_shm_idx].m_Status = 0;
            m_DispatcherShm->mas[m_shm_idx].m_Time = 0;
            m_DispatcherShm->mas[m_shm_idx].m_Pid = 0;
    }
}



// ************************************************************************************************
// подключиться к диспетчеру
bool    gtpCheckLoopClient::Open(void)
{
    return s_CheckLoop.Enable();
}

// ------------------------------------------------------------------------------------------------
// отключиться от диспетчера
void    gtpCheckLoopClient::Close(void)
{
    s_CheckLoop.Disable();
}

// ------------------------------------------------------------------------------------------------
// передать код события в диспетчер
void    gtpCheckLoopClient::Notify(Statuses StatusCode)
{
    s_CheckLoop.Notify(StatusCode);
}
