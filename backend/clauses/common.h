#ifndef COMMON_H
#define COMMON_H

#include <string>
#include <vector>

class Common
{
public:
    static std::string strip_quotes(const std::string &str);
    struct comparision_result
    {
        bool equal;
        std::vector<std::string> correct_parts;
        std::vector<std::string> incorrect_parts;
        std::vector<std::string> next_steps;
        std::string message;
    };
};

#endif // COMMON_H