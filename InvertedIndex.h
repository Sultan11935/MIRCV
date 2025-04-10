#ifndef INVERTEDINDEX_H
#define INVERTEDINDEX_H

#include <unordered_map>
#include <vector>
#include <string>
#include <utility>
#include <queue>
#include <fstream>

// Posting structure for document ID and frequency
struct Posting {
    int docID;
    int frequency;
};

// SearchResult structure for search results with TF-IDF
struct SearchResult {
    int docID;
    int frequency;
    double tfidf;
};

// MergeNode for priority queue in multi-way merge (SPIMI)
struct MergeNode {
    std::wstring term;
    int fileIndex;

    bool operator>(const MergeNode &other) const {
        return term > other.term; // Min-heap priority queue
    }
};

class InvertedIndex {
public:
    // Builds the inverted index using SPIMI
    void buildIndexSPIMI(const std::unordered_map<int, std::wstring>& documents, int chunkSize, const std::string& indexPath);

    // Saves a partial index to disk
    void savePartialIndex(const std::unordered_map<std::wstring, std::vector<Posting>>& partialIndex, int chunkID, const std::string& indexPath);

    // Merges partial index files into a final index
    void mergeIndexes(int numChunks, const std::string& indexPath);

    // Saves the index, lexicon, and metadata to files
    //void saveIndex(const std::string& indexPath) const;

    // Loads the final merged index from disk
    void loadIndex(const std::string& indexPath);

    static int estimateChunkSize();

    // Searches for documents matching the query (with optional conjunctive behavior)
    //std::vector<Posting> search(const std::wstring& query, bool conjunctive) const;

    // Searches for documents with TF-IDF scoring and returns ranked results
    std::vector<SearchResult> searchWithTFIDF(const std::wstring& query, bool conjunctive) const;

    // Opens the postings list for a given term
    void openList(const std::wstring& term) const;

    // Closes the postings list for the current term
    void closeList() const;

    // Retrieves the next document ID in the postings list
    int next() const;

    // Retrieves the frequency of the current document in the postings list
    int getFreq() const;

private:
    // Mapping of term IDs to terms (lexicon)
    std::unordered_map<int, std::wstring> lexicon;

    // Mapping of document IDs to their lengths
    std::unordered_map<int, int> docLengths;

    // Mapping of document IDs to external document numbers
    std::unordered_map<int, int> docIDToDocno;

    // Inverted index structure: term -> postings list
    mutable std::unordered_map<std::wstring, std::vector<Posting>> index;

    // Current term being processed
    mutable std::wstring currentTerm;

    // Iterators for the current postings list
    mutable std::vector<Posting>::const_iterator currentPosting;
    mutable std::vector<Posting>::const_iterator endPosting;
    mutable int lastDocID = -1;
    mutable int lastFreq = 0;

    // Preprocesses a word (lowercase and remove punctuation)
    std::wstring preprocessWord(const std::wstring& word) const;

    // Computes Term Frequency (TF) for a term in a document
    double computeTF(int termFreq) const;

    // Computes Inverse Document Frequency (IDF) for a term
    double computeIDF(int docCount) const;

    // Computes the combined TF-IDF score for a term
    double computeTFIDF(int termFreq, int docLength, int docCount) const;
};

#endif
