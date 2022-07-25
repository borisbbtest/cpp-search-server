#include "remove_duplicates.h"
#include <map>
#include <vector>
#include <iostream>

void RemoveDuplicates(SearchServer &search_server)
{
    using std::string_literals::operator""s;
    std::vector<int> id_for_delete;
    std::set<std::set<std::string_view>> tmp;

    for (const int document_id : search_server)
    {
        std::set<std::string_view> buff;
        for (auto &[word, tf] : search_server.GetWordFrequencies(document_id))
        {
            buff.insert(word);
        }

        if (tmp.find(buff) == tmp.end())
        {
            tmp.insert(buff);
        }
        else
        {
            id_for_delete.push_back(document_id);
        }
    }

    for (int id : id_for_delete)
    {
        search_server.RemoveDocument(id);
        std::cout << "Found duplicate document id "s << id << std::endl;
    }
}