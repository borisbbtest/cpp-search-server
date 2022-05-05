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



// -------- Начало модульных тестов поисковой системы ----------
template <typename T>
void RunTestImpl(T func, string name_func)
{
    func();
    cerr << name_func << " OK" << endl;
    // abort();
}
#define RUN_TEST(func) RunTestImpl(func, #func)


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
        auto [words_list, words_status] = server.MatchDocument("кот -пушистый"s, 2);
        ASSERT_EQUAL(words_list.size(), 0);
    }
    {
        auto [words_list, words_status] = server.MatchDocument("кот модный белый"s, 1);
        ASSERT_EQUAL(words_list.size(), 3);
        vector<string> doc_words_came_back = {"белый"s, "кот"s, "модный"s};
        ASSERT_EQUAL(words_list, doc_words_came_back);
        ASSERT(DocumentStatus::ACTUAL == words_status);
    }
}

void TestCheckedPushDocuments()
{
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
        const vector<string> pattern_check = {
            "белый кот модный ошейник"s,
            "пушистый кот пушистый хвост"s,
            "ухоженный пёс выразительные глаза"s};
        map<int, vector<int>> rating_pattern = {
            {0, {8, -3}},
            {1, {7, 2, 7}},
            {2, {5, -12, 2, 1}},
        };
        double pattern_size = double(pattern_check.size());
        map<string, double> pattern_check_idf;
        pattern_check_idf["пушистый"s] = log(pattern_size / 1.0);
        pattern_check_idf["ухоженный"s] = log(pattern_size / 1.0);
        pattern_check_idf["кот"s] = log(pattern_size / 2.0);

        map<int, map<string, double>> pattern_check_tf;
        pattern_check_tf[0] = {{"пушистый", 0 / 4.0}, {"ухоженный"s, 0 / 4.0}, {"кот", 1 / 4.0}};
        pattern_check_tf[1] = {{"пушистый", 2 / 4.0}, {"ухоженный"s, 0 / 4.0}, {"кот", 1 / 4.0}};
        pattern_check_tf[2] = {{"пушистый", 0 / 4.0}, {"ухоженный"s, 1 / 4.0}, {"кот", 0 / 4.0}};

        vector<Document> pattern_check_tf_idf;
        for (const auto &[id, x] : pattern_check_tf)
        {
            double relevante = x.at("пушистый") * pattern_check_idf.at("пушистый") +
                               x.at("ухоженный") * pattern_check_idf.at("ухоженный") +
                               x.at("кот") * pattern_check_idf.at("кот");
            double sum = 0;
            int rating = accumulate(rating_pattern.at(id).begin(), rating_pattern.at(id).end(), 0)/rating_pattern.at(id).size();

            pattern_check_tf_idf.push_back({id, relevante, rating});
        }

        sort(pattern_check_tf_idf.begin(), pattern_check_tf_idf.end(),
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
        SearchServer search_server;
        search_server.SetStopWords("и в на"s);
        search_server.AddDocument(0, pattern_check[0], DocumentStatus::ACTUAL, rating_pattern[0]);
        search_server.AddDocument(1, pattern_check[1], DocumentStatus::ACTUAL, rating_pattern[1]);
        search_server.AddDocument(2, pattern_check[2], DocumentStatus::ACTUAL, rating_pattern[2]);
        vector<Document> document = search_server.FindTopDocuments("пушистый ухоженный кот"s);
        ASSERT_EQUAL(document.size(), 3);
        ASSERT_HINT(document[0].id == pattern_check_tf_idf[0].id, "Тест: сортировки по релевантости ");
        ASSERT_HINT(document[1].id == pattern_check_tf_idf[1].id, "Тест: сортировки по релевантости ");
        ASSERT_HINT(document[2].id == pattern_check_tf_idf[2].id, "Тест: сортировки по релевантости ");
    }
}

