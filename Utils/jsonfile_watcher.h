#include <sys/types.h>
#include <sys/inotify.h>
#include <sys/stat.h>
#include <unistd.h>
#include <thread>
#include <string>
#include <atomic>
#include <cstdio>
#include <functional>

#include "json.hpp"
//#include "logger/logger.h"
#define msleep(X) usleep((X)*1000)
#ifndef LOGGER_H
#define pLogWarning printf
#define pLogDebug printf
#define D_pLogDebugNA printf
#endif

struct FileMonitor
{
    struct stat lastStat;
    std::function<void(void)> callback;
    std::function<void(const nlohmann::json &)> callbackArg;
    std::string filePath;
    std::string lastFileDump;
    nlohmann::json lastState;
};

static void dumpFile(const std::string &filePath, std::string &out)
{
    FILE *f;
    off_t end;

    out.clear();

    f = std::fopen(filePath.c_str(), "r");
    if(!f)
        return;

    std::fseek(f, 0, SEEK_END);
    end = std::ftell(f);
    std::fseek(f, 0, SEEK_SET);
    out.resize(end);
    std::fread((void*)out.data(), 1, end, f);
    std::fclose(f);
}

static void monitorRunCheck(FileMonitor &mon)
{
    struct stat st;
    std::string dump;
    nlohmann::json f_new;

    // Получаем состояние файла
    if(stat(mon.filePath.c_str(), &st) < 0)
    {
        pLogWarning("Невозможно проверить файл %s, ошибка доступа или файл не существует", mon.filePath.c_str());
        return;
    }

    // Сверяем дату изменения и размер файла
    if(st.st_mtim.tv_nsec == mon.lastStat.st_mtim.tv_nsec && st.st_size == mon.lastStat.st_size)
    {
        // D_pLogDebugNA("Файлы равны");
        return; // Файлы равны
    }

    // Считываем всё содержимое файла
    dumpFile(mon.filePath, dump);

    // Сверяем содержимое файла с последним дампом
    if(mon.lastFileDump == dump)
    {
        D_pLogDebugNA("Файлы равны по содержимому");
        mon.lastFileDump = dump;
        std::memcpy(&mon.lastStat, &st, sizeof(struct stat));
        return; // Содержимое файлов равно
    }

    // Попытка прочитать JSON-содержимое
    try {
        f_new = nlohmann::json::parse(dump);
    }
    catch(const std::exception &e)
    {
        pLogWarning("Содержимое файла %s не является корректным JSON-блоком: %s",
                    mon.filePath.c_str(), e.what());
        return;
    }

    // Сравнение JSON-дерева с предыдущем
    if(mon.lastState == f_new)
    {
        D_pLogDebugNA("Файлы равны по JSON-содержимому");
        mon.lastFileDump = dump;
        mon.lastState = f_new;
        std::memcpy(&mon.lastStat, &st, sizeof(struct stat));
        return;
    }

    // Файл считается изменённым лишь тогда, когда содержимое JSON-деревьев различное

    pLogDebug("Обнаружено обновление файла %s", mon.filePath.c_str());
    if(mon.callback)
        mon.callback();
    if(mon.callbackArg)
        mon.callbackArg(f_new);
    mon.lastFileDump = dump;
    mon.lastState = f_new;
    std::memcpy(&mon.lastStat, &st, sizeof(struct stat));
}

struct JsonFileWatcher
{
    std::atomic_bool isWorks;
    FileMonitor mon;
    std::thread worker;

    uint sleepMs = 1000;

    void run()
    {
        while(isWorks)
        {
            monitorRunCheck(mon);
            msleep(sleepMs);
        }
    }

    void startMonitor(const std::string &filePath, std::function<void(const nlohmann::json &)> callback, uint timeoutMs = 1000)
    {
        sleepMs = timeoutMs;
        mon.filePath = filePath;

        if(stat(mon.filePath.c_str(), &mon.lastStat) < 0)
        {
            pLogWarning("Невозможно проверить файл %s, ошибка доступа или файл не существует", mon.filePath.c_str());
            return;
        }

        dumpFile(filePath, mon.lastFileDump);

        try {
            mon.lastState = nlohmann::json::parse(mon.lastFileDump);
        }
        catch(const std::exception &e)
        {
            pLogWarning("Содержимое файла %s не является корректным JSON-блоком: %s",
                        mon.filePath.c_str(), e.what());
            return;
        }

        isWorks = true;
        mon.callbackArg = callback;

        worker = std::thread(&JsonFileWatcher::run, this);
    }

    void stopMonitor()
    {
        if(!isWorks)
            return;
        isWorks = false;
        if(worker.joinable())
            worker.join();
    }

    ~JsonFileWatcher()
    {
        stopMonitor();
    }
};
