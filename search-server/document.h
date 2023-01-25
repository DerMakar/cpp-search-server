#pragma once
#include <iostream>
#include <string>

struct Document {
    Document();

    Document(int id, double relevance, int rating);       

    int id = 0;
    double relevance = 0.0;
    int rating = 0;

};

std::ostream& operator<<(std::ostream& out, const Document document);

enum class DocumentStatus {
    ACTUAL,
    IRRELEVANT,
    BANNED,
    REMOVED,
};