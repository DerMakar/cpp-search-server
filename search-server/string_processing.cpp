#include "string_processing.h"

using namespace std;

vector<string_view> SplitIntoWords(string_view str) {
    
    vector<string_view> result;
    //Удалите начало из str до первого непробельного символа
    str.remove_prefix(min(str.size(),str.find_first_not_of(" ")));
    while (!str.empty()) {
        // В цикле используйте метод find с одним параметром, чтобы найти номер позиции первого пробела.
       int64_t space = str.find(' ');
       
        //Добавьте в результирующий вектор элемент string_view, полученный вызовом метода substr, где начальная позиция будет 0, а конечная — найденная позиция пробела или npos.
        
        result.push_back(str.substr(0, space));
    
        //Сдвиньте начало str так, чтобы оно указывало на позицию за пробелом. Это можно сделать методом remove_prefix, передвигая начало str на указанное в аргументе количество позиций
        //Чтобы не удалить в методе remove_prefix больше символов, чем есть в строке, используйте алгоритм min: str.remove_prefix(min(str.size(), ...))
        str.remove_prefix(min(str.size(), str.find_first_not_of(" ", space)));
        
    }

    return result;
}