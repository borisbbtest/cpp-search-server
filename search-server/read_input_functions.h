#pragma once
#include "search_server.h"
#include "request_queue.h"

std::string ReadLine();
int ReadLineWithNumber();
std::ostream &operator<<(std::ostream &out, const Document &document);
void PrintDocument(const Document &document);
void PrintMatchDocumentResult(int document_id, const std::vector<std::string> &words, DocumentStatus status);
void FindTopDocuments(const SearchServer &search_server, const std::string &raw_query);
void MatchDocuments(const SearchServer &search_server, const std::string &query);
void AddDocument(SearchServer &search_server, int document_id, const std::string &document, DocumentStatus status, const std::vector<int> &ratings);

template <typename Iterator>
std::ostream &operator<<(std::ostream &out, const IteratorRange<Iterator> &range)
{
    for (Iterator it = range.begin(); it != range.end(); ++it)
    {
        out << *it;
    }
    return out;
}