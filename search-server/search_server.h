#pragma once
#include <iostream>
#include <algorithm>
#include <map>
#include <cmath>
#include <vector>
#include <string>
#include <numeric>
#include <execution>
#include <set>
#include "string_processing.h"
#include "document.h"
#include "log_duration.h"
#include "concurrent_map.h"

const int MAX_RESULT_DOCUMENT_COUNT = 5;
const double EPSILON = 1e-6;

class SearchServer
{

public:
    template <typename StringContainer>
    explicit SearchServer(const StringContainer &stop_words);
    explicit SearchServer(const std::string &stop_words_text);
    explicit SearchServer(const std::string_view stop_words_text);
    std::set<int>::const_iterator begin() const;
    std::set<int>::const_iterator end() const;
    const std::map<std::string_view, double> &GetWordFrequencies(int document_id) const;
    void RemoveDocument(int document_id);
    void RemoveDocument(const std::execution::sequenced_policy &, int document_id);
    void RemoveDocument(const std::execution::parallel_policy &, int document_id);
    void AddDocument(int document_id, const std::string_view document, DocumentStatus status, const std::vector<int> &ratings);

    template <typename DocumentPredicate>
    std::vector<Document> FindTopDocuments(const std::string_view raw_query, DocumentPredicate document_predicate) const;
    std::vector<Document> FindTopDocuments(const std::string_view raw_query, DocumentStatus status) const;
    std::vector<Document> FindTopDocuments(const std::string_view raw_query) const;

    template <typename ExecutionPolicy>
    std::vector<Document> FindTopDocuments(ExecutionPolicy &&policy, const std::string_view raw_query) const;

    template <typename ExecutionPolicy, typename DocumentPredicate>
    std::vector<Document> FindTopDocuments(ExecutionPolicy &&policy, const std::string_view raw_query, DocumentPredicate document_predicate) const;

    template <typename ExecutionPolicy>
    std::vector<Document> FindTopDocuments(ExecutionPolicy &&policy, const std::string_view raw_query, DocumentStatus status) const;

    int GetDocumentCount() const;
    std::tuple<std::vector<std::string_view>, DocumentStatus> MatchDocument(const std::string_view raw_query, int document_id) const;
    std::tuple<std::vector<std::string_view>, DocumentStatus> MatchDocument(const std::execution::sequenced_policy &, const std::string_view raw_query, int document_id) const;
    std::tuple<std::vector<std::string_view>, DocumentStatus> MatchDocument(const std::execution::parallel_policy &, const std::string_view raw_query, int document_id) const;

private:
    struct DocumentData
    {
        int rating;
        DocumentStatus status;
        std::string data_str;
        std::map<std::string_view, double> word_f;
    };

    std::set<std::string, std::less<>> stop_words_;
    std::map<std::string_view, std::map<int, double>> word_to_document_freqs_;
    std::map<int, std::map<std::string_view, double>> document_to_word_freqs_;
    std::map<int, DocumentData> documents_;
    std::set<int> document_ids_;

    bool IsStopWord(const std::string_view word) const;
    static bool IsValidWord(const std::string_view word);

    std::vector<std::string_view> SplitIntoWordsNoStop(const std::string_view text) const;

    static int ComputeAverageRating(const std::vector<int> &ratings);
    struct QueryWord
    {
        std::string_view data;
        bool is_minus;
        bool is_stop;
    };

    QueryWord ParseQueryWord(const std::string_view word) const;

    struct Query
    {

        std::set<std::string_view> plus_words;
        std::set<std::string_view> minus_words;
    };
    struct ParQuery
    {
        std::vector<std::string_view> plus_words;
        std::vector<std::string_view> minus_words;
    };

    Query ParseQuery(const std::execution::sequenced_policy &, const std::string_view text) const;
    ParQuery ParseQuery(const std::execution::parallel_policy &, const std::string_view text) const;

    double ComputeWordInverseDocumentFreq(const std::string_view word) const;

    template <typename ExecutionPolicy, typename DocumentPredicate, typename Q>
    std::vector<Document> FindAllDocuments(ExecutionPolicy &&policy, const Q &query, DocumentPredicate document_predicate) const;
};

template <typename StringContainer>
SearchServer::SearchServer(const StringContainer &stop_words)
    : stop_words_(MakeUniqueNonEmptyStrings(stop_words)) // Extract non-empty stop words
{
    using std::string_literals::operator""s;

    if (!std::all_of(stop_words_.begin(), stop_words_.end(), IsValidWord))
    {
        throw std::invalid_argument("Some of stop words are invalid"s);
    }
}

