#pragma once

#include <algorithm>
#include <map>
#include <vector>
#include <set>
#include <stdexcept>
#include <string>
#include <tuple>
#include "string_processing.h"
#include "document.h"

const int MAX_RESULT_DOCUMENT_COUNT = 5;
const double EPSILON = 1e-6;

enum class DocumentStatus {
    ACTUAL,
    IRRELEVANT,
    BANNED,
    REMOVED
};

class SearchServer {
public:

    SearchServer() = default;

    explicit SearchServer(const std::string& stop_words_string);

    template <typename Collection>
    explicit SearchServer (const Collection& stop_words_collection);

    void SetStopWords(const std::string& text);

    void AddDocument(int document_id, const std::string& document, const DocumentStatus status, const std::vector<int>& ratings);

    std::vector<Document> FindTopDocuments(const std::string& raw_query, DocumentStatus document_status = DocumentStatus::ACTUAL) const;
    
    template<typename Filter>
    std::vector<Document> FindTopDocuments(const std::string& raw_query, Filter filter) const;

    size_t GetDocumentCount () const;

    int GetDocumentId (int index);

    std::tuple<std::vector<std::string>, DocumentStatus> MatchDocument (const std::string& raw_query, int document_id) const;

private:
    
    struct Query {
        std::set<std::string> plus_words;
        std::set<std::string> minus_words;
    };

    struct DocumentProperties {
        int rating;
        DocumentStatus status;
    };

    std::map<std::string, std::map<int, double>> documents_rev_id_; //inter map consist of doc id and TF of word in it

    std::set<std::string> stop_words_;
    
    std::map<int, DocumentProperties> documents_properties_;

    std::vector<int> doc_add_order_ = {};

    //total number of documents on server

    bool IsStopWord(const std::string& word) const;

    std::vector<std::string> SplitIntoWordsNoStop(const std::string& text) const;

    Query ParseQuery(const std::string& text) const;

    template <typename Filter>
    std::vector<Document> FindAllDocuments(const Query& query_words, Filter filter) const;

    double ComputeIDF (const std::string& word) const;   

    static int ComputeAverageRating (const std::vector<int>& ratings);

    static bool IsValidWord (const std::string& word);

    bool WordIsInDocument (const std::string& word, int document_id) const;
};



template <typename Collection>
SearchServer::SearchServer (const Collection& stop_words_collection) {
    using namespace std;
    all_of(stop_words_collection.begin(), stop_words_collection.end(), [this](const std::string& word) -> bool {
        if (IsValidWord(word)) {
            stop_words_.insert(word);
            return true;
        }
        throw invalid_argument("special symbols in stop words were detected");
    });
}

template<typename Filter>
std::vector<Document> SearchServer::FindTopDocuments(const std::string& raw_query, Filter filter) const {
    const auto query_words = ParseQuery(raw_query);
    std::vector<Document> matched_documents = FindAllDocuments(query_words, filter);

    sort(matched_documents.begin(), matched_documents.end(),
            [](const Document& lhs, const Document& rhs) -> bool {
            //double difference = lhs.relevance - rhs.relevance;
            return (lhs.relevance - rhs.relevance >= EPSILON) || (std::abs(lhs.relevance - rhs.relevance) < EPSILON && lhs.rating > rhs.rating);
            });
    if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
        matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
    }
    return matched_documents;
}

template <typename Filter>
std::vector<Document> SearchServer::FindAllDocuments(const SearchServer::Query& query_words, Filter filter) const {
    std::vector<Document> result;
    std::map <int, double> matched_documents_relevance;
    for (const std::string& plus_word : query_words.plus_words) {
        if (documents_rev_id_.count(plus_word)) {
        double word_IDF = ComputeIDF(plus_word); //get IDF of this query's word
        for (auto [doc_id, word_TF] : documents_rev_id_.at(plus_word)) {
            if (filter(doc_id, documents_properties_.at(doc_id).status, documents_properties_.at(doc_id).rating)) {
                matched_documents_relevance[doc_id] += word_TF * word_IDF;
            }
        }
        }
    }
    //deletion of documents with minus words
    for (const std::string& minus_word : query_words.minus_words) {
        if (documents_rev_id_.count(minus_word)) {
            for (auto [doc_id, word_TF] : documents_rev_id_.at(minus_word)) {
                matched_documents_relevance.erase(doc_id);        
            }
        }
    }

    //conversion of result to vector with deletion of documents that don't match to status
    for (const auto& [id, relevance] : matched_documents_relevance) {
        result.push_back({id, relevance, documents_properties_.at(id).rating});
    }

    return result;
}

SearchServer CreateSearchServer();