#pragma once
#include "search_server.h"
#include "document.h"
#include "paginator.h"
#include <deque>

template <typename Container>
auto Paginate(const Container &c, size_t page_size)
{
    return Paginator(begin(c), end(c), page_size);
}

class RequestQueue
{
public:
    explicit RequestQueue(const SearchServer &search_server);
    template <typename DocumentPredicate>
    std::vector<Document> AddFindRequest(const std::string &raw_query, DocumentPredicate document_predicate);
    std::vector<Document> AddFindRequest(const std::string &raw_query, DocumentStatus status);
    std::vector<Document> AddFindRequest(const std::string &raw_query);
    int GetNoResultRequests() const;

private:
    struct QueryResult
    {
        // определите, что должно быть в структуре
        bool status_failed;
        int time_set;
    };
    std::deque<QueryResult> requests_;
    int now_time = 0;
    const static int min_in_day_ = 1440;
    const SearchServer &search_server_;
    // возможно, здесь вам понадобится что-то ещё
};

template <typename DocumentPredicate>
std::vector<Document> RequestQueue::AddFindRequest(const std::string &raw_query, DocumentPredicate document_predicate)
{
    // напишите реализацию
    const std::vector<Document> &res = RequestQueue::search_server_.FindTopDocuments(raw_query, document_predicate);
    RequestQueue::QueryResult buff;
    buff.status_failed = res.empty();
    now_time++;
    if (min_in_day_ < now_time)
    {
        now_time = 1;
    }
    buff.time_set = now_time;
    if (!requests_.empty())
    {
        if (now_time == requests_.front().time_set)
        {
            requests_.pop_front();
        }
    }
    requests_.push_back(buff);

    return res;
}