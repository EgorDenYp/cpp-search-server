#pragma once

#include <deque>
#include <vector>
#include "search_server.h"

class RequestQueue {
public:
    explicit RequestQueue(const SearchServer& search_server);
    // сделаем "обёртки" для всех методов поиска, чтобы сохранять результаты для нашей статистики
    template <typename DocumentPredicate>
    std::vector<Document> AddFindRequest(const std::string& raw_query, DocumentPredicate document_predicate);
    
    std::vector<Document> AddFindRequest(const std::string& raw_query, DocumentStatus status);

    std::vector<Document> AddFindRequest(const std::string& raw_query);

    int GetNoResultRequests() const;
private:
    struct QueryResult {
        std::string query;
    };
    std::deque<QueryResult> requests_;
    const static int min_in_day_ = 1440;
    const SearchServer& search_server_;
    int time_minutes_ = 0;
    bool day_passed_ = false;
    // возможно, здесь вам понадобится что-то ещё
};

template <typename DocumentPredicate>
std::vector<Document> RequestQueue::AddFindRequest(const std::string& raw_query, DocumentPredicate document_predicate) {
    if (!day_passed_) {
        ++time_minutes_;
        day_passed_ = time_minutes_ > min_in_day_;
    }
    if (day_passed_ && !requests_.empty()) {
        requests_.pop_front();
    }
    auto result = search_server_.FindTopDocuments(raw_query, document_predicate);
    if (result.empty()) {
        requests_.push_back({raw_query});
    }
    return result;
}