// search_server_s1_t2_v2.cpp

#include <algorithm>
#include <cmath>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>
#include <numeric>
#include <cassert>

using namespace std;

const int MAX_RESULT_DOCUMENT_COUNT = 5;
const double EPSILON = 1e-6;

string ReadLine()
{
    string s;
    getline(cin, s);
    return s;
}

int ReadLineWithNumber()
{
    int result;
    cin >> result;
    ReadLine();
    return result;
}

vector<string> SplitIntoWords(const string &text)
{
    vector<string> words;
    string word;
    for (const char c : text)
    {
        if (c == ' ')
        {
            if (!word.empty())
            {
                words.push_back(word);
                word.clear();
            }
        }
        else
        {
            word += c;
        }
    }
    if (!word.empty())
    {
        words.push_back(word);
    }

    return words;
}

struct Document
{
    int id;
    double relevance;
    int rating;
};

enum class DocumentStatus
{
    ACTUAL,
    IRRELEVANT,
    BANNED,
    REMOVED,
};

class SearchServer
{
public:
    void SetStopWords(const string &text)
    {

        for (const string &word : SplitIntoWords(text))
        {

            stop_words_.insert(word);
        }
    }

    void AddDocument(int document_id, const string &document, DocumentStatus status, const vector<int> &ratings)
    {
        const vector<string> words = SplitIntoWordsNoStop(document);
        const double inv_word_count = 1.0 / words.size();
        for (const string &word : words)
        {
            word_to_document_freqs_[word][document_id] += inv_word_count;
        }
        documents_.emplace(document_id,
                           DocumentData{
                               ComputeAverageRating(ratings),
                               status});
    }

    template <typename T>
    vector<Document> FindTopDocuments(const string &raw_query, T document_predicate) const
    {
        const Query query = ParseQuery(raw_query);
        vector<Document> matched_documents = FindAllDocuments(query, document_predicate);
        sort(matched_documents.begin(), matched_documents.end(),
             [](const Document &lhs, const Document &rhs)
             {
                 if (abs(lhs.relevance - rhs.relevance) < EPSILON)
                 {
                     return lhs.rating > rhs.rating;
                 }
                 else
                 {
                     return lhs.relevance > rhs.relevance;
                 }
             });
        if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT)
        {
            matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
        }
        return matched_documents;
    }

    vector<Document> FindTopDocuments(const string &raw_query, DocumentStatus status) const
    {
        return FindTopDocuments(raw_query, [status](int document_id, DocumentStatus document_status, int rating)
                                { return document_status == status; });
    }

    vector<Document> FindTopDocuments(const string &raw_query) const
    {
        return FindTopDocuments(raw_query, DocumentStatus::ACTUAL);
    }

    int GetDocumentCount() const
    {
        return documents_.size();
    }

    tuple<vector<string>, DocumentStatus> MatchDocument(const string &raw_query, int document_id) const
    {
        const Query query = ParseQuery(raw_query);
        vector<string> matched_words;
        for (const string &word : query.plus_words)
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
        for (const string &word : query.minus_words)
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

private:
    struct DocumentData
    {
        int rating;
        DocumentStatus status;
    };

    set<string> stop_words_;
    map<string, map<int, double>> word_to_document_freqs_;
    map<int, DocumentData> documents_;

    bool IsStopWord(const string &word) const
    {
        return stop_words_.count(word) > 0;
    }

    vector<string> SplitIntoWordsNoStop(const string &text) const
    {
        vector<string> words;
        for (const string &word : SplitIntoWords(text))
        {
            if (!IsStopWord(word))
            {
                words.push_back(word);
            }
        }
        return words;
    }

    static int ComputeAverageRating(const vector<int> &ratings)
    {
        if (ratings.empty())
        {
            return 0;
        }
        int rating_sum = accumulate(ratings.begin(), ratings.end(), 0);
        return rating_sum / static_cast<int>(ratings.size());
    }

    struct QueryWord
    {
        string data;
        bool is_minus;
        bool is_stop;
    };

    QueryWord ParseQueryWord(string text) const
    {
        bool is_minus = false;
        // Word shouldn't be empty
        if (text[0] == '-')
        {
            is_minus = true;
            text = text.substr(1);
        }
        return {
            text,
            is_minus,
            IsStopWord(text)};
    }

    struct Query
    {
        set<string> plus_words;
        set<string> minus_words;
    };

    Query ParseQuery(const string &text) const
    {
        Query query;
        for (const string &word : SplitIntoWords(text))
        {
            const QueryWord query_word = ParseQueryWord(word);
            if (!query_word.is_stop)
            {
                if (query_word.is_minus)
                {
                    query.minus_words.insert(query_word.data);
                }
                else
                {
                    query.plus_words.insert(query_word.data);
                }
            }
        }
        return query;
    }

    // Existence required
    double ComputeWordInverseDocumentFreq(const string &word) const
    {
        return log(GetDocumentCount() * 1.0 / word_to_document_freqs_.at(word).size());
    }
    template <typename T>
    vector<Document> FindAllDocuments(const Query &query, T document_predicate) const
    {
        map<int, double> document_to_relevance;
        for (const string &word : query.plus_words)
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

        for (const string &word : query.minus_words)
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

        vector<Document> matched_documents;
        for (const auto [document_id, relevance] : document_to_relevance)
        {
            matched_documents.push_back({document_id,
                                         relevance,
                                         documents_.at(document_id).rating});
        }
        return matched_documents;
    }
};

