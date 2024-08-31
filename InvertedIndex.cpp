#include "InvertedIndex.h"
#include "utils.h"
#include <sstream>
#include <fstream>
#include <iostream>
#include <cmath>
#include <codecvt>
#include <locale>
#include <sys/stat.h>  // Include for stat and mkdir
#include <unistd.h>    // Include for POSIX functions
#include <algorithm>   // Include for std::transform and std::remove_if
#include <unordered_map>
#include <unordered_set> 
#include <algorithm> // Include this for std::sort


void InvertedIndex::buildIndex(const std::unordered_map<int, std::wstring>& documents) {
    int termID = 0;
    for (const auto& doc : documents) {
        int docID = doc.first;
        std::wstringstream wss(doc.second);
        std::wstring word;
        std::unordered_map<std::wstring, int> termFrequency;

        int docLength = 0;
        while (wss >> word) {
            word = preprocessWord(word);
            termFrequency[word]++;
            docLength++;
        }

        for (const auto& term : termFrequency) {
            if (lexicon.find(termID) == lexicon.end()) {
                lexicon[termID] = term.first;
                termID++;
            }
            index[term.first].push_back({docID, term.second});
        }

        docLengths[docID] = docLength;
        docIDToDocno[docID] = docID; // Assuming docID and docno are the same for simplicity
    }

    // Debug print
    int count = 0;
    for (const auto& entry : index) {
        if (count++ < 10) { // Print first 10 terms and their postings
            std::wcout << L"Term: " << entry.first << L", Postings: ";
            for (const auto& posting : entry.second) {
                std::wcout << L"(docID: " << posting.docID << L", freq: " << posting.frequency << L") ";
            }
            std::wcout << std::endl;
        }
    }
    std::wcout << L"Total terms indexed: " << index.size() << std::endl;
}

void InvertedIndex::saveIndex(const std::string& indexPath) const {
    // Create the directory if it doesn't exist
    struct stat info;
    if (stat(indexPath.c_str(), &info) != 0) {
        // Directory does not exist, create it
        if (mkdir(indexPath.c_str(), 0777) == -1) {
            std::cerr << "Error creating directory: " << indexPath << std::endl;
            return;
        } else {
            std::cout << "Directory created: " << indexPath << std::endl;
        }
    } else {
        std::cout << "Directory already exists: " << indexPath << std::endl;
    }

    std::ofstream indexFile(indexPath + "/index.dat", std::ios::binary);
    std::ofstream lexiconFile(indexPath + "/lexicon.dat");
    std::ofstream docLengthsFile(indexPath + "/doclengths.dat");
    std::ofstream docIDToDocnoFile(indexPath + "/docid_to_docno.dat");

    if (!indexFile) {
        std::cerr << "Error opening file: " << indexPath + "/index.dat" << std::endl;
    } else {
        std::cout << "Opened file for writing: " << indexPath + "/index.dat" << std::endl;
    }
    if (!lexiconFile) {
        std::cerr << "Error opening file: " << indexPath + "/lexicon.dat" << std::endl;
    } else {
        std::cout << "Opened file for writing: " << indexPath + "/lexicon.dat" << std::endl;
    }
    if (!docLengthsFile) {
        std::cerr << "Error opening file: " << indexPath + "/doclengths.dat" << std::endl;
    } else {
        std::cout << "Opened file for writing: " << indexPath + "/doclengths.dat" << std::endl;
    }
    if (!docIDToDocnoFile) {
        std::cerr << "Error opening file: " << indexPath + "/docid_to_docno.dat" << std::endl;
    } else {
        std::cout << "Opened file for writing: " << indexPath + "/docid_to_docno.dat" << std::endl;
    }

    if (!indexFile || !lexiconFile || !docLengthsFile || !docIDToDocnoFile) {
        std::cerr << "One or more files could not be opened. Exiting saveIndex." << std::endl;
        return;
    }

    std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;

    std::cout << "Saving index..." << std::endl;
    for (const auto& entry : index) {
        indexFile << converter.to_bytes(entry.first) << " ";
        for (const auto& posting : entry.second) {
            indexFile << posting.docID << ":" << posting.frequency << " ";
        }
        indexFile << "\n";
    }
    std::cout << "Index saved to " << indexPath + "/index.dat" << std::endl;

    std::cout << "Saving lexicon..." << std::endl;
    for (const auto& entry : lexicon) {
        lexiconFile << entry.first << " " << converter.to_bytes(entry.second) << "\n";
    }
    std::cout << "Lexicon saved to " << indexPath + "/lexicon.dat" << std::endl;

    std::cout << "Saving document lengths..." << std::endl;
    for (const auto& entry : docLengths) {
        docLengthsFile << entry.first << " " << entry.second << "\n";
    }
    std::cout << "Document lengths saved to " << indexPath + "/doclengths.dat" << std::endl;

    std::cout << "Saving document ID to document number mapping..." << std::endl;
    for (const auto& entry : docIDToDocno) {
        docIDToDocnoFile << entry.first << " " << entry.second << "\n";
    }
    std::cout << "Document ID to document number mapping saved to " << indexPath + "/docid_to_docno.dat" << std::endl;
}

