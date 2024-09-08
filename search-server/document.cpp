#include "document.h"

Document::Document(int id_p, double relevance_p, int rating_p) 
        :id(id_p), relevance(relevance_p), rating(rating_p)
{
};

std::ostream& operator<<(std::ostream& out, Document document) {
    out << "{ "
        << "document_id = " << document.id << ", "
        << "relevance = " << document.relevance << ", "
        << "rating = " << document.rating
        << " }";
    return out;
}