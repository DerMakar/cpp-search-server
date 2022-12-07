#include <iostream>
#include <algorithm>
#include <cassert>
#include <cmath>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>
#include <cmath>
#include <numeric>

using namespace std;

const int MAX_RESULT_DOCUMENT_COUNT = 5;

string ReadLine() {
    string s;
    getline(cin, s);
    return s;
}

int ReadLineWithNumber() {
    int result;
    cin >> result;
    ReadLine();
    return result;
}

vector<string> SplitIntoWords(const string& text) {
    vector<string> words;
    string word;
    for (const char c : text) {
        if (c == ' ') {
            if (!word.empty()) {
                words.push_back(word);
                word.clear();
            }
        } else {
            word += c;
        }
    }
    if (!word.empty()) {
        words.push_back(word);
    }

    return words;
}

struct Document {
    int id;
    double relevance;
    int rating;
};

enum class DocumentStatus {
    ACTUAL,
    IRRELEVANT,
    BANNED,
    REMOVED,
};

class SearchServer {
public:
    void SetStopWords(const string& text) {
        for (const string& word : SplitIntoWords(text)) {
            stop_words_.insert(word);
        }
    }

    void AddDocument(int document_id, const string& document, 
                     DocumentStatus status,
                     const vector<int>& ratings) {
        const vector<string> words = SplitIntoWordsNoStop(document);
        const double inv_word_count = 1.0 / words.size();
        for (const string& word : words) {
            word_to_document_freqs_[word][document_id] += inv_word_count;
        }
        documents_.emplace(document_id,
                           DocumentData{ComputeAverageRating(ratings), status});
    }

    #define EPS 1e-6    
    
    template <typename Key>
    
