#pragma once
#include <vector>
#include <string>
#include <set>

std::vector<std::string_view> SplitIntoWords(std::string_view str);

template <typename StringContainer>
inline std::set<std::string, std::less<>> MakeUniqueNonEmptyStrings(const StringContainer &strings)
{
    std::set<std::string, std::less<>> non_empty_strings;
    for (std::string_view str : strings)
    {
        if (!str.empty())
        {
            non_empty_strings.emplace(str);
        }
    }
    return non_empty_strings;
}
