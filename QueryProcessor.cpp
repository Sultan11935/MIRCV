#include "QueryProcessor.h"
#include <iostream>
#include <string>

QueryProcessor::QueryProcessor(const InvertedIndex& index) : index(index) {}

void QueryProcessor::processQueries() const {
    std::wstring query;
    while (true) {
        std::wcout << L"Enter query: ";
        std::getline(std::wcin, query);
        if (query.empty()) break;

        std::wcout << L"Conjunctive or disjunctive (c/d): ";
        std::wstring type;
        std::getline(std::wcin, type);
        bool conjunctive = (type == L"c");

        std::vector<int> results = index.search(query, conjunctive);
        std::wcout << L"Results: ";
        for (int docId : results) {
            std::wcout << docId << L" ";
        }
        std::wcout << std::endl;
    }
}