    vector<Document> FindTopDocuments(const string& raw_query,
                                      Key key) const {
        const Query query = ParseQuery(raw_query);
        auto matched_documents = FindAllDocuments(query, key);
        sort(matched_documents.begin(), matched_documents.end(),
             [](const Document& lhs, const Document& rhs) {
                if (abs(lhs.relevance - rhs.relevance) < EPS) {
                    return lhs.rating > rhs.rating;
                    }
                return lhs.relevance > rhs.relevance;
             });
        if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
            matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
        }
        return matched_documents;
    }

    int GetDocumentCount() const {
        return documents_.size();
    }

    tuple<vector<string>, DocumentStatus> MatchDocument(const string& raw_query,
                                                        int document_id) const {
        const Query query = ParseQuery(raw_query);
        vector<string> matched_words;
        for (const string& word : query.plus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            if (word_to_document_freqs_.at(word).count(document_id)) {
                matched_words.push_back(word);
            }
        }
        for (const string& word : query.minus_words) {
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
   
     
    vector<Document> FindTopDocuments(const string& raw_query, DocumentStatus status) const {
        return FindTopDocuments (raw_query, [status]([[maybe_unused]]int document_id, 
                                                     DocumentStatus stat,
                                                     [[maybe_unused]]int rating){
                                                     return stat == status;});
    }
            
    vector<Document> FindTopDocuments(const string& raw_query) const {
       DocumentStatus status = DocumentStatus::ACTUAL;
       return FindTopDocuments (raw_query, status);
    }
                     
private:
    struct DocumentData {
        int rating;
        DocumentStatus status;
    };

    set<string> stop_words_;
    map<string, map<int, double>> word_to_document_freqs_;
    map<int, DocumentData> documents_;

    bool IsStopWord(const string& word) const {
        return stop_words_.count(word) > 0;
    }

    vector<string> SplitIntoWordsNoStop(const string& text) const {
        vector<string> words;
        for (const string& word : SplitIntoWords(text)) {
            if (!IsStopWord(word)) {
                words.push_back(word);
            }
        }
        return words;
    }

    static int ComputeAverageRating(const vector<int>& ratings) {
        if (ratings.empty()) {
            return 0;
        }
    return accumulate (ratings.begin(),ratings.end(),0) / 
                       static_cast<int>(ratings.size());
    }

    struct QueryWord {
        string data;
        bool is_minus;
        bool is_stop;
    };

    QueryWord ParseQueryWord(string text) const {
        bool is_minus = false;
        if (text[0] == '-') {
            is_minus = true;
            text = text.substr(1);
        }
        return {text, is_minus, IsStopWord(text)};
    }

    struct Query {
        set<string> plus_words;
        set<string> minus_words;
    };

    Query ParseQuery(const string& text) const {
        Query query;
        for (const string& word : SplitIntoWords(text)) {
            const QueryWord query_word = ParseQueryWord(word);
            if (!query_word.is_stop) {
                if (query_word.is_minus) {
                    query.minus_words.insert(query_word.data);
                } else {
                    query.plus_words.insert(query_word.data);
                }
            }
        }
        return query;
    }

    double ComputeWordInverseDocumentFreq(const string& word) const {
        return log(GetDocumentCount() * 1.0 /
                   word_to_document_freqs_.at(word).size());
    }

    template <typename Key>
    
    vector<Document> FindAllDocuments(const Query& query, Key key) const {
        map<int, double> document_to_relevance;
        for (const string& word : query.plus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            const double inverse_document_freq =
                ComputeWordInverseDocumentFreq(word);
            for (const auto [document_id, term_freq] :
                 word_to_document_freqs_.at(word)) {
                const auto& curr_doc = documents_.at(document_id);
                if (key(document_id, 
                        curr_doc.status,
                        curr_doc.rating)) {
                    document_to_relevance[document_id] 
                        += term_freq * inverse_document_freq;
                }
            }
        }

        for (const string& word : query.minus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            for (const auto [document_id, _] : 
                 word_to_document_freqs_.at(word)) {
                document_to_relevance.erase(document_id);
            }
        }

        vector<Document> matched_documents;
        for (const auto [document_id, relevance] : document_to_relevance) {
            matched_documents.push_back(
                {document_id, relevance, documents_.at(document_id).rating});
        }
        return matched_documents;
    }
};

// -------- Начало модульных тестов поисковой системы ----------

void AssertImpl (bool value, const string& v_str, const string& file_name,
                 const string& func_name, unsigned line, const string& hint){
    if (value != true){
        cerr << file_name << "("s << line << "): "s << func_name << ": ";
        cerr << "ASSERT("s << v_str << ") failed."s;
        if (!hint.empty()){
            cerr << "Hint: "s << hint;
        }
        cerr << endl;
        abort();
    }
}

template <typename T, typename U>

void AssertEqualImpl (const T& t, const U& u, const string& t_str,
                      const string& u_str, const string& file_name, 
                      const string& func_name, unsigned line, 
                      const string& hint){
    if ( t != u){
       cerr << file_name << "("s << line << "): "s << func_name << ": ";
       cerr << "ASSERT_EQUAL("s << t_str << ", "s << u_str << ") failed. "s;
       cerr << t << " != "s << u << "."s; 
        if (!hint.empty()){
            cerr << "Hint: "s << hint;
        }
        cerr << endl;
        abort(); 
    }
}

template <typename F>

void RunTest (F func, const string& func_str){
    func();
    cerr << func_str << " OK"s;
    cerr << endl;
}

#define ASSERT(expr) AssertImpl((expr), #expr, __FILE__, __FUNCTION__, __LINE__, ""s)

#define ASSERT_HINT(expr, hint) AssertImpl((expr), #expr, __FILE__, __FUNCTION__, __LINE__, hint)

#define ASSERT_EQUAL(a, b) AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, ""s )

#define ASSERT_EQUAL_HINT AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, hint)

#define RUN_TEST(test) RunTest((test), #test)

void TestExcludeStopWordsFromAddedDocumentContent() {
    const int doc_id = 42;
    const string content = "cat in the city"s;
    const vector<int> ratings = {1, 2, 3};
    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto found_docs = server.FindTopDocuments("in"s);
        ASSERT_EQUAL(found_docs.size(), 1u);
        const Document& doc0 = found_docs[0];
        ASSERT_EQUAL(doc0.id, doc_id);
    }
    {
        SearchServer server;
        server.SetStopWords("in the"s);
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        ASSERT_HINT(server.FindTopDocuments("in"s).empty(),
                    "Stop words must be excluded from documents"s);
        const auto found_docs = server.FindTopDocuments("cat with white ears"s);
        const Document& doc0 = found_docs[0];
        ASSERT_EQUAL(doc0.id, doc_id);
    }
}

void TestExludeMinusWordsFromSearch(){
    const int doc_id = 42;
    const string content = "cat in the city"s;
    const vector<int> ratings = {1, 2, 3};
    {
        SearchServer server;
        const string content_with_minus = "-cat doesn't like dog"s;
        server.AddDocument(doc_id, content_with_minus,
                           DocumentStatus::ACTUAL,
                           ratings);
        ASSERT_HINT(server.FindTopDocuments("in"s).empty(),
                    "Документ 42 не вышел из поиска"s);
    }
}

