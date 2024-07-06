#include "Utils/FileMonitor.h"
#include "Utils/files.h"
#include <cstring>

FileMonitor::FileMonitor(const std::string &pathToFile)
{
    m_pathToFile = pathToFile;
    memset(&m_lastFileStat, 0, sizeof(m_lastFileStat));
}

FileMonitor::~FileMonitor()
{

}

void FileMonitor::rewriteLastDump(std::string &newFileDump, struct stat &newFileStat)
{
    m_lastFileDump = newFileDump;
    std::memcpy(&m_lastFileStat, &newFileStat, sizeof(struct stat));
}

std::string FileMonitor::pathToFile() const
{
    return m_pathToFile;
}

std::string FileMonitor::lastError() const
{
    return m_lastError;
}

bool FileMonitor::checkFileChange(std::string &newFileDump, struct stat &newFileStat)
{
    // Получаем состояние файла
    if(stat(m_pathToFile.c_str(), &newFileStat) < 0)
    {
        m_lastError = "Невозможно проверить файл "+m_pathToFile+
                      ", ошибка доступа или файл не существует";
        return false;
    }

    if(m_lastFileStat.st_dev != 0) // Ранее файл был открыт
    {
        // Сверяем дату изменения и размер файла
        if(newFileStat.st_mtim.tv_nsec == m_lastFileStat.st_mtim.tv_nsec &&
            newFileStat.st_size         == m_lastFileStat.st_size)
        {
            return false; // Файлы равны
        }
    }

    // Считываем всё содержимое файла
    if(!Files::dumpFile(m_pathToFile, newFileDump))
    {
        m_lastError = "Не удалось взять дамп от файла: "+m_pathToFile;
        return false;
    }

    if(m_lastFileStat.st_dev != 0) // Ранее файл был открыт
    {
        // Сверяем содержимое файла с последним дампом
        if(m_lastFileDump == newFileDump)
        {
            //LOG(DEBUG) << "Файлы разные по времени, но равны по содержимому!";
            rewriteLastDump(newFileDump, newFileStat);
            return false; // Содержимое файлов равно
        }
    }

    return true;
}