// ==================== для примера =========================

void PrintDocument(const Document &document)
{
    cout << "{ "s
         << "document_id = "s << document.id << ", "s
         << "relevance = "s << document.relevance << ", "s
         << "rating = "s << document.rating
         << " }"s << endl;
}

template <typename T1, typename F1>
void Print(T1 &out, const F1 &container)
{
    int x = 1;
    for (const auto &element : container)
    {
        if (x != 1)
            out << ", "s << element;
        else
            out << element;
        x = 0;
    }
}

template <typename T>
ostream &operator<<(ostream &out, const vector<T> &container)
{
    out << "[";
    Print(out, container);
    out << "]";
    return out;
}

template <typename T, typename V>
ostream &operator<<(ostream &out, const pair<T, V> &container)
{
    out << container.first << ": "s;
    out << container.second;
    return out;
}

template <typename T>
ostream &operator<<(ostream &out, const set<T> &container)
{
    out << "{";
    Print(out, container);
    out << "}";
    return out;
}

template <typename T, typename V>
ostream &operator<<(ostream &out, const map<T, V> &container)
{
    out << "{";
    Print(out, container);
    out << "}";
    return out;
}

template <typename T, typename U>
void AssertEqualImpl(const T &t, const U &u, const string &t_str, const string &u_str, const string &file,
                     const string &func, unsigned line, const string &hint)
{
    if (t != u)
    {
        cout << boolalpha;
        cout << file << "("s << line << "): "s << func << ": "s;
        cout << "ASSERT_EQUAL("s << t_str << ", "s << u_str << ") failed: "s;
        cout << t << " != "s << u << "."s;
        if (!hint.empty())
        {
            cout << " Hint: "s << hint;
        }
        cout << endl;
        abort();
    }
}

#define ASSERT_EQUAL(a, b) AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, ""s)

#define ASSERT_EQUAL_HINT(a, b, hint) AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, (hint))

void AssertImpl(bool value, const string &expr_str, const string &file, const string &func, unsigned line,
                const string &hint)
{
    if (!value)
    {
        cout << file << "("s << line << "): "s << func << ": "s;
        cout << "ASSERT("s << expr_str << ") failed."s;
        if (!hint.empty())
        {
            cout << " Hint: "s << hint;
        }
        cout << endl;
        abort();
    }
}

#define ASSERT(expr) AssertImpl(!!(expr), #expr, __FILE__, __FUNCTION__, __LINE__, ""s)

#define ASSERT_HINT(expr, hint) AssertImpl(!!(expr), #expr, __FILE__, __FUNCTION__, __LINE__, (hint))

/* Подставьте вашу реализацию класса SearchServer сюда */

template <typename T>
void RunTestImpl(T func, string name_func)
{
    func();
    cerr << name_func << " OK" << endl;
    // abort();
}
#define RUN_TEST(func) RunTestImpl(func, #func)

