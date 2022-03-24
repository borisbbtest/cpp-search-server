#include <iostream>
#include <string>

using namespace  std;
int main (){
    int count_three=0;
    for(int i=1;i<1000;i++){
        string buff=to_string(i);
        if (buff.find('3')!= string::npos)
            count_three++;
    }
    cout<<"fish code "<<count_three<<endl;
    return 0;
}
// Решите загадку: Сколько чисел от 1 до 1000 содержат как минимум одну цифру 3?
// Напишите ответ здесь: 271
// Закомитьте изменения и отправьте их в свой репозиторий.
