#pragma once
#include "search_server.h"
#include "request_queue.h"

std::string ReadLine();
int ReadLineWithNumber();
void PrintMatchDocumentResult(int document_id, const std::vector<std::string> &words, DocumentStatus status);
void MatchDocuments(const SearchServer &search_server, const std::string &query);

template <typename Iterator>
std::ostream &operator<<(std::ostream &out, const IteratorRange<Iterator> &range)
{
    for (Iterator it = range.begin(); it != range.end(); ++it)
    {
        out << *it;
    }
    return out;
}