void InvertedIndex::loadIndex(const std::string& indexPath) {
    std::ifstream lexiconFile(indexPath + "/lexicon.dat");
    std::ifstream docLengthsFile(indexPath + "/doclengths.dat");
    std::ifstream docIDToDocnoFile(indexPath + "/docid_to_docno.dat");

    if (!lexiconFile || !docLengthsFile || !docIDToDocnoFile) {
        std::cerr << "Error opening index files from directory: " << indexPath << std::endl;
        return;
    }

    std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
    std::string line;

    // Load lexicon
    while (std::getline(lexiconFile, line)) {
        std::istringstream iss(line);
        int termID;
        std::string term;
        if (iss >> termID >> term) {
            lexicon[termID] = converter.from_bytes(term);
        }
    }

    // Load document lengths
    while (std::getline(docLengthsFile, line)) {
        std::istringstream iss(line);
        int docID, docLength;
        if (iss >> docID >> docLength) {
            docLengths[docID] = docLength;
        }
    }

    // Load document ID to document number mapping
    while (std::getline(docIDToDocnoFile, line)) {
        std::istringstream iss(line);
        int docID, docno;
        if (iss >> docID >> docno) {
            docIDToDocno[docID] = docno;
        }
    }
}



std::vector<int> InvertedIndex::search(const std::wstring& query, bool conjunctive) const {
    std::vector<int> results;

    std::wstringstream wss(query);
    std::wstring word;
    std::vector<std::wstring> terms;

    // Preprocess and store all terms from the query
    while (wss >> word) {
        word = preprocessWord(word);
        terms.push_back(word);
    }

    // Early exit if no terms are provided in the query
    if (terms.empty()) {
        return results;
    }

    // To handle the different query types, we will have different logic for conjunctive and disjunctive
    std::unordered_map<int, int> docFreqMap;

    bool firstTerm = true;

    for (const auto& term : terms) {
        // Open list (read the postings from disk)
        openList(term);

        std::unordered_set<int> currentDocs;  // Track current term's document IDs

        // Traverse the postings list
        int docID;
        while ((docID = next()) != -1) {
            currentDocs.insert(docID);
            if (conjunctive) {
                // For the first term, initialize the map with document IDs
                if (firstTerm) {
                    docFreqMap[docID] = 1;
                } else if (docFreqMap.find(docID) != docFreqMap.end()) {
                    // For subsequent terms, only increment if docID exists in map
                    docFreqMap[docID]++;
                }
            } else {
                // For disjunctive, add directly to results if not already present
                if (docFreqMap.find(docID) == docFreqMap.end()) {
                    results.push_back(docID);
                    docFreqMap[docID] = 1;  // Mark as seen
                }
            }
        }

        closeList();

        if (conjunctive && !firstTerm) {
            // For conjunctive: Remove documents that are not in the intersection
            for (auto it = docFreqMap.begin(); it != docFreqMap.end();) {
                if (currentDocs.find(it->first) == currentDocs.end()) {
                    it = docFreqMap.erase(it);
                } else {
                    ++it;
                }
            }
        }

        firstTerm = false;
        
        // Early exit for conjunctive if no documents remain
        if (conjunctive && docFreqMap.empty()) {
            return results;  // No documents can possibly match all terms
        }
    }

    // For conjunctive queries, collect documents that matched all terms
    if (conjunctive) {
        for (const auto& entry : docFreqMap) {
            if (entry.second == terms.size()) {
                results.push_back(entry.first);
            }
        }
    }

    // Sort the results before returning
    std::sort(results.begin(), results.end());

    return results;
}



void InvertedIndex::openList(const std::wstring& term) const {
    currentTerm = term;

    std::ifstream indexFile("/home/sultan/MIRCV_Project/main/index_files/index.dat");
    if (!indexFile) {
        std::cerr << "Error opening index file." << std::endl;
        return;
    }

    std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
    std::string line;
    while (std::getline(indexFile, line)) {
        std::istringstream iss(line);
        std::string termStr;
        if (iss >> termStr && converter.from_bytes(termStr) == term) {
            std::vector<Posting> postings;
            std::string postingStr;
            while (iss >> postingStr) {
                size_t colonPos = postingStr.find(':');
                int docID = std::stoi(postingStr.substr(0, colonPos));
                int freq = std::stoi(postingStr.substr(colonPos + 1));
                postings.push_back({docID, freq});
            }
            index[term] = postings;
            break;
        }
    }

    if (index.find(term) != index.end()) {
        currentPosting = index[term].begin();
        endPosting = index[term].end();
    } else {
        currentPosting = endPosting; // Empty
    }
}


void InvertedIndex::closeList() const {
    currentTerm.clear();
    currentPosting = endPosting;
}

int InvertedIndex::next() const {
    if (currentPosting != endPosting) {
        return (currentPosting++)->docID;
    }
    return -1;
}

int InvertedIndex::getDocid() const {
    if (currentPosting != endPosting) {
        return currentPosting->docID;
    }
    return -1;
}

int InvertedIndex::getFreq() const {
    if (currentPosting != endPosting) {
        return currentPosting->frequency;
    }
    return -1;
}


std::wstring InvertedIndex::preprocessWord(const std::wstring& word) const {
    std::wstring result = word;
    std::transform(result.begin(), result.end(), result.begin(), ::towlower);
    result.erase(std::remove_if(result.begin(), result.end(), ::iswpunct), result.end());
    return result;
}

double InvertedIndex::computeTF(int termFreq, int docLength) const {
    return static_cast<double>(termFreq) / docLength;
}

double InvertedIndex::computeIDF(int docCount) const {
    return std::log(static_cast<double>(docLengths.size()) / (docCount + 1));
}

double InvertedIndex::computeTFIDF(int termFreq, int docLength, int docCount) const {
    return computeTF(termFreq, docLength) * computeIDF(docCount);
}
