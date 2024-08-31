#ifndef DOCUMENT_PARSER_H
#define DOCUMENT_PARSER_H

#include <unordered_map>
#include <string>

class DocumentParser {
public:
    DocumentParser(const std::string& datasetPath, int numDocs);
    void parseDocuments();
    const std::unordered_map<int, std::wstring>& getDocuments() const;

private:
    std::string datasetPath;
    int numDocs;
    std::unordered_map<int, std::wstring> documents;
};

#endif // DOCUMENT_PARSER_H
