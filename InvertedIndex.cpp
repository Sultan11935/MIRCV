#include "InvertedIndex.h"
#include "utils.h"
#include <sstream>
#include <fstream>
#include <iostream>
#include <cmath>
#include <codecvt>
#include <locale>
#include <sys/stat.h>
#include <unistd.h>
#include <algorithm>
#include <unordered_set>
#include <chrono>

void InvertedIndex::buildIndex(const std::unordered_map<int, std::wstring>& documents) {
    auto start = std::chrono::high_resolution_clock::now(); // Start timing

    int termID = 0; // Counter for unique term IDs

    for (const auto& doc : documents) {
        int docID = doc.first;

        // Skip empty documents
        if (doc.second.empty()) {
            //std::cerr << "Skipping empty document with DocID: " << docID << std::endl;
            continue;
        }

        std::wstringstream wss(doc.second);
        std::wstring word;
        std::unordered_map<std::wstring, int> termFrequency;

        int docLength = 0;

        while (wss >> word) {
            // Preprocess the word (remove punctuation, normalize case, etc.)
            try {
                word = preprocessWord(word);

                // Skip empty or invalid words after preprocessing
                if (word.empty() || word.find(L'/') != std::wstring::npos ||!std::all_of(word.begin(), word.end(), ::iswprint)) {
                    //std::cerr << "Skipping invalid or non-printable word in DocID: " << docID << std::endl;
                    continue;
                }

                termFrequency[word]++;
                docLength++;
            } catch (const std::exception& e) {
                std::cerr << "Error processing word in DocID: " << docID << ", Error: " << e.what() << std::endl;
                continue;
            }
        }

        // Skip documents with no valid words
        if (termFrequency.empty()) {
            //std::cerr << "Skipping document with no valid terms. DocID: " << docID << std::endl;
            continue;
        }

        std::unordered_map<std::wstring, int> termToID; // Local map for efficient lookups

        for (const auto& term : termFrequency) {
            // If the term is new, add it to the lexicon
            if (lexicon.find(termID) == lexicon.end()) {
                lexicon[termID] = term.first;
                termID++;
            }

            // Add term postings to the index
            index[term.first].push_back({docID, term.second});
        }

        // Store document length and external ID mapping
        docLengths[docID] = docLength;
        docIDToDocno[docID] = docID;
    }

    auto end = std::chrono::high_resolution_clock::now(); // End timing
    std::chrono::duration<double> elapsed = end - start;

    std::cout << "Index building completed in " << elapsed.count() << " seconds." << std::endl;

    // Debug: Print the first 5 indexed terms and their postings
    int count = 0;
    for (const auto& entry : index) {
        if (count++ < 5) {
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
    struct stat info;
    if (stat(indexPath.c_str(), &info) != 0) {
        if (mkdir(indexPath.c_str(), 0777) == -1) {
            std::cerr << "Error creating directory: " << indexPath << std::endl;
            return;
        }
    }

    std::ofstream indexFile(indexPath + "/index.dat", std::ios::binary);
    std::ofstream lexiconFile(indexPath + "/lexicon.dat");
    std::ofstream docLengthsFile(indexPath + "/doclengths.dat");
    std::ofstream docIDToDocnoFile(indexPath + "/docid_to_docno.dat");

    if (!indexFile || !lexiconFile || !docLengthsFile || !docIDToDocnoFile) {
        std::cerr << "Error opening one or more files for writing." << std::endl;
        return;
    }

    std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;

    // Debug logs
    std::cout << "Saving index with size: " << index.size() << std::endl;
    std::cout << "Saving lexicon with size: " << lexicon.size() << std::endl;
    std::cout << "Saving docLengths with size: " << docLengths.size() << std::endl;
    std::cout << "Saving docIDToDocno with size: " << docIDToDocno.size() << std::endl;

    // Save index
    for (const auto& entry : index) {
        indexFile << converter.to_bytes(entry.first) << " ";
        for (const auto& posting : entry.second) {
            indexFile << posting.docID << ":" << posting.frequency << " ";
        }
        indexFile << "\n";
    }

    // Save lexicon
    for (const auto& entry : lexicon) {
        lexiconFile << entry.first << " " << converter.to_bytes(entry.second) << "\n";
    }

    // Save document lengths
    for (const auto& entry : docLengths) {
        docLengthsFile << entry.first << " " << entry.second << "\n";
    }

    // Save document ID to document number mapping
    for (const auto& entry : docIDToDocno) {
        docIDToDocnoFile << entry.first << " " << entry.second << "\n";
    }

    std::cout << "Index, lexicon, and other data saved successfully." << std::endl;
}


void InvertedIndex::loadIndex(const std::string& indexPath) {
    std::ifstream indexFile(indexPath + "/index.dat");
    std::ifstream lexiconFile(indexPath + "/lexicon.dat");
    std::ifstream docLengthsFile(indexPath + "/doclengths.dat");
    std::ifstream docIDToDocnoFile(indexPath + "/docid_to_docno.dat");

    if (!indexFile || !lexiconFile || !docLengthsFile || !docIDToDocnoFile) {
        std::cerr << "Error opening one or more index files for reading." << std::endl;
        return;
    }

    std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
    std::string line;

    // Load index
    while (std::getline(indexFile, line)) {
        if (line.empty()) continue; // Skip empty lines

        std::istringstream iss(line);
        std::string term;
        if (!(iss >> term)) {
            //std::cerr << "Malformed line in index file: " << line << std::endl;
            continue;
        }

        try {
            std::wstring wterm = preprocessWord(converter.from_bytes(term));
            std::vector<Posting> postings;
            std::string postingStr;

            while (iss >> postingStr) {
                size_t colonPos = postingStr.find(':');
                if (colonPos == std::string::npos) {
                    //std::cerr << "Malformed posting in index file: " << postingStr << std::endl;
                    continue;
                }

                int docID = std::stoi(postingStr.substr(0, colonPos));
                int freq = std::stoi(postingStr.substr(colonPos + 1));
                postings.push_back({docID, freq});
            }

            if (!postings.empty()) {
                index[wterm] = postings;
            } else {
                std::cerr << "No valid postings for term: " << converter.to_bytes(wterm) << std::endl;
            }
        } catch (const std::exception& e) {
            std::cerr << "Error processing line in index file: " << line << " - " << e.what() << std::endl;
        }
    }

    // Load lexicon
    while (std::getline(lexiconFile, line)) {
        if (line.empty()) continue; // Skip empty lines

        std::istringstream iss(line);
        int termID;
        std::string term;

        if (!(iss >> termID >> term)) {
            //std::cerr << "Malformed line in lexicon file: " << line << std::endl;
            continue;
        }

        try {
            lexicon[termID] = converter.from_bytes(term);
        } catch (const std::exception& e) {
            std::cerr << "Error parsing lexicon entry: " << line << " - " << e.what() << std::endl;
        }
    }

    // Load document lengths
    while (std::getline(docLengthsFile, line)) {
        if (line.empty()) continue; // Skip empty lines

        std::istringstream iss(line);
        int docID, docLength;

        if (!(iss >> docID >> docLength)) {
            //std::cerr << "Malformed line in docLengths file: " << line << std::endl;
            continue;
        }

        docLengths[docID] = docLength;
    }

    // Load document ID to document number mapping
    while (std::getline(docIDToDocnoFile, line)) {
        if (line.empty()) continue; // Skip empty lines

        std::istringstream iss(line);
        int docID, docno;

        if (!(iss >> docID >> docno)) {
            //std::cerr << "Malformed line in docIDToDocno file: " << line << std::endl;
            continue;
        }

        docIDToDocno[docID] = docno;
    }

    std::cout << "Index, lexicon, and metadata successfully loaded." << std::endl;
}



std::vector<SearchResult> InvertedIndex::searchWithTFIDF(const std::wstring& query, bool conjunctive) const {
    std::vector<SearchResult> results;

    std::wstringstream wss(query);
    std::wstring word;
    std::vector<std::wstring> terms;

    while (wss >> word) {
        word = preprocessWord(word);
        terms.push_back(word);
    }

    if (terms.empty()) return results;

    std::unordered_map<int, std::pair<int, double>> docScores; // Store docID -> (frequency, tfidf)
    bool firstTerm = true;

    for (const auto& term : terms) {
        openList(term);

        if (currentPosting == endPosting) { // Skip terms with no postings
            std::wcout << L"No postings found for term: " << term << std::endl;
            closeList();
            if (conjunctive) return {}; // If conjunctive, terminate query as no documents can match
            continue;
        }

        auto termIt = index.find(term);
        if (termIt == index.end()) {
            if (conjunctive) return {}; // No results for conjunctive query
            continue;
        }

        int docID;
        int freq = getFreq();
        while ((docID = next()) != -1) {
            //std::cerr << " DocID: " << docID << std::endl;
            
            //std::cerr << " Freq: " << freq << std::endl;
            if (freq == -1) {
                std::cerr << "Error retrieving frequency for DocID: " << docID << std::endl;
                continue; // Skip invalid postings
            }

            auto docLengthIt = docLengths.find(docID);
            if (docLengthIt != docLengths.end()) {
                double tfidf = computeTFIDF(freq, docLengthIt->second, termIt->second.size());
                if (!conjunctive) {
                    docScores[docID].first += freq;
                    docScores[docID].second += tfidf;
                } else if (firstTerm || docScores.find(docID) != docScores.end()) {
                    docScores[docID].first += freq;
                    docScores[docID].second += tfidf;
                }
            }
         freq = getFreq(); 
        }

        closeList();

        if (conjunctive && !firstTerm) {
            for (auto it = docScores.begin(); it != docScores.end();) {
                if (termIt->second.end() == std::find_if(termIt->second.begin(), termIt->second.end(),
                                                         [&](const Posting& p) { return p.docID == it->first; })) {
                    it = docScores.erase(it); // Remove documents not in the intersection
                } else {
                    ++it;
                }
            }
        }

        firstTerm = false;
    }

            // Add all results to the vector
            for (const auto& score : docScores) {
                results.push_back({score.first, score.second.first, score.second.second});
            }

            // Debug: Print results before sorting
            //std::wcout << L"Results before sorting:" << std::endl;
            //for (const auto& result : results) {
                //std::wcout << L"DocID: " << result.docID 
                        //<< L", Frequency: " << result.frequency 
                        //<< L", TF-IDF: " << result.tfidf << std::endl;
           // }

            // Sort results by descending TF-IDF
            std::sort(results.begin(), results.end(), [](const auto& a, const auto& b) {
                return a.tfidf > b.tfidf;
            });

            // Debug: Print results after sorting (optional)
           // std::wcout << L"Results after sorting:" << std::endl;
            //for (const auto& result : results) {
                //std::wcout << L"DocID: " << result.docID 
                       // << L", Frequency: " << result.frequency 
                       // << L", TF-IDF: " << result.tfidf << std::endl;
           // }

            if (results.size() > 20) results.resize(20); // Return top 20 results

            return results;

}







void InvertedIndex::openList(const std::wstring& term) const {
    currentTerm = term;

    auto it = index.find(term);
    if (it != index.end()) {
        currentPosting = it->second.begin();
        endPosting = it->second.end();

        // Debug: Log the postings list for the term
        std::wcout << L"Opening list for term: " << term << L", Postings size: " << it->second.size() << std::endl;
        if (currentPosting != endPosting) {
            
            std::wcout << L"Postings list: ";
            for (const auto& posting : it->second) {
                std::wcout << L"(DocID: " << posting.docID << L", Freq: " << posting.frequency << L") ";
            }
            //std::wcout << std::endl;
        } else {
            std::wcout << L"No postings available for term: " << term << std::endl;
        }
    } else {
        currentPosting = endPosting; // Empty if term not found
        std::wcout << L"Term not found in index: " << term << std::endl;
    }
}


void InvertedIndex::closeList() const {
    std::wcout << L"Closing list for term: " << currentTerm << std::endl;
    currentTerm.clear();
    currentPosting = endPosting;
}

int InvertedIndex::next() const {
    if (currentPosting != endPosting) {
        int docID = currentPosting->docID; // Retrieve current docID
        ++currentPosting; // Advance iterator AFTER retrieving docID
        return docID;
    }
    std::cerr << "next() called with no more postings." << std::endl;
    return -1; // Indicates the end of postings list
}


int InvertedIndex::getFreq() const {
    if (currentPosting != endPosting) {
        return currentPosting->frequency; // Retrieve the frequency of the current posting
    }
    std::cerr << "getFreq() called on invalid posting." << std::endl;
    return -1; // Indicates an invalid frequency
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
