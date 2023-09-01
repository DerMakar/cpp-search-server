#include "process_queries.h"

#include <execution>
#include <functional>


std::vector<std::vector<Document>> ProcessQueries(
    const SearchServer& search_server,
    const std::vector<std::string>& queries){
    std::vector<std::vector<Document>> results(queries.size());
    transform(std::execution::par, queries.begin(), queries.end(), results.begin(),
              [&search_server](const auto& query) { return search_server.FindTopDocuments(query); });
    return results;
}

std::vector<Document> ProcessQueriesJoined(
    const SearchServer& search_server,
    const std::vector<std::string>& queries){
    std::vector<std::vector<Document>> results = ProcessQueries(search_server,queries);
    std::vector<Document> documents;
    for (auto const& document : results){
        documents.insert(documents.end(), document.begin(), document.end());
    }
    return documents;
}
