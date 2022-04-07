#include <string>
#include <vector>
#include <iostream>
#include <algorithm>
using namespace std;
int main() {
    int count;
    cin >> count;
    vector<pair<int, string>> who;
    for (int i = 0; i < count; ++i) {
        string name;
        int age;
        cin >> name >> age;
        who.push_back({age, name});
        name.clear();
        // сохраните в вектор пар
    }
    reverse(who.begin(), who.end());
    for (auto [x, y] : who) {
        cout << x << " " << y << endl;
    }
    // выведите только имена в порядке убывания возраста
    // Jack
    // John
    // ...
    return 0;
}