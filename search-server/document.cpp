#include "document.h"

using namespace std::literals;

 Document::Document() = default;

    Document::Document(int id, double relevance, int rating)
        : id(id)
        , relevance(relevance)
        , rating(rating) {
    }

   

std::ostream& operator<<(std::ostream& out, const Document document){
    return out << "{ document_id = "s << document.id << ", relevance = "s << document.relevance << ", rating = "s << document.rating << " }"s;
    
}