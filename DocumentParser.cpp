#include "DocumentParser.h"
#include <fstream>
#include <iostream>
#include <sstream>
#include <locale>
#include <codecvt>

DocumentParser::DocumentParser(const std::string& datasetPath, int numDocs) 
    : datasetPath(datasetPath), numDocs(numDocs) {}

void DocumentParser::parseDocuments() {
    std::ifstream file(datasetPath);
    if (!file) {
        std::cerr << "Error opening file: " << datasetPath << std::endl;
        return;
    }

    std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
    std::string line;
    int docID = 0;

    while (std::getline(file, line) && (numDocs == -1 || docID < numDocs)) {
        std::wstring wline = converter.from_bytes(line);
        documents[docID] = wline;
        std::wcout << L"Parsed document " << docID << L": " << wline << std::endl;
        docID++;
    }

    std::wcout << L"Total documents parsed: " << docID << std::endl;
}

const std::unordered_map<int, std::wstring>& DocumentParser::getDocuments() const {
    return documents;
}
