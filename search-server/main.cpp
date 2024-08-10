#include <algorithm>
#include <cmath>
#include <iostream>
#include <set>
#include <string>
#include <utility>
#include <vector>
#include <map>
#include <tuple>


using namespace std;

const int MAX_RESULT_DOCUMENT_COUNT = 5;
const double EPSILON = 1e-6;

string ReadLine() {
    string s;
    getline(cin, s);
    return s;
}

int ReadLineWithNumber() {
    int result = 0;
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
    int id = 0;
    double relevance = 0.0;
    int rating = 0;
};

enum class DocumentStatus {
    ACTUAL,
    IRRELEVANT,
    BANNED,
    REMOVED
};

class SearchServer {
public:
    void SetStopWords(const string& text) {
        for (const string& word : SplitIntoWords(text)) {
            stop_words_.insert(word);
        }
    }

    void AddDocument(int document_id, const string& document, const DocumentStatus status, const vector<int>& ratings) {
        const vector<string> words = SplitIntoWordsNoStop(document);
        double TF_incr = 1.0 / static_cast<double>(words.size());
        for (const string& word : words) {
            documents_rev_id_[word] [document_id] += TF_incr;
        }
        documents_properties_[document_id].rating = ComputeAverageRating(ratings);
        documents_properties_[document_id].status = status;
        ++document_count_;
        //print document
    }

    vector<Document> FindTopDocuments(const string& raw_query) const {
        return FindTopDocuments(raw_query, [](int doc_id, DocumentStatus status, int rating) { return (status == DocumentStatus::ACTUAL); });
    }

    vector<Document> FindTopDocuments(const string& raw_query, DocumentStatus document_status) const {
        return FindTopDocuments (raw_query, [document_status](int doc_id, DocumentStatus status, int rating) { return document_status == status; });
    }

