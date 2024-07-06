#include "Utils/mstrings.h"
#include <cstddef>
#include <cstdint>
#include <chrono>
#include <cstdio>
#include <sys/stat.h>

#include <Utils/files.h>
#include <DirManager/dirman.h>
#include <iostream>

constexpr std::chrono::nanoseconds timespecToDuration(timespec ts)
{
    auto duration = std::chrono::seconds{ts.tv_sec}
                    + std::chrono::nanoseconds{ts.tv_nsec};

    return std::chrono::duration_cast<std::chrono::nanoseconds>(duration);
}

constexpr std::chrono::time_point<std::chrono::system_clock, std::chrono::nanoseconds>
timespecToTimePoint(timespec ts)
{
    return std::chrono::time_point<std::chrono::system_clock, std::chrono::nanoseconds>{
            std::chrono::duration_cast<std::chrono::system_clock::duration>(timespecToDuration(ts))};
}

namespace fs {

using file_time_type = std::chrono::time_point<std::chrono::system_clock>;

class path {
public:
    path(const char* path)
    {
        DirMan d(path);
        m_absolutePath = d.absolutePath();
    }

    path(std::string path)
    {
        DirMan d(path);
        m_absolutePath = d.absolutePath();
    }

    ~path()
    {

    }

    path& operator=(const path& p)
    {
        this->m_absolutePath = p.m_absolutePath;
        return *this;
    }

    std::string filename() const
    {
        std::string filename = m_absolutePath;
        if(m_absolutePath.find("/") != std::string::npos)
        {
            filename = Strings::stdv_split(m_absolutePath, "/").back();
        }
        return filename;
    }

//    std::string parentPath()
//    {

//    }

    std::string absolutePath() const
    {
        return m_absolutePath;
    }

    std::string extension() const
    {
        auto splitFileName = Strings::stdv_split(m_absolutePath, ".");
        if(splitFileName.size() < 2)
        {
            return "" ;
        }
        return "."+splitFileName.back();
    }

private:
    std::string m_absolutePath;

};

bool is_regular_file(const path& p)
{
    return !Files::isSymLink(p.absolutePath());
}

file_time_type last_write_time(const path& p)
{
    struct stat fileStat;
    if(stat(p.absolutePath().c_str(), &fileStat) < 0)
    {
        perror(("ошибка доступа к файлу: "+p.absolutePath()).c_str());
    }

    return timespecToTimePoint(fileStat.st_mtim);
}

std::vector<fs::path> recursive_directory_iterator(const fs::path& dir)
{
    std::vector<std::string> tmpPathsList;

    DirMan d(dir.absolutePath());
    d.getListOfFiles(tmpPathsList);

    std::vector<fs::path> resPaths;
    for(auto &s : tmpPathsList)
    {
        resPaths.push_back(fs::path(dir.absolutePath()+"/"+s));
    }
    return resPaths;
}

bool remove(const fs::path& p)
{
    bool retOK = false;
    if(p.extension().empty()) // It is Dir, NOT FILE
    {
        retOK = DirMan::rmAbsDir(p.absolutePath());
    }
    else
    {
        retOK = Files::deleteFile(p.absolutePath());
    }
    return retOK;
}
}
