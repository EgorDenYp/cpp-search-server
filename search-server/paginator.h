#pragma once
#include <iostream>
#include <vector>

template <typename Iterator>
class IteratorRange {
public:
    IteratorRange (Iterator begin, Iterator end) 
    : begin_(begin), end_(end), size_(distance(begin, end)) {
    }

    auto begin() const {
        return begin_;
    }

    auto end() const {
        return end_;
    }
    
    auto size() const {
        return size_    ;
    }


private:
    Iterator begin_;
    Iterator end_;
    size_t size_;

};

template <typename Iterator>
std::ostream& operator<<(std::ostream& out, const IteratorRange<Iterator>& object) {
    for (auto i = object.begin(); i != object.end(); advance(i, 1)) {
        out << *i;
    }
    return out;
}

template <typename Iterator>
class Paginator {
public:
    Paginator (Iterator begin, Iterator end, size_t page_size)
    : page_size_(page_size) {
        int full_pages_count = distance(begin, end) / page_size_;
        int last_page_fields_count = distance(begin, end) % page_size_;
        Iterator iter = begin;
        for (int i = 0; i < full_pages_count; ++i) {
            pages_.push_back(IteratorRange<Iterator>{iter, next(iter, page_size_)});
            advance(iter, page_size_);
        }
        if (last_page_fields_count > 0) {
            pages_.push_back(IteratorRange<Iterator>{iter, next(iter, last_page_fields_count)});
        }
    }

    auto begin () const {
        return pages_.begin();
    }

    auto end () const {
        return pages_.end();
    }

    size_t size () const {
        return pages_.size();
    }

private:
    size_t page_size_;
    std::vector<IteratorRange<Iterator>> pages_;

};

template <typename Container>
auto Paginate(const Container& c, size_t page_size) {
    return Paginator(begin(c), end(c), page_size);
}