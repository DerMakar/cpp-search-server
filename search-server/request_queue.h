#pragma once
#include "search_server.h"
#include <stack>

class RequestQueue {
public:
    explicit RequestQueue(const SearchServer& search_server);
   
    template <typename DocumentPredicate>
    std::vector<Document> AddFindRequest(const std::string& raw_query, DocumentPredicate document_predicate);
    std::vector<Document> AddFindRequest(const std::string& raw_query, DocumentStatus status);
    std::vector<Document> AddFindRequest(const std::string& raw_query);
    int GetNoResultRequests() const;
private:
    struct QueryResult {
        std::vector<Document> request;
        bool empt = true;
    };
    std::deque<QueryResult> requests_;
    const static int min_in_day_ = 1440;
    const SearchServer& search_server_;
    int emptys = 0;
};

template <typename DocumentPredicate>
    std::vector<Document> RequestQueue::AddFindRequest(const std::string& raw_query, DocumentPredicate document_predicate) {
        std::vector<Document> result = search_server_.FindTopDocuments(raw_query, document_predicate);
        if (result.empty()){
            ++RequestQueue::emptys;
        }
        
        if (requests_.size() < RequestQueue::min_in_day_){
            QueryResult query_result = {result, result.empty()};
            requests_.push_back(x);
        }else if (RequestQueue::requests_.size() == RequestQueue::min_in_day_){
            if(requests_.front().empt){
                --RequestQueue::emptys;
            }
           requests_.pop_front();
           QueryResult x = {result, result.empty()};
           requests_.push_back(x);
        }
        return result;
    }