void TestCalculatedRelevanceFromFindTopDocuments()
{
    {
        const vector<string> pattern_check = {
                                                "белый кот модный ошейник"s,
                                                "пушистый кот пушистый хвост"s,
                                                "ухоженный пёс выразительные глаза"s
                                             };
        map<int, vector<int>> rating_pattern = {
                                                    {0, {8, -3}},
                                                    {1, {7, 2, 7}},
                                                    {2, {5, -12, 2, 1}},
                                                };
        double pattern_size = double(pattern_check.size());
        map<string, double> pattern_check_idf;
        pattern_check_idf["пушистый"s] = log(pattern_size / 1.0);
        pattern_check_idf["ухоженный"s] = log(pattern_size / 1.0);
        pattern_check_idf["кот"s] = log(pattern_size / 2.0);

        map<int, map<string, double>> pattern_check_tf;
        pattern_check_tf[0] = {{"пушистый", 0 / 4.0}, {"ухоженный"s, 0 / 4.0}, {"кот", 1 / 4.0}};
        pattern_check_tf[1] = {{"пушистый", 2 / 4.0}, {"ухоженный"s, 0 / 4.0}, {"кот", 1 / 4.0}};
        pattern_check_tf[2] = {{"пушистый", 0 / 4.0}, {"ухоженный"s, 1 / 4.0}, {"кот", 0 / 4.0}};

        vector<Document> pattern_check_relevanceFordocuments;
        for (const auto &[id, x] : pattern_check_tf)
        {
            double relevante = x.at("пушистый") * pattern_check_idf.at("пушистый") +
                               x.at("ухоженный") * pattern_check_idf.at("ухоженный") +
                               x.at("кот") * pattern_check_idf.at("кот");
            double sum = 0;
            int rating = accumulate(rating_pattern.at(id).begin(), rating_pattern.at(id).end(), 0)/rating_pattern.at(id).size();

            pattern_check_relevanceFordocuments.push_back({id, relevante, rating});
        }

        SearchServer search_server;
        search_server.SetStopWords("и в на"s);
        search_server.AddDocument(0, pattern_check[0], DocumentStatus::ACTUAL, rating_pattern[0]);
        search_server.AddDocument(1, pattern_check[1], DocumentStatus::ACTUAL, rating_pattern[1]);
        search_server.AddDocument(2, pattern_check[2], DocumentStatus::ACTUAL, rating_pattern[2]);
        vector<Document> document = search_server.FindTopDocuments("пушистый ухоженный кот"s);
        ASSERT_EQUAL(document.size(), 3);
        for (const Document& patt_doc : pattern_check_relevanceFordocuments)
        {
            for (const Document &doc : document)
            {
                if (patt_doc.id == doc.id)
                {
                    ASSERT_HINT(abs(doc.relevance - patt_doc.relevance) < EPSILON, "Тест: колькуляции релевантности"s);
                }
            }
        }
    }
}

void TestCalculatedAvgRatingFromFindTopDocuments()
{
    {
        SearchServer search_server;
        search_server.SetStopWords("и в на"s);
        map<int, int> pattern_avg_rating_document;
        vector<int> buff;

        buff = {8, -3};
        search_server.AddDocument(0, "белый кот и модный ошейник"s, DocumentStatus::ACTUAL, buff);
        pattern_avg_rating_document[0] = accumulate(buff.begin(), buff.end(), 0) / buff.size();

        buff = {7, 2, 7};
        search_server.AddDocument(1, "пушистый кот пушистый хвост"s, DocumentStatus::ACTUAL, buff);
        pattern_avg_rating_document[1] = accumulate(buff.begin(), buff.end(), 0) / buff.size();

        buff = {5, -12, 2, 1};
        search_server.AddDocument(2, "ухоженный пёс выразительные глаза"s, DocumentStatus::ACTUAL, buff);
        pattern_avg_rating_document[2] = accumulate(buff.begin(), buff.end(), 0) / buff.size();

        buff = {9};
        search_server.AddDocument(3, "ухоженный скворец евгений"s, DocumentStatus::BANNED, buff);
        pattern_avg_rating_document[3] = accumulate(buff.begin(), buff.end(), 0) / buff.size();

        vector<Document> document = search_server.FindTopDocuments("пушистый ухоженный кот"s);

        ASSERT_EQUAL(document.size(), 3);
        ASSERT_HINT(document[0].id == 1 && document[0].rating == pattern_avg_rating_document.at(1), "Тест:рейтинг:вычисление среднего арефметического");
        ASSERT_HINT(document[1].id == 0 && document[1].rating == pattern_avg_rating_document.at(0), "Тест:рейтинг:вычисление среднего арефметического");
        ASSERT_HINT(document[2].id == 2 && document[2].rating == pattern_avg_rating_document.at(2), "Тест:рейтинг:вычисление среднего арефметического");
    }
}

void TestPredicateFromFindTopDocuments()
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
        ASSERT_HINT(document.size() == 0, "Таких документов нет");
    }
}

// Функция TestSearchServer является точкой входа для запуска тестов
void TestSearchServer()
{
    RUN_TEST(TestExcludedStopWordsFromAddedDocumentContent);
    RUN_TEST(TestExcludedSentencesMinusWordsFromAddedDocumentContent);
    RUN_TEST(TestMatchedDocumetsFromMatchDocument);
    RUN_TEST(TestCalculatedRelevanceFromFindTopDocuments);
    RUN_TEST(TestSortedRelevanceFromFindTopDocuments);
    RUN_TEST(TestCalculatedAvgRatingFromFindTopDocuments);
    RUN_TEST(TestPredicateFromFindTopDocuments);
    RUN_TEST(TestCheckedPushDocuments);
    RUN_TEST(TestGotStatusDocumentsFromFindTopDocuments);
    // Не забудьте вызывать остальные тесты здесь
}

// --------- Окончание модульных тестов поисковой системы -----------