// -------- Начало модульных тестов поисковой системы ----------
void TestExcludedStopWordsFromAddedDocumentContent()
{
    const int doc_id = 42;
    const string content = "cat in the city"s;
    const vector<int> ratings = {1, 2, 3};
    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto found_docs = server.FindTopDocuments("in"s);
        ASSERT_EQUAL(found_docs.size(), 1u);
        const Document &doc0 = found_docs[0];
        ASSERT_EQUAL(doc0.id, doc_id);
    }

    {
        SearchServer server;
        server.SetStopWords("in the"s);
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        ASSERT_HINT(server.FindTopDocuments("in"s).empty(), "Stop words must be excluded from documents"s);
    }
}

// Тест проверяет, что поисковая система исключает минус-слова поискового запроса
void TestExcludedSentencesMinusWordsFromAddedDocumentContent()
{
    {
        SearchServer server;
        server.AddDocument(1, "найден белый кот . на нём модный ошейник"s, DocumentStatus::ACTUAL, {1, 2, 3});
        server.AddDocument(2, "пушистый кот ищет хозяина . особые приметы : пушистый хвост"s, DocumentStatus::ACTUAL, {1, 2, 3});
        server.AddDocument(3, "в парке горького найден ухоженный пёс с выразительными глазами"s, DocumentStatus::ACTUAL, {1, 2, 3});
        const auto found_docs = server.FindTopDocuments("пушистый ухоженный кот -ошейник "s);
        ASSERT_EQUAL(found_docs.size(), 2);
        ASSERT_EQUAL(found_docs[0].id, 2);
        ASSERT_EQUAL(found_docs[1].id, 3);
    }
}

void TestMatchedDocumetsFromMatchDocument()
{
    SearchServer server;
    server.AddDocument(1, "найден белый кот . на нём модный кот кот ошейник"s, DocumentStatus::ACTUAL, {1, 2, 3});
    server.AddDocument(2, "пушистый кот ищет хозяина . особые приметы : пушистый кот хвост"s, DocumentStatus::ACTUAL, {1, 2, 3});
    server.AddDocument(3, "в парке горького найден ухоженный пёс с выразительными глазами"s, DocumentStatus::ACTUAL, {1, 2, 3});
    {
        auto [documents_list, documents_status] = server.MatchDocument("кот -пушистый"s, 2);
        ASSERT_EQUAL(documents_list.size(), 0);
    }
    {
        auto [documents_list, documents_status] = server.MatchDocument("кот модный белый"s, 1);
        ASSERT_EQUAL(documents_list.size(), 3);
        vector<string> doc_words_came_back={"белый"s,"кот"s,"модный"s};
        ASSERT_EQUAL(documents_list,doc_words_came_back);
        ASSERT(DocumentStatus::ACTUAL == documents_status);
    }
}

void TestCheckedPushDocuments(){
    {
        SearchServer search_server;
        ASSERT_EQUAL(search_server.GetDocumentCount(), 0);
        search_server.SetStopWords("и в на"s);
        search_server.AddDocument(0, "белый кот и модный ошейник"s, DocumentStatus::ACTUAL, {8, -3});
        ASSERT_EQUAL(search_server.GetDocumentCount(), 1);
        search_server.AddDocument(1, "пушистый кот пушистый хвост"s, DocumentStatus::ACTUAL, {7, 2, 7});
        ASSERT_EQUAL(search_server.GetDocumentCount(), 2);
        search_server.AddDocument(2, "ухоженный пёс выразительные глаза"s, DocumentStatus::ACTUAL, {5, -12, 2, 1});
        ASSERT_EQUAL(search_server.GetDocumentCount(), 3);
        search_server.AddDocument(3, "ухоженный скворец евгений"s, DocumentStatus::BANNED, {9});
        ASSERT_EQUAL(search_server.GetDocumentCount(), 4);
    }
}

