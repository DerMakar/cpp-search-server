#include "remove_duplicates.h"

//вне класса сервера функция удаления дубликатов - реализация
void RemoveDuplicates(SearchServer& search_server){
    
    // будем формировать пары с id и набором слов документа
    std::vector<std::vector<std::string>> docs_struct;
    
    for (const int document_id : search_server){
        std::vector<std::string> key_words;
        std::map<std::string, double> id_freq = search_server.GetWordFrequencies(document_id);
        std::transform(id_freq.begin(), id_freq.end(), std::back_inserter(key_words), [](std::pair<std::string, double> word) { return word.first;});
        docs_struct.push_back(key_words);    
    }
   
    for (auto i = 0; i < docs_struct.size(); ++i){
        auto it = find(docs_struct.begin() + i + 1,docs_struct.end(), docs_struct[i]);
        if (it != docs_struct.end()){
            int id = distance(docs_struct.begin(), it) + 1;
            std::cout << "Found duplicate document id "s << id << std::endl;
            search_server.RemoveDocument(id);
            
        }
    }
}