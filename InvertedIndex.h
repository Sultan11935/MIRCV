#ifndef INVERTEDINDEX_H
#define INVERTEDINDEX_H

#include <unordered_map>
#include <vector>
#include <string>

struct Posting {
    int docID;
    int frequency;
};

class InvertedIndex {
public:
    void buildIndex(const std::unordered_map<int, std::wstring>& documents);
    void saveIndex(const std::string& indexPath) const;
    void loadIndex(const std::string& indexPath);
    std::vector<int> search(const std::wstring& query, bool conjunctive) const;
    void openList(const std::wstring& term) const;
    void closeList() const;
    int next() const;
    int getDocid() const;
    int getFreq() const;

private:
    std::unordered_map<int, std::wstring> lexicon;
    std::unordered_map<int, int> docLengths;
    std::unordered_map<int, int> docIDToDocno;

    mutable std::wstring currentTerm;
    mutable std::vector<Posting>::const_iterator currentPosting;
    mutable std::vector<Posting>::const_iterator endPosting;

    mutable std::unordered_map<std::wstring, std::vector<Posting>> index;  // Mark as mutable

    std::wstring preprocessWord(const std::wstring& word) const;
    double computeTF(int termFreq, int docLength) const;
    double computeIDF(int docCount) const;
    double computeTFIDF(int termFreq, int docLength, int docCount) const;
};

#endif
