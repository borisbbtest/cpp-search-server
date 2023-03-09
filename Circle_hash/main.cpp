#include <iostream>
#include <string>

using namespace std;

struct Circle
{
    double x;
    double y;
    double r;
};

struct Dumbbell
{
    Circle circle1;
    Circle circle2;
    string text;
};

struct DumbbellHash
{
    // реализуйте хешер для Dumbbell
    size_t operator()(const Dumbbell &dumll) const
    {
        size_t res=0;
        size_t h1_x =  d_hasher_(dumll.circle1.x);
        size_t h1_y =  d_hasher_(dumll.circle1.y);
        size_t h1_r =  d_hasher_(dumll.circle1.r);

        size_t h2_x = d_hasher_(dumll.circle2.x);
        size_t h2_y = d_hasher_(dumll.circle2.y);
        size_t h2_r =  d_hasher_(dumll.circle2.r);

        size_t str_h1 = str_hasher_(dumll.text);
        size_t h1= h1_x + h1_y * 37 + h1_r * (37 * 37);
        size_t h2 = h2_x + h2_y * 37 + h2_r * (37 * 37);
        
        res = (h1*37*37*37*37)+(h2*37)+str_h1 ;
        return res ;
    }
    std::hash<double> d_hasher_;
    std::hash<string> str_hasher_;

};

int main()
{
    DumbbellHash hash;
    Dumbbell dumbbell{{10, 11.5, 2.3}, {3.14, 15, -8}, "abc"s};
    cout << "Dumbbell hash "s << hash(dumbbell);
}