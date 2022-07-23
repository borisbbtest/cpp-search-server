#include "search_server.h"

void AddDocument(SearchServer &search_server, int document_id, const std::string &document, DocumentStatus status, const std::vector<int> &ratings)
{
    using std::string_literals::operator""s;
    try
    {
        search_server.AddDocument(document_id, document, status, ratings);
    }
    catch (const std::invalid_argument &e)
    {
        std::cout << "Ошибка добавления документа "s << document_id << ": "s << e.what() << std::endl;
    }
}

void FindTopDocuments(const SearchServer &search_server, const std::string &raw_query)
{
    using std::string_literals::operator""s;

    std::cout << "Результаты поиска по запросу: "s << raw_query << std::endl;
    try
    {
        for (const Document &document : search_server.FindTopDocuments(raw_query))
        {
            PrintDocument(document);
        }
    }
    catch (const std::invalid_argument &e)
    {
        std::cout << "Ошибка поиска: "s << e.what() << std::endl;
    }
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
        for (const int document_id : search_server)
        {
            const auto [words, status] = search_server.MatchDocument(query, document_id);
           // PrintMatchDocumentResult(document_id, words, status);
        }
    }
    catch (const std::invalid_argument &e)
    {
        std::cout << "Ошибка матчинга документов на запрос "s << query << ": "s << e.what() << std::endl;
    }
}

void SearchServer::AddDocument(int document_id, const std::string &document, DocumentStatus status, const std::vector<int> &ratings)
{
    using std::string_literals::operator""s;

    if ((document_id < 0) || (documents_.count(document_id) > 0))
    {
        throw std::invalid_argument("Invalid document_id"s);
    }
    const auto words = SplitIntoWordsNoStop(document);
    const double inv_word_count = 1.0 / words.size();
    for (const std::string &word : words)
    {
        word_to_document_freqs_[word][document_id] += inv_word_count;
        document_to_word_freqs_[document_id][word] += inv_word_count;
    }
    documents_.emplace(document_id, DocumentData{ComputeAverageRating(ratings), status});
    document_ids_.insert(document_id);
}

std::vector<Document> SearchServer::FindTopDocuments(const std::string &raw_query, DocumentStatus status) const
{
    return FindTopDocuments(raw_query, [status](int document_id, DocumentStatus document_status, int rating)
                            { return document_status == status; });
}

SearchServer::SearchServer(const std::string &stop_words_text)
    : SearchServer(SplitIntoWords(stop_words_text)) // Invoke delegating constructor
                                                    // from std::string container
{
}

std::set<int>::const_iterator SearchServer::begin() const
{
    return document_ids_.begin();
}

std::set<int>::const_iterator SearchServer::end() const
{
    return document_ids_.end();
}

const std::map<std::string, double> &SearchServer::GetWordFrequencies(int document_id) const
{
    std::map<int, std::map<std::string, double>>::const_iterator res = document_to_word_freqs_.find(document_id);
    if (res != document_to_word_freqs_.end())
    {
        return res->second;
    }
    static const std::map<std::string, double> dummy;
    return dummy;
}

void SearchServer::RemoveDocument(int document_id)
{

    RemoveDocument(std::execution::seq, document_id);
}

void SearchServer::RemoveDocument(const std::execution::sequenced_policy &, int document_id)
{
    std::map<int, std::map<std::string, double>>::iterator dwf_it = document_to_word_freqs_.find(document_id);
    if (dwf_it != document_to_word_freqs_.end())
    {
        for (const auto &[k, v] : dwf_it->second)
        {
            std::map<std::string, std::map<int, double>>::iterator wdf_it = word_to_document_freqs_.find(k);
            if (wdf_it != word_to_document_freqs_.end())
            {
                wdf_it->second.erase(document_id);
                if (wdf_it->second.size() == 0)
                {
                    word_to_document_freqs_.erase(k);
                }
            }
        }
        document_to_word_freqs_.erase(dwf_it);
        documents_.erase(document_id);
        document_ids_.erase(document_id);
    }
}

void SearchServer::RemoveDocument(const std::execution::parallel_policy &, int document_id)
{
    if (document_to_word_freqs_.count(document_id) == 0)
    {
        return;
    }

    const auto &word_freqs = document_to_word_freqs_.at(document_id);
    std::vector<std::string_view> words(word_freqs.size());
    transform(
        std::execution::par,
        word_freqs.begin(), word_freqs.end(),
        words.begin(),
        [](const auto &item)
        {
            return std::string_view{item.first};
        });

    std::for_each(
        std::execution::par,
        words.begin(),
        words.end(),
        [this, document_id](const auto &m)
        {
            word_to_document_freqs_.at({m.begin(), m.end()}).erase(document_id);
        });
    document_to_word_freqs_.erase(document_id);
    documents_.erase(document_id);
    document_ids_.erase(document_id);
}

std::vector<Document> SearchServer::FindTopDocuments(const std::string &raw_query) const
{
    return FindTopDocuments(raw_query, DocumentStatus::ACTUAL);
}

