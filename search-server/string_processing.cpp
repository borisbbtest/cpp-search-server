#include "string_processing.h"

std::vector<std::string_view> SplitIntoWords(std::string_view str)
{
    std::vector<std::string_view> result;
    while (true)
    {
        uint64_t space = str.find(' ');
        if (str[0] != ' ')
        {
            result.push_back(str.substr(0, space));
        }
        if (space == str.npos)
        {
            break;
        }
        else
        {
            str.remove_prefix(space + 1);
        }
    }
    return result;
}