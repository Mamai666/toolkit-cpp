#ifndef FILEMONITOR_H
#define FILEMONITOR_H

#include <functional>
#include <sys/stat.h>
#include <unistd.h>
#include <string>

class FileMonitor
{
public:
    FileMonitor(const std::string &pathToFile);
    virtual ~FileMonitor();

    bool checkFileChange(std::string &newFileDump, struct stat &newFileStat);
    void rewriteLastDump(std::string &newFileDump, struct stat &newFileStat);

    std::string pathToFile() const;

    std::string lastError() const;

protected:
    struct stat m_lastFileStat;
    std::string m_lastFileDump;    // Последний сохраненный дамп файла

private:
    std::string m_pathToFile;
    std::string m_lastError;
};

#endif // FILEMONITOR_H
