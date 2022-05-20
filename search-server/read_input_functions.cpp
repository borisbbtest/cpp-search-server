#include "read_input_functions.h"

std::string ReadLine()
{
    std::string s;
    getline(std::cin, s);
    return s;
}

int ReadLineWithNumber()
{
    int result;
    std::cin >> result;
    ReadLine();
    return result;
}

void PrintMatchDocumentResult(int document_id, const std::vector<std::string> &words, DocumentStatus status)
{
    using std::string_literals::operator""s;
    std::cout << "{ "s
              << "document_id = "s << document_id << ", "s
              << "status = "s << static_cast<int>(status) << ", "s
              << "words ="s;
    for (const std::string &word : words)
    {
        std::cout << ' ' << word;
    }
    std::cout << "}"s << std::endl;
}

void MatchDocuments(const SearchServer &search_server, const std::string &query)
{
    using std::string_literals::operator""s;
    try
    {
        std::cout << "Матчинг документов по запросу: "s << query << std::endl;
        const int document_count = search_server.GetDocumentCount();
        for (int index = 0; index < document_count; ++index)
        {
            const int document_id = search_server.GetDocumentId(index);
            const auto [words, status] = search_server.MatchDocument(query, document_id);
            PrintMatchDocumentResult(document_id, words, status);
        }
    }
    catch (const std::invalid_argument &e)
    {
        std::cout << "Ошибка матчинга документов на запрос "s << query << ": "s << e.what() << std::endl;
    }
}