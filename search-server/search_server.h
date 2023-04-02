#pragma once
#include "string_processing.h"
#include "document.h"
#include "read_input_functions.h"
#include "concurrent_map.h"
#include <map>
#include <algorithm>
#include <cmath>
#include <execution>
#include <vector>
#include <deque>


using namespace std::literals;

const int MAX_RESULT_DOCUMENT_COUNT = 5;

class SearchServer {
public:
    template <typename StringContainer>
    explicit SearchServer(const StringContainer& stop_words);
    explicit SearchServer(const std::string_view stop_words_text);
    explicit SearchServer(const std::string& stop_word_text);
    void AddDocument(int document_id, std::string_view document, DocumentStatus status,
                     const std::vector<int>& ratings);

    template <typename DocumentPredicate>
    std::vector<Document> FindTopDocuments(std::string_view raw_query,
                                      DocumentPredicate document_predicate) const;

    template <typename ExecutionPolicy, typename DocumentPredicate>
    std::vector<Document> FindTopDocuments(const ExecutionPolicy& policy, std::string_view raw_query,
        DocumentPredicate document_predicate) const;

   
    std::vector<Document> FindTopDocuments(std::string_view raw_query, DocumentStatus status) const;

    template <typename ExecutionPolicy>
    std::vector<Document> FindTopDocuments(const ExecutionPolicy& policy, std::string_view raw_query, DocumentStatus status) const;

    std::vector<Document> FindTopDocuments(std::string_view raw_query) const;
    
    template <typename ExecutionPolicy>
    std::vector<Document> FindTopDocuments(const ExecutionPolicy& policy, std::string_view raw_query) const;
    
    size_t GetDocumentCount() const;

    //Объявили в хидере методы
    std::set<int>::const_iterator begin() const;
    std::set<int>::const_iterator end() const;
    std::set<int>::iterator begin();
    std::set<int>::iterator end();
    
    // Объявляем метод удаления документа
    void RemoveDocument(int document_id);
    void RemoveDocument(std::execution::sequenced_policy, int document_id);
    void RemoveDocument(const std::execution::parallel_policy& policy, int document_id);
   
    using DocTuple = std::tuple<std::vector<std::string_view>, DocumentStatus>;

    DocTuple MatchDocument(std::string_view raw_query, int document_id) const;
    DocTuple MatchDocument(const std::execution::sequenced_policy&, std::string_view raw_query, int document_id) const;
    DocTuple MatchDocument(const std::execution::parallel_policy&, std::string_view raw_query, int document_id) const;

     //объявляем метод получения частот слов по id
    const std::map<std::string_view, double>& GetWordFrequencies(int document_id) const;
    
private:
    struct DocumentData {
        int rating;
        DocumentStatus status;
        std::string document_content;
    };
    const std::set<std::string, std::less<>> stop_words_;
    std::map<std::string_view, std::map<int, double>> word_to_document_freqs_;
    std::map<int, std::map<std::string_view, double>> id_with_words_freq;
    std::map<int, DocumentData> documents_;
    std::set<int> docs_index;

    bool IsStopWord(std::string_view word) const;

    static bool IsValidWord(std::string_view word);

    std::vector<std::string_view> SplitIntoWordsNoStop(std::string_view text) const;

   
     
    static int ComputeAverageRating(const std::vector<int>& ratings);

    struct QueryWord {
        std::string_view data;
        bool is_minus;
        bool is_stop;
    };

    QueryWord ParseQueryWord(std::string_view text) const ;

    struct Query {
        std::vector<std::string_view> plus_words;
        std::vector<std::string_view> minus_words;
    };

    Query ParseQuery(std::string_view text, bool policy = 0) const ;
   
    double ComputeWordInverseDocumentFreq(const std::string_view word) const ;

    template <typename DocumentPredicate>
    std::vector<Document> FindAllDocuments(const Query& query,
                                      DocumentPredicate document_predicate) const;

    template <typename ExecutionPolicy, typename DocumentPredicate>
    std::vector<Document> FindAllDocuments(const ExecutionPolicy& policy,
        const Query& query, DocumentPredicate document_predicate) const;
};

