#include "read_input_functions.h"
#include <iostream>

std::string ReadLine() {
    using namespace std;
    string s;
    getline(std::cin, s);
    return s;
}

int ReadLineWithNumber() {
    int result = 0;
    std::cin >> result;
    ReadLine();
    return result;
}

std::vector<int> ReadRatings () {
    int number_of_ratings;
    std::vector<int> ratings;
    std::cin >> number_of_ratings;
    for (int i = 0; i < number_of_ratings; ++i){
        int rate;
        std::cin >> rate;
        ratings.push_back(rate);
    }
    ReadLine();
    return ratings;
}