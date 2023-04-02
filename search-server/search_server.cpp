#include "search_server.h"
#include "process_queries.h"
#include "concurrent_map.h"

SearchServer::SearchServer(const std::string_view stop_words_text)
        : SearchServer::SearchServer(SplitIntoWords(stop_words_text)){
    }

SearchServer::SearchServer(const std::string& stop_word_text) 
        : SearchServer::SearchServer(SplitIntoWords(stop_word_text)){
    }

void SearchServer::AddDocument(int document_id, std::string_view document,
                               DocumentStatus status,
                               const std::vector<int>& ratings) {
        if ((document_id < 0) || (documents_.count(document_id) > 0)) {
            throw std::invalid_argument("Invalid document_id"s);
        }
        DocumentData doc_{0,status, static_cast<std::string>(document) };
        std::vector<std::string_view> words = SplitIntoWordsNoStop(doc_.document_content);

        const double inv_word_count = 1.0 / words.size();
        for (std::string_view word : words) {
            
            word_to_document_freqs_[word][document_id] += inv_word_count;
            //в хидере объявили словарь id и сета слов. Здесь в него добавляем
            id_with_words_freq[document_id][word] += inv_word_count;
            
        }
        documents_.emplace(document_id, SearchServer::DocumentData{SearchServer::ComputeAverageRating(ratings), status});
        docs_index.insert(document_id);
    }

std::vector<Document> SearchServer::FindTopDocuments(std::string_view raw_query, DocumentStatus status) const {
        return SearchServer::FindTopDocuments(
            raw_query, [status](int document_id, DocumentStatus document_status, int rating) {
                return document_status == status;
            });
    }

std::vector<Document> SearchServer::FindTopDocuments(std::string_view raw_query) const {
        return SearchServer::FindTopDocuments(raw_query, DocumentStatus::ACTUAL);
    }



size_t SearchServer::GetDocumentCount() const {
        return documents_.size();
    }

// заменяем метод GetDocumentId(int index) на итераторы
std::set<int>::const_iterator SearchServer::begin() const
{
	return docs_index.begin();
}

std::set<int>::const_iterator SearchServer::end() const
{
	return docs_index.end();
}

std::set<int>::iterator SearchServer::begin()
{
	return docs_index.begin();
}

std::set<int>::iterator SearchServer::end()
{
	return docs_index.end();
}



//реализация метода удаления
void SearchServer::RemoveDocument(int document_id){
    if(id_with_words_freq.count(document_id) == 0){
        return;
    }
    for (const auto& [word, freq] : id_with_words_freq.at(document_id)){
    //std::map<std::string, std::map<int, double>> word_to_document_freqs_;
    //std::map<int, std::map<std::string, double>> id_with_words_freq;
    auto it = word_to_document_freqs_.at(word).find(document_id);
        if (it != word_to_document_freqs_.at(word).end()){
            word_to_document_freqs_.at(word).erase(it);
        }
    }
    docs_index.erase(find(docs_index.begin(),docs_index.end(),document_id));
    id_with_words_freq.erase(document_id);
    documents_.erase(document_id);
}

void SearchServer::RemoveDocument(std::execution::sequenced_policy policy, int document_id){
    RemoveDocument (document_id);
}

void SearchServer::RemoveDocument(const std::execution::parallel_policy& policy, int document_id){
    if(id_with_words_freq.count(document_id) == 0){
        return;
    }
        std::vector<std::string_view> strings_(id_with_words_freq.at(document_id).size());
    transform(policy, id_with_words_freq.at(document_id).begin(), id_with_words_freq.at(document_id).end(), strings_.begin(), [](auto document_){return document_.first;});
    auto p = [this, document_id](std::string_view str_){word_to_document_freqs_.at(str_).erase(document_id);};
    for_each(policy, strings_.begin(), strings_.end(), p);
    docs_index.erase(find(policy, docs_index.begin(),docs_index.end(),document_id));
    id_with_words_freq.erase(document_id);
    documents_.erase(document_id);
}


std::tuple<std::vector<std::string_view>, DocumentStatus> SearchServer::MatchDocument(std::string_view raw_query,
                                                        int document_id) const {
        if (0 == documents_.count(document_id)) {
        throw std::out_of_range("There is no document with this document_id"s);
        }
        const auto query = ParseQuery(raw_query);
        for (const std::string_view& word : query.minus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            if (word_to_document_freqs_.at(word).count(document_id)) {
                return { {},documents_.at(document_id).status };
            }
        }
        std::vector<std::string_view> matched_words;
        for (const std::string_view& word : query.plus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            if (word_to_document_freqs_.at(word).count(document_id)) {
                matched_words.push_back(word);
            }
        }
       
        return {matched_words, documents_.at(document_id).status};
    }

