#include "common.h"
#include <algorithm>


std::string Common::strip_quotes(const std::string& str)
{
    std::string result = str;
    result.erase(std::remove(result.begin(), result.end(), '"'), result.end());
    return result;
}
