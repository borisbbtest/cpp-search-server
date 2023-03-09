#include <iostream>
#include <vector>
#include <algorithm>
#include <cassert>
#include <iterator>


using namespace std;

class MoneyBox
{
public:
    explicit MoneyBox(vector<int64_t> nominals)
        : nominals_(move(nominals)), counts_(nominals_.size())
    {
    }

    const vector<int> &GetCounts() const
    {
        return counts_;
    }

    int getIndex(int64_t i)
    {
        auto res=find(nominals_.begin(),nominals_.end(),i);
        return res-nominals_.begin();
    }
    void PushCoin(int64_t value)
    {
        // реализуйте метод добавления купюры или монеты
        // assert(value >= 0 && value <= 5000);
        ++counts_[getIndex(value)];
    }

    void PrintCoins(ostream &out) const
    {
        // реализуйте метод печати доступных средств
        for (int i = 0; i < counts_.size(); ++i)
        {
            if (counts_[i] > 0)
            {
                out << *(nominals_.begin()+i)<< ": "s << counts_[i] << endl;
            }
        }
    }

private:
    const vector<int64_t> nominals_;
    vector<int> counts_;
};

ostream &operator<<(ostream &out, const MoneyBox &cash)
{
    cash.PrintCoins(out);
    return out;
}

int main()
{
    MoneyBox cash({1, 500, 10000});
    cash.PushCoin(500);
    cash.PushCoin(500);
    cash.PushCoin(10000);
    assert((cash.GetCounts() == vector<int>{0, 2, 1}));
    cout << cash << endl;
}