// исполняемые шаблоны

 template <typename StringContainer>
    SearchServer::SearchServer(const StringContainer& stop_words)
        : stop_words_(MakeUniqueNonEmptyStrings(stop_words))  // Extract non-empty stop words
    {
        if (!all_of(stop_words_.begin(), stop_words_.end(), IsValidWord)) {
            throw std::invalid_argument("Some of stop words are invalid"s);
        }
    }

    const uint32_t epsilon = 1e-6;
   
 template <typename DocumentPredicate>
    std::vector<Document>  SearchServer::FindTopDocuments( std::string_view raw_query, DocumentPredicate document_predicate) const {
        const auto query = ParseQuery(raw_query);

        auto matched_documents = SearchServer::FindAllDocuments(query, document_predicate);

        sort(matched_documents.begin(), matched_documents.end(),
             [](const Document& lhs, const Document& rhs) {
                 if (std::abs(lhs.relevance - rhs.relevance) < epsilon) {
                     return lhs.rating > rhs.rating;
                 } else {
                     return lhs.relevance > rhs.relevance;
                 }
             });
        if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
            matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
        }

        return matched_documents;
    }

    template <typename ExecutionPolicy, typename DocumentPredicate>
    std::vector<Document> SearchServer::FindTopDocuments(const ExecutionPolicy& policy, std::string_view raw_query,
        DocumentPredicate document_predicate) const {
        if constexpr (std::is_same_v<ExecutionPolicy, std::execution::sequenced_policy>) {
            return SearchServer::FindTopDocuments(raw_query, document_predicate);
        }
        else {
            const auto query = ParseQuery(raw_query);

            auto matched_documents = SearchServer::FindAllDocuments(policy, query, document_predicate);
            std::sort(policy, matched_documents.begin(), matched_documents.end(),
                [](const Document& lhs, const Document& rhs) {
                    if (std::abs(lhs.relevance - rhs.relevance) < epsilon) {
                        return lhs.rating > rhs.rating;
                    }
                    else {
                        return lhs.relevance > rhs.relevance;
                    }
                });
            if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
                matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
            }
            return matched_documents;
        }
    }
 
template <typename ExecutionPolicy>
    std::vector<Document> SearchServer::FindTopDocuments(const ExecutionPolicy& policy,
                                                        std::string_view raw_query,
                                                        DocumentStatus status) const {
        if constexpr (std::is_same_v<ExecutionPolicy, std::execution::sequenced_policy>) {
        return SearchServer::FindTopDocuments(raw_query, status);
        }
        else {
        return SearchServer::FindTopDocuments(policy,
            raw_query, [status](int document_id, DocumentStatus document_status, int rating) {
            return document_status == status;
            });
        }
    }

template <typename ExecutionPolicy>
    std::vector<Document> SearchServer::FindTopDocuments(const ExecutionPolicy& policy, std::string_view raw_query) const {
        if constexpr (std::is_same_v<ExecutionPolicy, std::execution::sequenced_policy>) {
        return SearchServer::FindTopDocuments(raw_query);
        }
        else {
        return SearchServer::FindTopDocuments(policy, raw_query, DocumentStatus::ACTUAL);
        }
    }

template <typename DocumentPredicate>
    std::vector<Document> SearchServer::FindAllDocuments(const Query& query, DocumentPredicate document_predicate) const {
        std::map<int, double> document_to_relevance;
        for (std::string_view word : query.plus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            const double inverse_document_freq = SearchServer::ComputeWordInverseDocumentFreq(word);
            for (const auto& [document_id, term_freq] : word_to_document_freqs_.at(word)) {
                const auto& document_data = documents_.at(document_id);
                if (document_predicate(document_id, document_data.status, document_data.rating)) {
                    document_to_relevance[document_id] += term_freq * inverse_document_freq;
                }
            }
        }

        for (const std::string_view& word : query.minus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            for (const auto [document_id, _] : word_to_document_freqs_.at(word)) {
                document_to_relevance.erase(document_id);
            }
        }

        std::vector<Document> matched_documents;
        for (const auto [document_id, relevance] : document_to_relevance) {
            matched_documents.push_back(
                {document_id, relevance, documents_.at(document_id).rating});
        }
        return matched_documents;
    }

    template <typename ExecutionPolicy, typename DocumentPredicate>
    std::vector<Document> SearchServer::FindAllDocuments(const ExecutionPolicy& policy, const Query& query, DocumentPredicate document_predicate) const {
       ConcurrentMap<int, double> document_to_relevance(7);
       for_each(policy, 
                query.plus_words.begin(), query.plus_words.end(), 
                [&, document_predicate] (auto word) {
                    if (word_to_document_freqs_.count(word) != 0) {
                        const double inverse_document_freq = SearchServer::ComputeWordInverseDocumentFreq(word);
                        for_each(policy,
                                word_to_document_freqs_.at(word).begin(),
                                word_to_document_freqs_.at(word).end(),
                                [&document_to_relevance, this, &document_predicate, &inverse_document_freq](const auto& info_) {
                                const auto& document_data = documents_.at(info_.first);
                                if (document_predicate(info_.first, document_data.status, document_data.rating)) {
                                    document_to_relevance[info_.first].ref_to_value += info_.second * inverse_document_freq;
                                }
                        });
                    }
                }
        );

            for_each(policy, 
                query.minus_words.begin(), query.minus_words.end(),
                [&](const auto& word) {
                    if (word_to_document_freqs_.count(word) != 0) {
                        for_each(policy,
                                word_to_document_freqs_.at(word).begin(),
                                word_to_document_freqs_.at(word).end(),
                                [&document_to_relevance, this, &word](const auto& info_) {
                                document_to_relevance.Delete(info_.first);
                        });
                    }
                });
                       
            std::vector<Document> matched_documents;
            for (const auto& [document_id, relevance] : document_to_relevance.BuildOrdinaryMap()) {
                matched_documents.push_back(
                    { document_id, relevance, documents_.at(document_id).rating });
            }
            return matched_documents;
        }
    