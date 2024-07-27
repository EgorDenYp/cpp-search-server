#include <algorithm>
#include <iostream>
#include <set>
#include <string>
#include <utility>
#include <vector>
#include <map>
#include <cmath>

using namespace std;

const int MAX_RESULT_DOCUMENT_COUNT = 5;

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
};

class SearchServer {
public:
    void SetStopWords(const string& text) {
        for (const string& word : SplitIntoWords(text)) {
            stop_words_.insert(word);
        }
    }

    void AddDocument(int document_id, const string& document) {
        const vector<string> words = SplitIntoWordsNoStop(document);
        double TF_incr = 1.0 / static_cast<double>(words.size());
        for (const string& word : words) {
            documents_rev_id_[word] [document_id] += TF_incr;
        }
        ++document_count_;
    }

    vector<Document> FindTopDocuments(const string& raw_query) const {
        const Query query_words = ParseQuery(raw_query);
        vector<Document> matched_documents = FindAllDocuments(query_words);

        sort(matched_documents.begin(), matched_documents.end(),
             [](const Document& lhs, const Document& rhs) -> bool {
                 return lhs.relevance > rhs.relevance;
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

private:

    struct DocWordsCount {
        int doc_id;
        int word_count;
    };
    
    struct Query {
        set<string> plus_words;
        set<string> minus_words;
    };

    map<string, map<int, double>> documents_rev_id_; //inter map consist of doc id and TF of word in it

    set<string> stop_words_;

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

    vector<Document> FindAllDocuments(const Query& query_words) const {
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
        for (const string& minus_word : query_words.minus_words) {
            if (documents_rev_id_.count(minus_word)) {
                for (auto [doc_id, word_TF] : documents_rev_id_.at(minus_word)) {
                    matched_documents_relevance.erase(doc_id);        
                }
            }
        }
        for (const auto& doc_rel : matched_documents_relevance) {
            result.push_back({doc_rel.first, doc_rel.second});
        }

        return result;
    }

    double ComputeIDF (const string& word) const { //we accept that word's presense in documents checks before function call
        return log(static_cast<double>(document_count_)/static_cast<double>(documents_rev_id_.at(word).size()));
    }   

    
    
};

SearchServer CreateSearchServer() {
    SearchServer search_server;
    
    search_server.SetStopWords(ReadLine());

    const int document_count = ReadLineWithNumber();
    for (int document_id = 0; document_id < document_count; ++document_id) {
        search_server.AddDocument(document_id, ReadLine());
    }

    return search_server;
}

//*******************
//******MAIN*********
//*******************

int main() {
    const SearchServer search_server = CreateSearchServer();

    const string query = ReadLine();
    for (const Document& doc : search_server.FindTopDocuments(query)) {
        cout << "{ document_id = "s << doc.id << ", "
             << "relevance = "s << doc.relevance << " }"s << endl;
    }
}