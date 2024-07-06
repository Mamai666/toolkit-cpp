#ifndef MSTRINGS_H
#define MSTRINGS_H

#include <string>
#include <vector>

namespace Strings
{
    typedef std::vector<std::string> List;
    bool endsWith(const std::string &str, char what);
    bool endsWith(const std::string &str, const std::string &what);
    bool startsWith(const std::string &str, char what);
    bool startsWith(const std::string &str, const std::string &what);
    std::string trim(std::string str);
    void doTrim(std::string &str);

    void c_split(Strings::List& out, const char* c_str, char delimiter);
    List c_split(const char* c_str, char delimiter);

    std::vector<std::string> stdv_split(const std::string& input, const std::string& regex);

    bool isContainNonEmptyString(const std::vector<std::string> &vec);

    std::string toLower(std::string source);
    std::string toUpper(std::string source);
    void chop(std::string &str, size_t numChars);
    std::string join(const std::vector<std::string>& strings, const std::string& delimiter);

    void split(List &out, const std::string &str, char delimiter);
    void split(List &out, const std::string &str, const std::string &delimiter);
    List split(const std::string &str, char delimiter);
    List split(const std::string &str, const std::string &delimiter);
}

#endif // MSTRINGS_H
