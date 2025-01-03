#include "DocumentParser.h"
#include "utils.h"
#include <fstream>
#include <iostream>
#include <sstream>
#include <locale>
#include <codecvt>
#include <chrono>

DocumentParser::DocumentParser(const std::string& datasetPath, int numDocs)
    : datasetPath(datasetPath), numDocs(numDocs) {}

void DocumentParser::parseDocuments() {
    std::ifstream file(datasetPath);
    if (!file) {
        std::cerr << "Error opening file: " << datasetPath << std::endl;
        return;
    }

    auto start = std::chrono::high_resolution_clock::now(); // Start timing

    std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
    std::string line;
    int docID = 0;

    while (std::getline(file, line) && (numDocs == -1 || docID < numDocs)) {
        try {
            std::wstring wline = converter.from_bytes(line);

            // Skip empty lines
            if (wline.empty()) {
                //std::cerr << "Skipping empty line in document ID: " << docID << std::endl;
                continue;
            }

            std::wstringstream wss(wline);
            std::wstring word;
            std::wstring processedDocument;

            // Process each word
            while (wss >> word) {
                std::wstring processedWord = preprocessWord(word);

                // Skip invalid or empty processed words
                if (processedWord.empty()) {
                    continue;
                }

                // Reassemble the document after preprocessing
                if (!processedDocument.empty()) {
                    processedDocument += L" ";
                }
                processedDocument += processedWord;
            }

            // Skip empty documents after preprocessing
            if (processedDocument.empty()) {
                //std::cerr << "Skipping document with no valid content. DocID: " << docID << std::endl;
                continue;
            }

            // Add the processed document to the collection
            documents[docID] = processedDocument;
            //std::wcout << L"Processed document " << docID << L": " << processedDocument << std::endl;

            docID++;
        } catch (const std::exception& e) {
            std::cerr << "Error processing document ID: " << docID << ", Error: " << e.what() << std::endl;
            continue;
        }
    }

    auto end = std::chrono::high_resolution_clock::now(); // End timing
    std::chrono::duration<double> elapsed = end - start;

    std::wcout << L"Total documents parsed and processed: " << docID << std::endl;
    std::cout << "Document parsing and preprocessing completed in " << elapsed.count() << " seconds." << std::endl;
}

const std::unordered_map<int, std::wstring>& DocumentParser::getDocuments() const {
    return documents;
}