int SearchServer::GetDocumentCount() const
{
    return documents_.size();
}

std::tuple<std::vector<std::string>, DocumentStatus> SearchServer::MatchDocument(const std::string_view raw_query, int document_id) const
{

    return SearchServer::MatchDocument(std::execution::seq, raw_query, document_id);
}

std::tuple<std::vector<std::string>, DocumentStatus> SearchServer::MatchDocument(const std::execution::sequenced_policy &, const std::string_view raw_query, int document_id) const
{
    if (!document_ids_.count(document_id))
    {
        using namespace std::string_literals;
        throw std::out_of_range("out of range"s);
    }
    const auto query = ParseQuery(raw_query);

    std::set<std::string> matched_words;
    for (const std::string &word : query.plus_words)
    {
        if (word_to_document_freqs_.count(word) == 0)
        {
            continue;
        }
        if (word_to_document_freqs_.at(word).count(document_id))
        {
            matched_words.insert(word);
        }
    }
    for (const std::string &word : query.minus_words)
    {
        if (word_to_document_freqs_.count(word) == 0)
        {
            continue;
        }
        if (word_to_document_freqs_.at(word).count(document_id))
        {
            matched_words.clear();
            break;
        }
    }
    std::vector<std::string> res(matched_words.begin(), matched_words.end());
    return {res, documents_.at(document_id).status};
}

std::tuple<std::vector<std::string>, DocumentStatus> SearchServer::MatchDocument(const std::execution::parallel_policy &, const std::string_view raw_query, int document_id) const
{
    if (!document_ids_.count(document_id))
    {
        using namespace std::string_literals;
        throw std::out_of_range("out of range"s);
    }
    const auto query = ParseQuery(raw_query);

    const auto status = documents_.at(document_id).status;

    const auto word_checker = [this, document_id](const std::string word)
    {
        const auto it = word_to_document_freqs_.find(word);
        return it != word_to_document_freqs_.end() && it->second.count(document_id);
    };

    if (std::any_of(std::execution::par, query.minus_words.begin(), query.minus_words.end(), word_checker))
    {
        std::vector<std::string> m;
        return {m, status};
    }

    std::vector<std::string> matched_words(query.plus_words.size());
    auto words_end = std::copy_if(
        std::execution::par,
        query.plus_words.begin(), query.plus_words.end(),
        matched_words.begin(),
        word_checker);
    std::sort(std::execution::par, matched_words.begin(), words_end);
    words_end = std::unique(std::execution::par, matched_words.begin(), words_end);
    matched_words.erase(words_end, matched_words.end());

    return {matched_words, status};
}

bool SearchServer::IsStopWord(const std::string_view word) const
{
    return stop_words_.count(word) > 0;
}

bool SearchServer::IsValidWord(const std::string_view word)
{
    // A valid word must not contain special characters
    return std::none_of(std::execution::par,word.begin(), word.end(), [](char c)
                        { return c >= '\0' && c < ' '; });
}

std::vector<std::string> SearchServer::SplitIntoWordsNoStop(const std::string &text) const
{
    using std::string_literals::operator""s;

    std::vector<std::string> words;
    for (const std::string_view &word : SplitIntoWords({text}))
    {
        if (!IsValidWord(word))
        {
            throw std::invalid_argument("Word "s + std::string(word) + " is invalid"s);
        }
        if (!IsStopWord(word))
        {
            words.push_back({word.begin(), word.end()});
        }
    }
    return words;
}

int SearchServer::ComputeAverageRating(const std::vector<int> &ratings)
{
    if (ratings.empty())
    {
        return 0;
    }
    // Постоянно эта функция меняется эта функция платформой. Мой код изначально был другим. Я ее правлю постоянно)))
    int rating_sum = std::accumulate(ratings.begin(), ratings.end(), 0);
    return rating_sum / static_cast<int>(ratings.size());
}

SearchServer::QueryWord SearchServer::ParseQueryWord(std::string_view word) const
{
    using std::string_literals::operator""s;

    if (word.empty())
    {
        throw std::invalid_argument("Query word is empty"s);
    }
    bool is_minus = false;
    if (word[0] == '-')
    {
        is_minus = true;
        word.remove_prefix(1);
    }
    if (word.empty() || word[0] == '-' || !IsValidWord(word))
    {
        throw std::invalid_argument("Query word "s + std::string(word) + " is invalid");
    }
    return {word, is_minus, IsStopWord(word)};
}

SearchServer::Query SearchServer::ParseQuery(const std::string_view text) const
{
    Query result;
    for (const std::string_view word : SplitIntoWords(text))
    {
        const auto query_word = ParseQueryWord(word);
        if (!query_word.is_stop)
        {
            if (query_word.is_minus)
            {
                result.minus_words.insert(std::string(query_word.data));
            }
            else
            {
                result.plus_words.insert(std::string(query_word.data));
            }
        }
    }
    return result;
}

double SearchServer::ComputeWordInverseDocumentFreq(const std::string &word) const
{
    return std::log(GetDocumentCount() * 1.0 / word_to_document_freqs_.at(word).size());
}