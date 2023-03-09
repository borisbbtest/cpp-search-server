#include <iostream>
#include <string>
#include <sstream>
#include <unordered_set>
#include <unordered_map>
using namespace std;

template <typename Hash>
int FindCollisions(const Hash &hasher, istream &text)
{

    // int collisions = 0;
    // unordered_map<size_t, unordered_set<string>> container;
    // string word;
    
    // while (text >> word) {
    //     size_t hash = hasher(word);
    //     auto [inserted_word, collision_bool] = container[hash].insert(move(word));
    //     collisions += collision_bool && container.at(hash).size() > 1;
    // }
    
    //return collisions;
    // место для вашей реализации
    string word;
    int sum = 0;
    unordered_map<size_t, unordered_set<string>> words;

    while (text >> word)
    {
        size_t x = hasher(word);
        unordered_set<string> *b = &words[x];
        auto [inserted_word, add_bool]  = b->insert(std::move (word));
        if(b->size()>1 && add_bool){
            sum++;
        }
    }
    // for (auto [k, v] : words)
    // {
    //     cout << "\nhash = " << k << endl;
    //     for (auto itr = v.begin(); itr != v.end(); ++itr)
    //     {
    //         cout <<" "<< "-"<< *itr;
    //     }
    //     cout<<endl;
    // }
    return sum;
}

// Это плохой хешер. Его можно использовать для тестирования.
// Подумайте, в чём его недостаток
struct HasherDummy
{
    size_t operator()(const string &str) const
    {
        size_t res = 0;
        for (char c : str)
        {
            res += static_cast<size_t>(c);
        }
        return res;
    }
};

struct DummyHash
{
    size_t operator()(const string &) const
    {
        return 42;
    }
};
int main()
{
    hash<string> str_hasher;
    int collisions = FindCollisions(str_hasher, cin);
    HasherDummy dummy_hash;
    DummyHash dummy_hash2;
    hash<string> good_hash;
    cout << "Found collisions: "s << collisions << endl;

    {
        istringstream stream("I love C++"s);
        cout << "dumpy hash-" << FindCollisions(dummy_hash, stream) << endl;
    }

    {
        istringstream stream("I love C++"s);
        cout << "good hash-" << FindCollisions(good_hash, stream) << endl;
    }

    {
        istringstream stream("I love C++"s);
        cout << "dummy hash2-" << FindCollisions(dummy_hash2, stream) << endl;
    }
}