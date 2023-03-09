// файл wordstat.cpp

#include "log_duration.h"

#include <algorithm>
#include <iostream>
#include <iterator>
#include <map>
#include <unordered_map>
#include <string>
#include <tuple>
#include <vector>

using namespace std;

vector<pair<string, int>> GetSortedWordCounts(vector<string> words)
{
    unordered_map<string, int> counts_map;

    {
        LOG_DURATION("Filling"s);

        for (auto &word : words)
        {
            ++counts_map[move(word)];
        }
    }

    cout << "Содержимое counts_map:"s << endl;
    for (auto [iter, i] = tuple(counts_map.begin(), 0); i < 10 && iter != counts_map.end(); ++i, ++iter)
    {
        cout << iter->first << " - "s << iter->second << endl;
    }
    {
        LOG_DURATION("Moving & sorting"s);

        vector<pair<string, int>> counts_vector(move_iterator(counts_map.begin()), move_iterator(counts_map.end()));
        sort(counts_vector.begin(), counts_vector.end(), [](const auto &l, const auto &r)
             { return l.second > r.second; });

        return counts_vector;
    }
}

int main()
{
    vector<string> words;
    string word;

    while (cin >> word)
    {
        words.push_back(word);
    }

    auto counts_vector = GetSortedWordCounts(move(words));

    cout << "Слово - Количество упоминаний в тексте"s << endl;
    // выводим первые 10 слов
    for (auto [iter, i] = tuple(counts_vector.begin(), 0); i < 10 && iter != counts_vector.end(); ++i, ++iter)
    {
        cout << iter->first << " - "s << iter->second << endl;
    }
}