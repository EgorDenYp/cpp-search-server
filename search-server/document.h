#pragma once
#include <iostream>

struct Document {
    
    Document() = default;

    Document(int id_p, double relevance_p, int rating_p);

    int id = 0;
    double relevance = 0.0;
    int rating = 0;

};

std::ostream& operator<<(std::ostream& out, Document document);