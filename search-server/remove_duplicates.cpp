#include "remove_duplicates.h"

//вне класса сервера функция удаления дубликатов - реализация
void RemoveDuplicates(SearchServer& search_server){
    
    // будем формировать пары с id и набором слов документа
    std::vector<std::pair<int, std::vector<std::string>>> docs_struct;
    
    for (const int document_id : search_server){
        std::vector<std::string> key_words;
        std::map<std::string, double> id_freq = search_server.GetWordFrequencies(document_id);
        for (auto it = id_freq.begin(); it != id_freq.end(); ++it){
            key_words.push_back(it->first);
        }
    docs_struct.push_back(std::make_pair(document_id, key_words));    
    }
   
    for (auto i = 0; i < docs_struct.size() -1; ++i){
        for(auto j = i +1; j < docs_struct.size(); ++j){
            if (docs_struct[i].second == docs_struct[j].second){
                std::cout << "Found duplicate document id "s << docs_struct[j].first << std::endl;
            search_server.RemoveDocument(docs_struct[j].first);
            docs_struct.erase(docs_struct.begin() + j);
            j--;
            }
        }
    }
}