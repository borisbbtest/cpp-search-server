#include "request_queue.h"

RequestQueue::RequestQueue(const SearchServer &search_server) : search_server_(search_server)
{
    // напишите реализацию
}
// сделаем "обёртки" для всех методов поиска, чтобы сохранять результаты для нашей статистики

std::vector<Document> RequestQueue::AddFindRequest(const std::string &raw_query, DocumentStatus status)
{
    // напишите реализацию
    return AddFindRequest(raw_query, [status](int document_id, DocumentStatus document_status, int rating)
                          { return document_status == status; });
}

std::vector<Document> RequestQueue::AddFindRequest(const std::string &raw_query)
{
    // напишите реализацию
    std::vector<Document> res = AddFindRequest(raw_query, DocumentStatus::ACTUAL);
    return res;
}

int RequestQueue::GetNoResultRequests() const
{
    // напишите реализацию
    int count = 0;
    for (int i = 0; i < requests_.size(); i++)
    {
        if (requests_[i].status_failed)
            ++count;
    }
    return count;
}
