#include <vector>
#include <string>
#include <execution>
#include <numeric>
#include "document.h"
#include "search_server.h"

std::vector<std::vector<Document>> ProcessQueries(const SearchServer &search_server, const std::vector<std::string> &queries)
{
    std::vector<std::vector<Document>> result(queries.size());
    std::transform(std::execution::par,
                   queries.begin(), queries.end(),
                   result.begin(),
                   [&search_server](const std::string &query)
                   {
                       return search_server.FindTopDocuments(query);
                   });
    return result;
}

std::vector<Document> ProcessQueriesJoined(const SearchServer &search_server, const std::vector<std::string> &queries)
{
    std::vector<Document> documents;
    for (const auto &local_documents : ProcessQueries(search_server, queries))
    {
        documents.insert(documents.end(), local_documents.begin(), local_documents.end());
    }
    return documents;
}