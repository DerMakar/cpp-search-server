#include "search_server.h"

SearchServer::SearchServer(const std::string& stop_words_text)
        : SearchServer::SearchServer(SplitIntoWords(stop_words_text)){
    }

void SearchServer::AddDocument(int document_id, const std::string& document,
                               DocumentStatus status,
                               const std::vector<int>& ratings) {
        if ((document_id < 0) || (documents_.count(document_id) > 0)) {
            throw std::invalid_argument("Invalid document_id"s);
        }
        const auto words = SplitIntoWordsNoStop(document);

        const double inv_word_count = 1.0 / words.size();
        for (const std::string& word : words) {
            word_to_document_freqs_[word][document_id] += inv_word_count;
            //в хидере объявили словарь id и сета слов. Здесь в него добавляем
            id_with_words_freq[document_id][word] += inv_word_count;
            
        }
        documents_.emplace(document_id, SearchServer::DocumentData{SearchServer::ComputeAverageRating(ratings), status});
        docs_index.insert(document_id);
    }

std::vector<Document> SearchServer::FindTopDocuments(const std::string& raw_query, DocumentStatus status) const {
        return SearchServer::FindTopDocuments(
            raw_query, [status](int document_id, DocumentStatus document_status, int rating) {
                return document_status == status;
            });
    }

std::vector<Document> SearchServer::FindTopDocuments(const std::string& raw_query) const {
        return SearchServer::FindTopDocuments(raw_query, DocumentStatus::ACTUAL);
    }

int SearchServer::GetDocumentCount() const {
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
    for (auto [word, freq] : id_with_words_freq.at(document_id)){
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

std::tuple<std::vector<std::string>, DocumentStatus> SearchServer::MatchDocument(const std::string& raw_query,
                                                        int document_id) const {
        const auto query = ParseQuery(raw_query);

        std::vector<std::string> matched_words;
        for (const std::string& word : query.plus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            if (word_to_document_freqs_.at(word).count(document_id)) {
                matched_words.push_back(word);
            }
        }
        for (const std::string& word : query.minus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            if (word_to_document_freqs_.at(word).count(document_id)) {
                matched_words.clear();
                break;
            }
        }
        return {matched_words, documents_.at(document_id).status};
    }

bool SearchServer::IsStopWord(const std::string& word) const {
        return stop_words_.count(word) > 0;
    }

bool SearchServer::IsValidWord(const std::string& word) {
        return none_of(word.begin(), word.end(), [](char c) {
            return c >= '\0' && c < ' ';
        });
    }

std::vector<std::string> SearchServer::SplitIntoWordsNoStop(const std::string& text) const {
        std::vector<std::string> words;
        for (const std::string& word : SplitIntoWords(text)) {
            if (!IsValidWord(word)) {
                throw std::invalid_argument("Word "s + word + " is invalid"s);
            }
            if (!IsStopWord(word)) {
                words.push_back(word);
            }
        }
        return words;
    }

//реализация метода поиска частот
const std::map<std::string, double>& SearchServer::GetWordFrequencies(int document_id) const{
    static std::map<std::string, double> word_freq;
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

SearchServer::QueryWord SearchServer::ParseQueryWord(const std::string& text) const {
        if (text.empty()) {
            throw std::invalid_argument("Query word is empty"s);
        }
        std::string word = text;
        bool is_minus = false;
        if (word[0] == '-') {
            is_minus = true;
            word = word.substr(1);
        }
        if (word.empty() || word[0] == '-' || !IsValidWord(word)) {
            throw std::invalid_argument("Query word "s + text + " is invalid");
        }

        return {word, is_minus, IsStopWord(word)};
    }

SearchServer::Query SearchServer::ParseQuery(const std::string& text) const {
        Query result;
        for (const std::string& word : SplitIntoWords(text)) {
            const auto query_word = ParseQueryWord(word);
            if (!query_word.is_stop) {
                if (query_word.is_minus) {
                    result.minus_words.insert(query_word.data);
                } else {
                    result.plus_words.insert(query_word.data);
                }
            }
        }
        return result;
    }

double SearchServer::ComputeWordInverseDocumentFreq(const std::string& word) const {
        return log(GetDocumentCount() * 1.0 / word_to_document_freqs_.at(word).size());
    }