void TestSortedRelevanceFromFindTopDocuments()
{
    {
        const vector<string> pattern_check={
                                    "белый кот модный ошейник"s,
                                    "пушистый кот пушистый хвост"s,
                                    "ухоженный пёс выразительные глаза"s
                                    };
         map<int,vector<int>>  rating_pattern= {
                                                    {0,{8, -3}},
                                                    {1,{7, 2, 7}},
                                                    {2,{5, -12, 2, 1}},
                                                  };
        double pattern_size=double(pattern_check.size());
        map <string,double> pattern_check_idf;
        pattern_check_idf["пушистый"s]=log(pattern_size/1.0);
        pattern_check_idf["ухоженный"s]=log(pattern_size/1.0);
        pattern_check_idf["кот"s]=log(pattern_size/2.0);

        map <int,map<string,double>> pattern_check_tf;
        pattern_check_tf[0]={{"пушистый",0/4.0},{"ухоженный"s,0/4.0},{"кот",1/4.0}};
        pattern_check_tf[1]={{"пушистый",2/4.0},{"ухоженный"s,0/4.0},{"кот",1/4.0}};
        pattern_check_tf[2]={{"пушистый",0/4.0},{"ухоженный"s,1/4.0},{"кот",0/4.0}};

        vector <Document> pattern_check_tf_idf;
        for(const auto&[id,x] : pattern_check_tf)
        {
          double relevante=x.at("пушистый")*pattern_check_idf.at("пушистый")+
                                         x.at("ухоженный")*pattern_check_idf.at("ухоженный")+
                                         x.at("кот")*pattern_check_idf.at("кот");
         double sum =0;
         for(double s : rating_pattern.at(id) ){
             sum+=s;
         }
         int rating = sum/rating_pattern.at(id).size();
         // int rating = accumulate(rating_pattern.at(id).begin(), rating_pattern.at(id).end(), 0)/rating_pattern.at(id).size();

          pattern_check_tf_idf.push_back({id,relevante,rating});
        }
        const double EPSILONEXTERNAL = 1e-6;
        sort(pattern_check_tf_idf.begin(), pattern_check_tf_idf.end(),
             [&EPSILONEXTERNAL](const Document &lhs, const Document &rhs)
             {
                 if (abs(lhs.relevance - rhs.relevance) < EPSILONEXTERNAL)
                 {
                     return lhs.rating > rhs.rating;
                 }
                 else
                 {
                     return lhs.relevance > rhs.relevance;
                 }
             });
        SearchServer search_server;
        search_server.SetStopWords("и в на"s);
        search_server.AddDocument(0,pattern_check[0], DocumentStatus::ACTUAL, rating_pattern[0]);
        search_server.AddDocument(1,pattern_check[1], DocumentStatus::ACTUAL, rating_pattern[1]);
        search_server.AddDocument(2,pattern_check[2], DocumentStatus::ACTUAL, rating_pattern[2]);
        vector<Document> document = search_server.FindTopDocuments("пушистый ухоженный кот"s);
        ASSERT_EQUAL(document.size(), 3);
        ASSERT_HINT(document[0].id == pattern_check_tf_idf[0].id && document[0].relevance ==  pattern_check_tf_idf[0].relevance, "Тест: сортировки по релевантости ");
        ASSERT_HINT(document[1].id == pattern_check_tf_idf[1].id && document[1].relevance ==  pattern_check_tf_idf[1].relevance, "Тест: сортировки по релевантости ");
        ASSERT_HINT(document[2].id == pattern_check_tf_idf[2].id && document[2].relevance ==  pattern_check_tf_idf[2].relevance, "Тест: сортировки по релевантости ");
    }
}

