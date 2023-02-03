#include "remove_duplicates.h"

//вне класса сервера функция удаления дубликатов - реализация
void RemoveDuplicates(SearchServer& search_server){
    std::vector<int> ids_for_removing;
    for (const int document_id : search_server) {
        auto iter = find_if(search_server.begin(), search_server.end(), [document_id] (std::set<std::string> words) {return SearchServer::id_with_words.at(document_id) == words;});
        if(*iter > document_id){
            ids_for_removing.push_back(*iter);
        }
    }
    for (int id :ids_for_removing){
    std::cout << "Found duplicate document id " << id << std::endl;
    SearchServer::RemoveDocument(id);   
    }
    ids_for_removing.clear();
}