    template<typename Filter>
    vector<Document> FindTopDocuments(const string& raw_query, Filter filter) const {
        const Query query_words = ParseQuery(raw_query);
        vector<Document> matched_documents = FindAllDocuments(query_words, filter);

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


    void GetDocumentBase () const {
        for (auto& [word, docs] : documents_rev_id_) {
            cout << "word:"s << word << endl;
            for (auto [doc_id, TF] : docs) {
                cout << "  doc_id:"s << doc_id << " TF: "s << TF << endl;
            }
        }
        cout << "number of documents: "s<< document_count_ << endl;
    }

    void PrintDocumentsProperties () const {
        for (const auto& document : documents_properties_) {
            cout << "id: "s << document.first << " status: "s << static_cast<int>(document.second.status) << endl;  
        }
    }

    int GetDocumentCount () const {
        return document_count_;
    }

    tuple<vector<string>, DocumentStatus> MatchDocument (const string& raw_query, int document_id) const {
        auto query = ParseQuery(raw_query);
        vector<string> matched_words;
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
        return tuple(matched_words, documents_properties_.at(document_id).status);
    }


private:
    
    struct Query {
        set<string> plus_words;
        set<string> minus_words;
    };

    struct DocumentProperties {
        int rating;
        DocumentStatus status;
    };

    map<string, map<int, double>> documents_rev_id_; //inter map consist of doc id and TF of word in it

    set<string> stop_words_;
    
    map<int, DocumentProperties> documents_properties_;

    int document_count_ = 0; //total number of documents on server

    bool IsStopWord(const string& word) const {
        return stop_words_.count(word) > 0;
    }

    vector<string> SplitIntoWordsNoStop(const string& text) const {
        vector<string> words;
        for (const string& word : SplitIntoWords(text)) {
            if (!IsStopWord(word[0]=='-' ? word.substr(1) : word)) {
                words.push_back(word);
            }
        }
        return words;
    }

    Query ParseQuery(const string& text) const {
        Query query_words;
        for (const string& word : SplitIntoWordsNoStop(text)) {
            if (word[0] == '-') {
                query_words.minus_words.insert(word.substr(1));
            } else {
                query_words.plus_words.insert(word);
            }
        }
        return query_words;
    }

    template <typename Filter>
    vector<Document> FindAllDocuments(const Query& query_words, Filter filter) const {
        vector<Document> result;
        map <int, double> matched_documents_relevance;
        for (const string& plus_word : query_words.plus_words) {
          if (documents_rev_id_.count(plus_word)) {
            double word_IDF = ComputeIDF(plus_word); //get IDF of this query's word
            for (auto [doc_id, word_TF] : documents_rev_id_.at(plus_word)) {
                matched_documents_relevance[doc_id] += word_TF*word_IDF;
            }
          }
        }
        //deletion of documents with minus words
        for (const string& minus_word : query_words.minus_words) {
            if (documents_rev_id_.count(minus_word)) {
                for (auto [doc_id, word_TF] : documents_rev_id_.at(minus_word)) {
                    matched_documents_relevance.erase(doc_id);        
                }
            }
        }

        //conversion of result to vector with deletion of documents that don't match to status
        for (const auto& [id, relevance] : matched_documents_relevance) {
            if (filter(id, documents_properties_.at(id).status, documents_properties_.at(id).rating)) {
                result.push_back({id, relevance, documents_properties_.at(id).rating});
            }
            
        }

        return result;
    }

    double ComputeIDF (const string& word) const { //we accept that word's presense in documents checks before function call
        return log(static_cast<double>(document_count_)/static_cast<double>(documents_rev_id_.at(word).size()));
    }   

   static int ComputeAverageRating (const vector<int>& ratings) {
        int result = 0;
        for (int rating : ratings) {
            result += rating;
        }
        if (!ratings.empty()) {
            return result / static_cast<int>(ratings.size());
        } else {
            return 0;
        }
    }

    bool WordIsInDocument (const string& word, int document_id) const {
        return documents_rev_id_.count(word) && documents_rev_id_.at(word).count(document_id);
    }
    
};

vector<int> ReadRatings () {
    int number_of_ratings;
    vector<int> ratings;
    cin >> number_of_ratings;
    for (int i = 0; i < number_of_ratings; ++i){
        int rate;
        cin >> rate;
        ratings.push_back(rate);
    }
    ReadLine();
    return ratings;
}

SearchServer CreateSearchServer() {
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

void PrintDocument(const Document& document) {
    cout << "{ "s
         << "document_id = "s << document.id << ", "s
         << "relevance = "s << document.relevance << ", "s
         << "rating = "s << document.rating
         << " }"s << endl;
}

//*******************
//******MAIN*********
//*******************

int main() {
    SearchServer search_server;
    search_server.SetStopWords("и в на"s);
    search_server.AddDocument(0, "белый кот и модный ошейник"s,        DocumentStatus::ACTUAL, {8, -3});
    search_server.AddDocument(1, "пушистый кот пушистый хвост"s,       DocumentStatus::ACTUAL, {7, 2, 7});
    search_server.AddDocument(2, "ухоженный пёс выразительные глаза"s, DocumentStatus::ACTUAL, {5, -12, 2, 1});
    search_server.AddDocument(3, "ухоженный скворец евгений"s,         DocumentStatus::BANNED, {9});
    cout << "ACTUAL by default:"s << endl;
    for (const Document& document : search_server.FindTopDocuments("пушистый ухоженный кот"s)) {
        PrintDocument(document);
    }
    cout << "BANNED:"s << endl;
    for (const Document& document : search_server.FindTopDocuments("пушистый ухоженный кот"s, DocumentStatus::BANNED)) {
        PrintDocument(document);
    }
    cout << "Even ids:"s << endl;
    for (const Document& document : search_server.FindTopDocuments("пушистый ухоженный кот"s, [](int document_id, DocumentStatus status, int rating) { return document_id % 2 == 0; })) {
        PrintDocument(document);
    }
    return 0;
}