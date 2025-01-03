#include "QueryProcessor.h"
#include <iostream>
#include <string>
#include <chrono>

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

        // Start timing
        auto start = std::chrono::high_resolution_clock::now();

        // Perform search with TF-IDF
        auto results = index.searchWithTFIDF(query, conjunctive);

        // End timing
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> elapsed = end - start;

        // Display query time
        std::wcout << L"Query processed in " << elapsed.count() << L" seconds." << std::endl;

        // Display results
        if (results.empty()) {
            std::wcout << L"No results found for query." << std::endl;
        } else {
            std::wcout << L"Top Results (DocID, Freq, TF-IDF):" << std::endl;
            for (const auto& [docID, frequency, tfidf] : results) {
                    std::wcout << L"DocID: " << docID << L", Frequency: " << frequency << L", TF-IDF: " << tfidf << std::endl;
                }

            }
        }
    }