SearchServer::DocTuple SearchServer::MatchDocument(const std::execution::sequenced_policy&, std::string_view raw_query, int document_id) const {
    return MatchDocument(raw_query, document_id);
}
   
SearchServer::DocTuple SearchServer::MatchDocument(const std::execution::parallel_policy& policy, std::string_view raw_query, int document_id) const {
    if (0 == documents_.count(document_id)) {
        throw std::out_of_range("There is no document with this document_id"s);
    }
    const auto query = ParseQuery(raw_query,1);
    
    if (std::any_of(std::execution::par, query.minus_words.begin(), query.minus_words.end(), [this, document_id](const auto& word) {return word_to_document_freqs_.count(word) != 0 && word_to_document_freqs_.at(word).count(document_id); })) {
        return { {},documents_.at(document_id).status };
    }
    
        std::vector<std::string_view> matched_words (query.plus_words.size());
        auto it = std::copy_if (policy,
                     query.plus_words.begin(),
                     query.plus_words.end(),
                     matched_words.begin(),
                     [this, document_id](const auto& word){
                         return       word_to_document_freqs_.count(word) != 0 && word_to_document_freqs_.at(word).count(document_id);}
        );
    matched_words.erase(it, matched_words.end());
    sort(policy, matched_words.begin(),
        matched_words.end()
        );
    
    matched_words.erase(unique(policy, matched_words.begin(), matched_words.end()), matched_words.end());
        return {matched_words, documents_.at(document_id).status};
    }


bool SearchServer::IsStopWord(std::string_view word) const {
        return stop_words_.count(word) > 0;
    }

bool SearchServer::IsValidWord(std::string_view word) {
        return std::none_of(word.begin(), word.end(), [](char c) {
            return c >= '\0' && c < ' ';
        });
    }

std::vector<std::string_view> SearchServer::SplitIntoWordsNoStop(std::string_view text) const {
        std::vector<std::string_view> words;
        for (std::string_view word : SplitIntoWords(text)) {
            if (!IsValidWord(word)) {
                throw std::invalid_argument("Word "s + static_cast<std::string>(word) + " is invalid"s);
            }
            if (!IsStopWord(word)) {
                words.push_back(word);
            }
        }
        return words;
    }

//реализация метода поиска частот
const std::map<std::string_view, double>& SearchServer::GetWordFrequencies(int document_id) const{
    static std::map<std::string_view, double> word_freq;
    if (id_with_words_freq.count(document_id) == 0){
        return word_freq;
    }
    return id_with_words_freq.at(document_id);
        
}

// создаем функцию для возврата приватного id
// vector<int> GetId(){return  docs_index}

int SearchServer::ComputeAverageRating(const std::vector<int>& ratings) {
        if (ratings.empty()) {
            return 0;
        }
        int rating_sum = 0;
        for (const int rating : ratings) {
            rating_sum += rating;
        }
        return rating_sum / static_cast<int>(ratings.size());
    }

SearchServer::QueryWord SearchServer::ParseQueryWord(std::string_view text) const {
               
    bool is_minus = false;
        if (text[0] == '-') {
            is_minus = true;
            text.remove_prefix(1);
        }
     
    if (text.empty()) {
            throw std::invalid_argument("Query word is empty"s);
        }
    
        if (text[0] == '-' || !IsValidWord(text)) {
            throw std::invalid_argument("Query word "s + static_cast<std::string>(text) + " is invalid");
        }

        return {text, is_minus, IsStopWord(text)};
    }

SearchServer::Query SearchServer::ParseQuery(std::string_view text, bool policy = 0) const {
    if (text.empty()) {
        throw std::invalid_argument("Query word is empty"s);
    }
    Query result;
    for (const std::string_view& word : SplitIntoWords(text)) {
        const auto query_word = ParseQueryWord(word);
        if (!query_word.is_stop) {
            if (query_word.is_minus) {
                result.minus_words.push_back(query_word.data);
            }
            else {
                result.plus_words.push_back(query_word.data);
            }
        }
    }
    if (!policy) {
        sort(result.minus_words.begin(), result.minus_words.end());
        sort(result.plus_words.begin(), result.plus_words.end());
        result.minus_words.erase(unique(result.minus_words.begin(), result.minus_words.end()), result.minus_words.end());
        result.plus_words.erase(unique(result.plus_words.begin(), result.plus_words.end()), result.plus_words.end());
        return result;
    }
    else {
        return result;
    }
}

double SearchServer::ComputeWordInverseDocumentFreq(const std::string_view word) const {
        return log(GetDocumentCount() * 1.0 / word_to_document_freqs_.at(word).size());
    }