void TestMatchDocument(){
    const int doc_id0 = 42;
    const string content0 = "cat in the -city"s;
    const vector<int> ratings0 = {1, 2, 3};
    const int doc_id1 = 55;
    const string content1 = "dog in the park"s;
    const vector<int> ratings1 = {3, 4, 5};
    {
        SearchServer server;
        server.SetStopWords("in the"s);
        server.AddDocument(doc_id0, content0, DocumentStatus::ACTUAL, ratings0);
        server.AddDocument(doc_id1, content1, DocumentStatus::ACTUAL, ratings1);
        string query = "dog walks in city park"s;
        vector <string> words55;
        vector <string> words42;
        DocumentStatus status;
        tie (words55, status) = server.MatchDocument(query,doc_id1);
        ASSERT (words55.size() == 2);
        ASSERT (words55[0] == "dog"s);
        ASSERT (words55[1] == "park"s);
        tie (words42, status) = server.MatchDocument(query,doc_id0);
        ASSERT_HINT (words42.empty(), "\"city\" - минус-слово"s);
    }
}

void TestDocsRelevance(){
    const int doc_id0 = 42;
    const string content0 = "cat in the city"s;
    const vector<int> ratings0 = {1, 2, 3};
    const int doc_id1 = 55;
    const string content1 = "dog in the park"s;
    const vector<int> ratings1 = {3, 4, 5};
    {
        SearchServer server;
        server.SetStopWords("in the"s);
        server.AddDocument(doc_id0, content0, DocumentStatus::ACTUAL, ratings0);
        server.AddDocument(doc_id1, content1, DocumentStatus::ACTUAL, ratings1);
        const auto found_docs = server.FindTopDocuments("dog walks in city park"s);
        double city = 0.693147;
        double dog = 0.693147;
        double park = 0.693147;
        double IDTF0 = 0.5 * city;
        double IDTF1 = (0.5*(dog + park));
        ASSERT(found_docs.size() == 2);
        const Document& doc0 = found_docs[0];
        const Document& doc1 = found_docs[1]; 
        ASSERT(abs(doc0.relevance - IDTF1) < EPS);
        ASSERT(abs(doc1.relevance - IDTF0) < EPS);
        ASSERT_HINT(doc0.relevance > doc1.relevance, "неверная сортировка"s);
     }
}

void TestSortByRelevance(){
    const int doc_id0 = 42;
    const string content0 = "cat in the city"s;
    const vector<int> ratings0 = {1, 2, 3};
    const int doc_id1 = 55;
    const string content1 = "dog in the park"s;
    const string content2 = "bird in the park"s;
    const vector<int> ratings1 = {3, 4, 5};
    
    {
        SearchServer server;
        server.SetStopWords("in the"s);
        server.AddDocument(doc_id0, content0, DocumentStatus::ACTUAL, ratings0);
        server.AddDocument(doc_id1, content1, DocumentStatus::ACTUAL, ratings1);
        const auto found_docs = server.FindTopDocuments("dog walks in city park"s);
        ASSERT(found_docs.size() == 2);
        const Document& doc0 = found_docs[0];
        const Document& doc1 = found_docs[1]; 
        ASSERT_HINT(doc0.relevance > doc1.relevance, "неверная сортировка"s);
     }
     {
        SearchServer server;
        server.SetStopWords("in the"s);
        server.AddDocument(doc_id0, content0, DocumentStatus::ACTUAL, ratings0);
        server.AddDocument(doc_id1, content2, DocumentStatus::ACTUAL, ratings1);
        const auto found_docs = server.FindTopDocuments("dog walks in city park"s);
        ASSERT(found_docs.size() == 2);
        const Document& doc0 = found_docs[0];
        const Document& doc1 = found_docs[1]; 
        ASSERT (doc0.relevance - doc1.relevance < EPS);
        ASSERT (doc0.rating > doc1.rating); 
     }
}    

