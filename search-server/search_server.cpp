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
            PrintMatchDocumentResult(document_id, words, status);
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

const std::map<std::string, double>& SearchServer::GetWordFrequencies(int document_id) const
{
    std::map<int, std::map<std::string, double>>::const_iterator res = document_to_word_freqs_.find(document_id);
    if (res != document_to_word_freqs_.end())
    {
        return res->second;
    }
    return dummy;
}

void SearchServer::RemoveDocument(int document_id)
{
    std::map<int, std::map<std::string, double>>::iterator dwf_it = document_to_word_freqs_.find(document_id);
    if (dwf_it != document_to_word_freqs_.end())
    {
        for(const auto& [k,v] :dwf_it->second)
        {
            std::map<std::string, std::map<int, double>>::iterator wdf_it=word_to_document_freqs_.find(k);
            if(wdf_it != word_to_document_freqs_.end())
            {
                wdf_it->second.erase(document_id);
                if(wdf_it->second.size()==0)
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


std::vector<Document> SearchServer::FindTopDocuments(const std::string &raw_query) const
{
    return FindTopDocuments(raw_query, DocumentStatus::ACTUAL);
}

int SearchServer::GetDocumentCount() const
{
    return documents_.size();
}

std::tuple<std::vector<std::string>, DocumentStatus> SearchServer::MatchDocument(const std::string &raw_query, int document_id) const
{
    const auto query = ParseQuery(raw_query);

    std::vector<std::string> matched_words;
    for (const std::string &word : query.plus_words)
    {
        if (word_to_document_freqs_.count(word) == 0)
        {
            continue;
        }
        if (word_to_document_freqs_.at(word).count(document_id))
        {
            matched_words.push_back(word);
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
    return {matched_words, documents_.at(document_id).status};
}

bool SearchServer::IsStopWord(const std::string &word) const
{
    return stop_words_.count(word) > 0;
}

bool SearchServer::IsValidWord(const std::string &word)
{
    // A valid word must not contain special characters
    return none_of(word.begin(), word.end(), [](char c)
                   { return c >= '\0' && c < ' '; });
}

std::vector<std::string> SearchServer::SplitIntoWordsNoStop(const std::string &text) const
{
    using std::string_literals::operator""s;

    std::vector<std::string> words;
    for (const std::string &word : SplitIntoWords(text))
    {
        if (!IsValidWord(word))
        {
            throw std::invalid_argument("Word "s + word + " is invalid"s);
        }
        if (!IsStopWord(word))
        {
            words.push_back(word);
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

SearchServer::QueryWord SearchServer::ParseQueryWord(const std::string &text) const
{
    using std::string_literals::operator""s;
    if (text.empty())
    {
        throw std::invalid_argument("Query word is empty"s);
    }
    std::string word = text;
    bool is_minus = false;
    if (word[0] == '-')
    {
        is_minus = true;
        word = word.substr(1);
    }
    if (word.empty() || word[0] == '-' || !IsValidWord(word))
    {
        throw std::invalid_argument("Query word "s + text + " is invalid");
    }

    return {word, is_minus, IsStopWord(word)};
}

SearchServer::Query SearchServer::ParseQuery(const std::string &text) const
{
    Query result;
    for (const std::string &word : SplitIntoWords(text))
    {
        const auto query_word = ParseQueryWord(word);
        if (!query_word.is_stop)
        {
            if (query_word.is_minus)
            {
                result.minus_words.insert(query_word.data);
            }
            else
            {
                result.plus_words.insert(query_word.data);
            }
        }
    }
    return result;
}

double SearchServer::ComputeWordInverseDocumentFreq(const std::string &word) const
{
    return std::log(GetDocumentCount() * 1.0 / word_to_document_freqs_.at(word).size());
}