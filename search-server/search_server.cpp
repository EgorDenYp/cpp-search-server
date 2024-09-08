#include <algorithm>
#include <numeric>
#include <cmath>
#include "search_server.h"
#include "read_input_functions.h"

using namespace std;

SearchServer::SearchServer(const string& stop_words_string) 
    : SearchServer(SplitIntoWords(stop_words_string)) {
}

void SearchServer::SetStopWords(const std::string& text) {
    for (const std::string& word : SplitIntoWords(text)) {
        stop_words_.insert(word);
    }
}

void SearchServer::AddDocument(int document_id, const std::string& document, const DocumentStatus status, const std::vector<int>& ratings) {
    //checks
    if (document_id < 0) {
        throw invalid_argument("document id is less then zero");
    }

    if (documents_properties_.count(document_id)) {
        throw invalid_argument("document with this id had been added before");
    }

    //endchecks

    doc_add_order_.push_back(document_id);

    const std::vector<std::string> words = SplitIntoWordsNoStop(document);
    double TF_incr = 1.0 / static_cast<double>(words.size());
    for (const std::string& word : words) {
        documents_rev_id_[word] [document_id] += TF_incr;
    }
    DocumentProperties document_propeties_bundled = {ComputeAverageRating(ratings), status};
    documents_properties_[document_id] = document_propeties_bundled;
    //print document
}

std::vector<Document> SearchServer::FindTopDocuments(const std::string& raw_query, DocumentStatus document_status) const {
    return FindTopDocuments (raw_query, [document_status](int doc_id, DocumentStatus status, int rating) { return document_status == status; });
}

size_t SearchServer::GetDocumentCount () const {
    return documents_properties_.size();
}

int SearchServer::GetDocumentId (int index) {
    return static_cast<int>(doc_add_order_.at(static_cast<size_t>(index)));
}

std::tuple<std::vector<std::string>, DocumentStatus> SearchServer::MatchDocument (const std::string& raw_query, int document_id) const {
    auto query = ParseQuery(raw_query);
    std::vector<std::string> matched_words;
    bool minus_words_in_document = false;
    for (auto& word : query.minus_words) {
        if (WordIsInDocument(word, document_id)) {
            minus_words_in_document = true;
        }
    }
    if (!minus_words_in_document) {
        for (auto& word : query.plus_words) {
            if (WordIsInDocument(word, document_id)) {
                matched_words.push_back(word);
            }
        }
    }
    return {matched_words, documents_properties_.at(document_id).status};
}

bool SearchServer::IsStopWord(const std::string& word) const {
    return stop_words_.count(word) > 0;
}

std::vector<std::string> SearchServer::SplitIntoWordsNoStop(const std::string& text) const {
    if (!IsValidWord(text)) {
        throw invalid_argument ("contains special symbols");
    }
    std::vector<std::string> words;
    for (const std::string& word : SplitIntoWords(text)) {
        if (!IsStopWord(word[0]=='-' ? word.substr(1) : word)) {
            words.push_back(word);
        }
    }
    return words;
}

SearchServer::Query SearchServer::ParseQuery(const std::string& text) const {
    Query query_words;
    for (const std::string& word : SplitIntoWordsNoStop(text)) {
        if (word[0] == '-') {
            if (word.size() < 2) {
                throw invalid_argument("no word after minus");
            } 
            if (word[1] == '-') {
                throw invalid_argument("more then one minus before minus-word in query");
            }
            query_words.minus_words.insert(word.substr(1));
        } else {
            query_words.plus_words.insert(word);
        }
    }
    return query_words;
}

double SearchServer::ComputeIDF (const std::string& word) const { //we accept that word's presense in documents checks before function call
    return log(static_cast<double>(GetDocumentCount())/static_cast<double>(documents_rev_id_.at(word).size()));
}

int SearchServer::ComputeAverageRating (const std::vector<int>& ratings) {
    if (!ratings.empty()) {
        return accumulate(ratings.begin(), ratings.end(), 0) / static_cast<int>(ratings.size());
    }
    return 0;
}

bool SearchServer::IsValidWord (const std::string& word) {
    return none_of(word.begin(), word.end(), [](char symbol) {return symbol >= 0 && symbol <= 31;});
}

bool SearchServer::WordIsInDocument (const std::string& word, int document_id) const {
    return documents_rev_id_.count(word) && documents_rev_id_.at(word).count(document_id);
}

SearchServer CreateSearchServer() {
    using namespace std;
    SearchServer search_server;
    
    search_server.SetStopWords(ReadLine());

    const int document_count = ReadLineWithNumber();
   
    for (int document_id = 0; document_id < document_count; ++document_id) {
        string document = ReadLine();
        vector<int> ratings = ReadRatings();
        search_server.AddDocument(document_id, document, DocumentStatus::ACTUAL, ratings);
    }

    return search_server;
}