void TestCalcedRelevanceFromFindTopDocuments()
{
  {
        const vector<string> pattern_check={
                                    "белый кот модный ошейник"s,
                                    "пушистый кот пушистый хвост"s,
                                    "ухоженный пёс выразительные глаза"s
                                    };
         map<int,vector<int>>  rating_pattern= {
                                                    {0,{8, -3}},
                                                    {1,{7, 2, 7}},
                                                    {2,{5, -12, 2, 1}},
                                                  };
        double pattern_size=double(pattern_check.size());
        map <string,double> pattern_check_idf;
        pattern_check_idf["пушистый"s]=log(pattern_size/1.0);
        pattern_check_idf["ухоженный"s]=log(pattern_size/1.0);
        pattern_check_idf["кот"s]=log(pattern_size/2.0);

        map <int,map<string,double>> pattern_check_tf;
        pattern_check_tf[0]={{"пушистый",0/4.0},{"ухоженный"s,0/4.0},{"кот",1/4.0}};
        pattern_check_tf[1]={{"пушистый",2/4.0},{"ухоженный"s,0/4.0},{"кот",1/4.0}};
        pattern_check_tf[2]={{"пушистый",0/4.0},{"ухоженный"s,1/4.0},{"кот",0/4.0}};

        vector <Document> pattern_check_tf_idf;
        for(const auto&[id,x] : pattern_check_tf)
        {
          double relevante=x.at("пушистый")*pattern_check_idf.at("пушистый")+
                                         x.at("ухоженный")*pattern_check_idf.at("ухоженный")+
                                         x.at("кот")*pattern_check_idf.at("кот");
         double sum =0;
         for(double s : rating_pattern.at(id) ){
             sum+=s;
         }
         int rating = sum/rating_pattern.at(id).size();
         //int rating = accumulate(rating_pattern.at(id).begin(), rating_pattern.at(id).end(), 0)/rating_pattern.at(id).size();

          pattern_check_tf_idf.push_back({id,relevante,rating});
        }


        SearchServer search_server;
        search_server.SetStopWords("и в на"s);
        search_server.AddDocument(0,pattern_check[0], DocumentStatus::ACTUAL, rating_pattern[0]);
        search_server.AddDocument(1,pattern_check[1], DocumentStatus::ACTUAL, rating_pattern[1]);
        search_server.AddDocument(2,pattern_check[2], DocumentStatus::ACTUAL, rating_pattern[2]);
        vector<Document> document = search_server.FindTopDocuments("пушистый ухоженный кот"s);
        ASSERT_EQUAL(document.size(), 3);
        int count_in_relevant=0 ;
        for(Document x :pattern_check_tf_idf)
        {
            for(Document y :document)
            {
                if(x.id==y.id)
                {
                      ASSERT_HINT( x.relevance == y.relevance, "Тест: колькуляции релевантности"s);
                      ++count_in_relevant;
                }
            }
        }
        ASSERT_HINT(count_in_relevant==document.size(), "Тест: есть ошибки в расчете релевантности");
    }
}

template <typename T>
int caccumulate(const T x)
{
    double sum =0;
    for(double s : x )
    {
        sum+=s;
    }
    return sum;
}
void TestCalcedAvgRatingFromFindTopDocuments()
{
    {
        SearchServer search_server;
        search_server.SetStopWords("и в на"s);
        map<int,int> pattern_avg_rating_document;
        vector<int> buff;

        buff={8, -3};
        search_server.AddDocument(0, "белый кот и модный ошейник"s, DocumentStatus::ACTUAL, buff);
        pattern_avg_rating_document[0]=caccumulate(buff)/buff.size();

        buff={7, 2, 7};
        search_server.AddDocument(1, "пушистый кот пушистый хвост"s, DocumentStatus::ACTUAL,buff);
        pattern_avg_rating_document[1]=caccumulate(buff)/buff.size();

        buff={5, -12, 2, 1};
        search_server.AddDocument(2, "ухоженный пёс выразительные глаза"s, DocumentStatus::ACTUAL,buff);
        pattern_avg_rating_document[2]=caccumulate(buff)/buff.size();

        buff={9};
        search_server.AddDocument(3, "ухоженный скворец евгений"s, DocumentStatus::BANNED, buff);
        pattern_avg_rating_document[3]=caccumulate(buff)/buff.size();

        vector<Document> document = search_server.FindTopDocuments("пушистый ухоженный кот"s);

        ASSERT_EQUAL(document.size(), 3);
        ASSERT_HINT(document[0].id == 1 && document[0].rating == pattern_avg_rating_document.at(1),"Тест:рейтинг:вычисление среднего арефметического");
        ASSERT_HINT(document[1].id == 0 && document[1].rating == pattern_avg_rating_document.at(0), "Тест:рейтинг:вычисление среднего арефметического");
        ASSERT_HINT(document[2].id == 2 && document[2].rating == pattern_avg_rating_document.at(2), "Тест:рейтинг:вычисление среднего арефметического");
    }
}

