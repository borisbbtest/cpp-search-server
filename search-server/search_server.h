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
    Query ParseQuery(const std::string_view text) const;
    ParQuery ParParseQuery(const std::string_view text) const;

    double ComputeWordInverseDocumentFreq(const std::string_view word) const;

    template <typename DocumentPredicate>
    std::vector<Document> FindAllDocuments(const Query &query, DocumentPredicate document_predicate) const;
};

template <typename DocumentPredicate>
std::vector<Document> SearchServer::FindTopDocuments(const std::string_view raw_query, DocumentPredicate document_predicate) const
{
    const auto query = ParseQuery(raw_query);

    auto matched_documents = FindAllDocuments(query, document_predicate);

    sort(matched_documents.begin(), matched_documents.end(), [](const Document &lhs, const Document &rhs)
         {
            //Постоянно меняется мой код. Всегда была константа.
            if (std::abs(lhs.relevance - rhs.relevance) < EPSILON) {
                return lhs.rating > rhs.rating;
            } else {
                return lhs.relevance > rhs.relevance;
            } });
    if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT)
    {
        matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
    }

    return matched_documents;
}

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

template <typename DocumentPredicate>
std::vector<Document> SearchServer::FindAllDocuments(const Query &query, DocumentPredicate document_predicate) const
{
    std::map<int, double> document_to_relevance;
    for (const std::string_view word : query.plus_words)
    {
        if (word_to_document_freqs_.count(word) == 0)
        {
            continue;
        }
        const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);
        for (const auto [document_id, term_freq] : word_to_document_freqs_.at(word))
        {
            const auto &document_data = documents_.at(document_id);
            if (document_predicate(document_id, document_data.status, document_data.rating))
            {
                document_to_relevance[document_id] += term_freq * inverse_document_freq;
            }
        }
    }

    for (const std::string_view word : query.minus_words)
    {
        if (word_to_document_freqs_.count(word) == 0)
        {
            continue;
        }
        for (const auto [document_id, _] : word_to_document_freqs_.at(word))
        {
            document_to_relevance.erase(document_id);
        }
    }

    std::vector<Document> matched_documents;
    for (const auto [document_id, relevance] : document_to_relevance)
    {
        matched_documents.push_back({document_id, relevance, documents_.at(document_id).rating});
    }
    return matched_documents;
}

void PrintMatchDocumentResult(int document_id, const std::vector<std::string_view> words, DocumentStatus status);
void MatchDocuments(const SearchServer &search_server, const std::string_view query);
void FindTopDocuments(const SearchServer &search_server, const std::string_view raw_query);
void AddDocument(SearchServer &search_server, int document_id, const std::string_view document, DocumentStatus status, const std::vector<int> &ratings);