void TestOfAverageRating(){
    const int doc_id0 = 42;
    const string content0 = "cat in the city"s;
    const vector<int> ratings0 = {1, 2, 3};
    const int doc_id1 = 55;
    const string content1 = "dog in the park"s;
    const vector<int> ratings1 = {-3, -4, -5};
    const int doc_id2 = 67;
    const string content2 = "bird flies in the sky up the city"s;
    const vector<int> ratings2 = {-1, 0, 3, -4, 0, 8};
    {
        SearchServer server;
        server.SetStopWords("in the"s);
        server.AddDocument(doc_id0, content0, DocumentStatus::ACTUAL, ratings0);
        server.AddDocument(doc_id1, content1, DocumentStatus::ACTUAL, ratings1);
        server.AddDocument(doc_id2, content2, DocumentStatus::ACTUAL, ratings2);
        const auto found_docs = server.FindTopDocuments("dog walks in city park"s);
        ASSERT(found_docs.size() == 3);
        const Document& doc0 = found_docs[0];
        const Document& doc1 = found_docs[1];
        const Document& doc2 = found_docs[2]; 
        ASSERT_EQUAL(doc0.rating, -4);
        ASSERT_EQUAL(doc1.rating, 2);
        ASSERT_EQUAL(doc2.rating, 1);
     }
}

void TestStatus(){
    const int doc_id0 = 42;
    const string content0 = "cat in the city"s;
    const vector<int> ratings0 = {1, 2, 3};
    const int doc_id1 = 55;
    const string content1 = "dog in the park"s;
    const vector<int> ratings1 = {3, 4, 5};
    const int doc_id2 = 65;
    const string content2 = "bird flies in the sky up the city"s;
    const vector<int> ratings2 = {1, 3, 7};
    {
        SearchServer server;
         server.SetStopWords("in the and from has and"s);
        server.AddDocument(doc_id0, content0, DocumentStatus::ACTUAL, ratings0);
        server.AddDocument(doc_id1, content1, DocumentStatus::BANNED, ratings1);
        server.AddDocument(doc_id2, content2, DocumentStatus::REMOVED, ratings1);
        const auto found_docs_banned = server.FindTopDocuments("dog walks in city park"s, DocumentStatus::BANNED);
        const auto found_docs_actual = server.FindTopDocuments("cat falled from sky in park"s, DocumentStatus::ACTUAL);
        const auto found_docs_removed = server.FindTopDocuments("bird has gone from cat and dog"s, DocumentStatus::REMOVED);
        const auto banned = found_docs_banned[0];
        const auto actual = found_docs_actual[0];
        const auto removed = found_docs_removed[0];
        ASSERT_EQUAL(found_docs_banned.size(), 1);
        ASSERT_EQUAL(found_docs_actual.size(), 1);
        ASSERT_EQUAL(found_docs_removed.size(), 1); 
        ASSERT_EQUAL(banned.id, doc_id1);
        ASSERT_EQUAL(actual.id, doc_id0);
        ASSERT_EQUAL(removed.id, doc_id2);
    }       
}

void TestPredicate(){
    const int doc_id0 = 42;
    const string content0 = "cat in the city"s;
    const vector<int> ratings0 = {1, 2, 3};
    const int doc_id1 = 55;
    const string content1 = "dog in the park"s;
    const vector<int> ratings1 = {3, 4, 5};
    {
        SearchServer server;
        server.AddDocument(doc_id0, content0, DocumentStatus::ACTUAL, ratings0);
        server.AddDocument(doc_id1, content1, DocumentStatus::BANNED, ratings1);
        const auto found_docs_stat = server.FindTopDocuments("dog walks in city park"s, DocumentStatus::BANNED);
        const auto found_docs_predic = server.FindTopDocuments("dog walks in city park"s,
                                                               [](int document_id, [[maybe_unused]]DocumentStatus status,
                                                                  [[maybe_unused]]int rating)
                                                               { return document_id % 2 == 0; });
    ASSERT_EQUAL(found_docs_predic.size(), 1);
    }       
}

void TestSearchServer() {
    RUN_TEST(TestExcludeStopWordsFromAddedDocumentContent);
    RUN_TEST(TestExludeMinusWordsFromSearch);
    RUN_TEST(TestMatchDocument);
    RUN_TEST(TestDocsRelevance);
    RUN_TEST(TestPredicate);
    RUN_TEST(TestStatus);
    RUN_TEST(TestSortByRelevance);
    RUN_TEST(TestOfAverageRating);
}

// --------- Окончание модульных тестов поисковой системы -----------

int main() {
    TestSearchServer();
    cout << "Search server testing finished"s << endl;
}