// Обертки по поиску
//новые

template <typename ExecutionPolicy>
std::vector<Document> SearchServer::FindTopDocuments(ExecutionPolicy &&policy, const std::string_view raw_query) const
{
    return FindTopDocuments(policy, raw_query, DocumentStatus::ACTUAL);
}

template <typename ExecutionPolicy>
std::vector<Document> SearchServer::FindTopDocuments(ExecutionPolicy &&policy, const std::string_view raw_query, DocumentStatus status) const
{
    return FindTopDocuments(policy, raw_query, [status](int document_id, DocumentStatus document_status, int rating)
                            { return document_status == status; });
}

template <typename DocumentPredicate>
std::vector<Document> SearchServer::FindTopDocuments(const std::string_view raw_query, DocumentPredicate document_predicate) const
{
    return FindTopDocuments(std::execution::seq, raw_query, document_predicate);
}

/* Логика тут по поиску документов всех и топ */

template <typename ExecutionPolicy, typename DocumentPredicate>
std::vector<Document> SearchServer::FindTopDocuments(ExecutionPolicy &&policy, const std::string_view raw_query, DocumentPredicate document_predicate) const
{
    const auto query = ParseQuery(policy, raw_query);

    auto matched_documents = FindAllDocuments(policy, query, document_predicate);

    std::sort(policy, matched_documents.begin(), matched_documents.end(), [](const Document &lhs, const Document &rhs)
              {
            //Постоянно меняется мой код. Всегда была константа.
                if (std::abs(lhs.relevance - rhs.relevance) < EPSILON) 
                {
                    return lhs.rating > rhs.rating;
                }
                else
                {
                    return lhs.relevance > rhs.relevance;
                } });
    if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT)
    {
        matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
    }

    return matched_documents;
}

template <typename ExecutionPolicy, typename DocumentPredicate, typename Q>
std::vector<Document> SearchServer::FindAllDocuments(ExecutionPolicy &&policy, const Q &query, DocumentPredicate document_predicate) const
{
    ConcurrentMap<int, double> document_to_relevance(10);
    std::for_each(policy,
                  query.plus_words.begin(), query.plus_words.end(),
                  [this, &document_to_relevance, &document_predicate, &policy](const std::string_view word)
                  {
                      if (word_to_document_freqs_.count(word) > 0)
                      {
                          const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);
                          std::for_each(policy,
                                        word_to_document_freqs_.at(word).begin(), word_to_document_freqs_.at(word).end(),
                                        [this, &document_to_relevance, &document_predicate, &inverse_document_freq](const auto &p)
                                        {
                                            const auto &document_data = documents_.at(p.first);
                                            if (document_predicate(p.first, document_data.status, document_data.rating))
                                            {
                                                document_to_relevance[p.first].ref_to_value += p.second * inverse_document_freq;
                                            }
                                        });
                      }
                  });
    std::for_each(policy,
                  query.minus_words.begin(), query.minus_words.end(),
                  [this, &document_to_relevance, &policy](const std::string_view word)
                  {
                      if (word_to_document_freqs_.count(word) > 0)
                      {
                          std::for_each(policy,
                                        word_to_document_freqs_.at(word).begin(), word_to_document_freqs_.at(word).end(),
                                        [&document_to_relevance](const auto &p)
                                        {
                                            document_to_relevance.erase(p.first);
                                        });
                      }
                  });
    auto result = document_to_relevance.BuildOrdinaryMap();
    std::vector<Document> matched_documents(result.size());
    std::atomic_int index = 0;
    std::for_each(policy,
                  result.begin(), result.end(),
                  [this, &matched_documents, &index](const auto &p)
                  {
                      matched_documents[index++] = Document(p.first, p.second, documents_.at(p.first).rating);
                  });
    return matched_documents;
}

void PrintMatchDocumentResult(int document_id, const std::vector<std::string_view> words, DocumentStatus status);
void MatchDocuments(const SearchServer &search_server, const std::string_view query);
void FindTopDocuments(const SearchServer &search_server, const std::string_view raw_query);
void AddDocument(SearchServer &search_server, int document_id, const std::string_view document, DocumentStatus status, const std::vector<int> &ratings);