void TestRanPredicateFromFindTopDocuments()
{
    SearchServer search_server;
    search_server.SetStopWords("и в на"s);
    search_server.AddDocument(0, "белый кот и модный ошейник"s, DocumentStatus::ACTUAL, {8, -3});
    search_server.AddDocument(1, "пушистый кот пушистый хвост"s, DocumentStatus::ACTUAL, {7, 2, 7});
    search_server.AddDocument(2, "ухоженный пёс выразительные глаза"s, DocumentStatus::ACTUAL, {5, -12, 2, 1});
    search_server.AddDocument(3, "ухоженный скворец евгений"s, DocumentStatus::BANNED, {9});
    {
        vector<Document> document = search_server.FindTopDocuments("пушистый ухоженный кот"s, [](int document_id, DocumentStatus status, int rating)
                                                                   { return document_id % 2 == 0; });
        ASSERT(document.size() == 2);
        ASSERT_HINT(document[0].id == 0, "Тест: Фильтрация результатов поиска с использованием предиката");
        ASSERT_HINT(document[1].id == 2, "Тест Фильтрация результатов поиска с использованием предиката");
    }
}

void TestGotStatusDocumentsFromFindTopDocuments()
{
    SearchServer search_server;
    search_server.SetStopWords("и в на"s);
    search_server.AddDocument(0, "белый кот и модный ошейник"s, DocumentStatus::ACTUAL, {8, -3});
    search_server.AddDocument(1, "пушистый кот пушистый хвост"s, DocumentStatus::ACTUAL, {7, 2, 7});
    search_server.AddDocument(2, "ухоженный пёс выразительные глаза"s, DocumentStatus::ACTUAL, {5, -12, 2, 1});
    search_server.AddDocument(3, "ухоженный скворец евгений"s, DocumentStatus::BANNED, {9});
    {
        vector<Document> document = search_server.FindTopDocuments("пушистый ухоженный кот"s, DocumentStatus::BANNED);
        ASSERT(document.size() == 1);
        ASSERT_HINT(document[0].id == 3, "Тест Поиск документов, имеющих заданный статус");
    }

    {
        vector<Document> document = search_server.FindTopDocuments("пушистый ухоженный кот"s, DocumentStatus::IRRELEVANT);
        ASSERT_HINT(document.size() == 0,"Таких документов нет");
    }
}


// Функция TestSearchServer является точкой входа для запуска тестов
void TestSearchServer()
{
    RUN_TEST(TestExcludedStopWordsFromAddedDocumentContent);
    RUN_TEST(TestExcludedSentencesMinusWordsFromAddedDocumentContent);
    RUN_TEST(TestMatchedDocumetsFromMatchDocument);
    RUN_TEST(TestCalcedRelevanceFromFindTopDocuments);
    RUN_TEST(TestSortedRelevanceFromFindTopDocuments);
    RUN_TEST(TestCalcedAvgRatingFromFindTopDocuments);
    RUN_TEST(TestRanPredicateFromFindTopDocuments);
    RUN_TEST(TestCheckedPushDocuments);
    RUN_TEST(TestGotStatusDocumentsFromFindTopDocuments);
    // Не забудьте вызывать остальные тесты здесь
}

// --------- Окончание модульных тестов поисковой системы -----------

int main()
{
    TestSearchServer();
    SearchServer search_server;
    search_server.SetStopWords("и в на"s);

    search_server.AddDocument(0, "белый кот и модный ошейник"s, DocumentStatus::ACTUAL, {8, -3});
    search_server.AddDocument(1, "пушистый кот пушистый хвост"s, DocumentStatus::ACTUAL, {7, 2, 7});
    search_server.AddDocument(2, "ухоженный пёс выразительные глаза"s, DocumentStatus::ACTUAL, {5, -12, 2, 1});
    search_server.AddDocument(3, "ухоженный скворец евгений"s, DocumentStatus::BANNED, {9});

    cout << "ACTUAL by default:"s << endl;
    for (const Document &document : search_server.FindTopDocuments("пушистый ухоженный кот"s))
    {
        PrintDocument(document);
    }

    cout << "BANNED:"s << endl;
    for (const Document &document : search_server.FindTopDocuments("пушистый ухоженный кот"s, DocumentStatus::BANNED))
    {
        PrintDocument(document);
    }

    cout << "Even ids:"s << endl;
    for (const Document &document : search_server.FindTopDocuments("пушистый ухоженный кот"s, [](int document_id, DocumentStatus status, int rating)
                                                                   { return document_id % 2 == 0; }))
    {
        PrintDocument(document);
    }
    return 0;
}