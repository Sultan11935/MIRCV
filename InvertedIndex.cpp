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

int InvertedIndex::estimateChunkSize() {
    return 1000000; // Manually set chunk size 
}


void InvertedIndex::buildIndexSPIMI(const std::unordered_map<int, std::wstring>& documents, int chunkSize, const std::string& indexPath) {
    auto start = std::chrono::high_resolution_clock::now(); // Start timing

    struct stat info;
    if (stat(indexPath.c_str(), &info) != 0) {
        if (mkdir(indexPath.c_str(), 0777) == -1) {
            std::cerr << " Error creating directory: " << indexPath << std::endl;
            return;
        }
    }

    chunkSize = 1000000; //  Set chunk size to 200 documents

    int chunkCounter = 0;
    std::unordered_map<std::wstring, std::vector<Posting>> partialIndex;

    int processedDocs = 0;
    for (const auto& doc : documents) {
        int docID = doc.first;
        if (doc.second.empty()) continue; // Skip empty documents

        std::wstringstream wss(doc.second);
        std::wstring word;
        std::unordered_map<std::wstring, int> termFrequency;
        int docLength = 0;

        while (wss >> word) {
            try {
                word = preprocessWord(word);
                if (word.empty()) continue;

                termFrequency[word]++;
                docLength++;
            } catch (const std::exception& e) {
                std::cerr << " Error processing word in DocID: " << docID << ", Error: " << e.what() << std::endl;
                continue;
            }
        }

        if (termFrequency.empty()) continue; // Skip documents with no valid words

        for (const auto& term : termFrequency) {
            partialIndex[term.first].push_back({docID, term.second});
        }

        docLengths[docID] = docLength;
        docIDToDocno[docID] = docID;

        processedDocs++;

        //  Save partial index every 200 documents
        if (processedDocs % chunkSize == 0) {
            savePartialIndex(partialIndex, chunkCounter++, indexPath);
            partialIndex.clear();
        }
    }

    //  Save the last remaining chunk if not empty
    if (!partialIndex.empty()) {
        savePartialIndex(partialIndex, chunkCounter, indexPath);
    }

    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = end - start;

    std::cout << " Indexing completed. Created " << chunkCounter + 1 << " partial indexes in " << elapsed.count() << " seconds.\n";
}


void InvertedIndex::savePartialIndex(const std::unordered_map<std::wstring, std::vector<Posting>>& partialIndex, int chunkID, const std::string& indexPath) {
    std::string indexFilePath = indexPath + "/index_chunk_" + std::to_string(chunkID) + ".dat";
    std::string docLengthsPath = indexPath + "/doclengths_chunk_" + std::to_string(chunkID) + ".dat";
    std::string lexiconPath = indexPath + "/lexicon_chunk_" + std::to_string(chunkID) + ".dat";
    std::string docIDToDocnoPath = indexPath + "/docid_to_docno_chunk_" + std::to_string(chunkID) + ".dat";

    std::ofstream indexFile(indexFilePath);
    std::ofstream docLengthsFile(docLengthsPath);
    std::ofstream lexiconFile(lexiconPath);
    std::ofstream docIDToDocnoFile(docIDToDocnoPath);

    if (!indexFile.is_open() || !docLengthsFile.is_open() || !lexiconFile.is_open() || !docIDToDocnoFile.is_open()) {
        std::cerr << "Error opening chunk files for writing." << std::endl;
        return;
    }

    std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
    std::unordered_set<std::wstring> uniqueTerms;

    // Write partial index with grouped docIDs and frequencies
    std::unordered_map<std::wstring, std::unordered_set<int>> termDocIDCheck;
    for (const auto& entry : partialIndex) {
        std::wstring term = entry.first;
        std::vector<Posting> uniquePostings;

        for (const auto& posting : entry.second) {
            if (termDocIDCheck[term].find(posting.docID) == termDocIDCheck[term].end()) {
                termDocIDCheck[term].insert(posting.docID);
                uniquePostings.push_back(posting);
            }
        }

        if (uniquePostings.empty()) continue;

        indexFile << converter.to_bytes(term) << " " << uniquePostings.size() << " ";

        // Write docIDs
        for (const auto& posting : uniquePostings) {
            indexFile << posting.docID << " ";
        }

        // Write frequencies
        for (const auto& posting : uniquePostings) {
            indexFile << posting.frequency << " ";
        }

        indexFile << "\n";

        uniqueTerms.insert(term);
    }

    // Write the lexicon file
    for (const auto& term : uniqueTerms) {
        lexiconFile << converter.to_bytes(term) << "\n";
    }

    // Write document lengths
    for (const auto& entry : docLengths) {
        docLengthsFile << entry.first << " " << entry.second << "\n";
    }

    // Write docID to external mapping
    for (const auto& entry : docIDToDocno) {
        docIDToDocnoFile << entry.first << " " << entry.second << "\n";
    }

    // Close files
    indexFile.close();
    docLengthsFile.close();
    lexiconFile.close();
    docIDToDocnoFile.close();

    // Debugging Output
    std::cout << "Saved chunk " << chunkID << " to disk: " << indexFilePath << std::endl;
    std::cout << "Saved docLengths with size: " << docLengths.size() << std::endl;
    std::cout << "Saved lexicon with size: " << uniqueTerms.size() << std::endl;
    std::cout << "Saved docIDToDocno with size: " << docIDToDocno.size() << std::endl;
}



void InvertedIndex::mergeIndexes(int numChunks, const std::string& indexPath) {
    std::cout << " Merging " << numChunks << " index chunks into final index...\n";

    std::ofstream finalIndexFile(indexPath + "/final_index.dat");
    std::ofstream finalDocLengthsFile(indexPath + "/final_doclengths.dat");
    std::ofstream finalDocIDToDocnoFile(indexPath + "/final_docid_to_docno.dat");
    std::ofstream finalLexiconFile(indexPath + "/final_lexicon.dat");

    if (!finalIndexFile || !finalDocLengthsFile || !finalDocIDToDocnoFile || !finalLexiconFile) {
        std::cerr << "ERROR: Failed to open final index files for writing!" << std::endl;
        return;
    }

    std::unordered_map<std::wstring, std::vector<Posting>> termPostings;
    std::unordered_set<std::wstring> uniqueTerms;

    std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;

    // Read and merge postings from each chunk
    for (int i = 0; i < numChunks; ++i) {
        std::ifstream chunkFile(indexPath + "/index_chunk_" + std::to_string(i) + ".dat");
        if (!chunkFile.is_open()) {
            std::cerr << "WARNING: Could not open chunk file " << i << std::endl;
            continue;
        }

        std::string line;
        while (std::getline(chunkFile, line)) {
            if (line.empty()) continue;

            std::istringstream iss(line);
            std::string term;
            int numPostings;

            if (!(iss >> term >> numPostings)) continue;

            std::wstring wterm = utf8ToWstring(term);
            std::vector<int> docIDs(numPostings);
            std::vector<int> freqs(numPostings);

            for (int j = 0; j < numPostings; ++j) iss >> docIDs[j];
            for (int j = 0; j < numPostings; ++j) iss >> freqs[j];

            for (int j = 0; j < numPostings; ++j) {
                termPostings[wterm].push_back({docIDs[j], freqs[j]});
            }

            uniqueTerms.insert(wterm);
        }

        chunkFile.close();
    }

    // Merge postings per term and write final index
    for (const auto& [term, postingsList] : termPostings) {
        std::unordered_map<int, int> mergedMap;

        // Merge: docID → aggregated frequency
        for (const auto& posting : postingsList) {
            mergedMap[posting.docID] += posting.frequency;
        }

        // Convert to Posting list
        std::vector<Posting> mergedPostings;
        for (const auto& [docID, freq] : mergedMap) {
            mergedPostings.push_back({docID, freq});
        }

        // Sort by docID
        std::sort(mergedPostings.begin(), mergedPostings.end(), [](const Posting& a, const Posting& b) {
            return a.docID < b.docID;
        });

        // Write to final index in grouped format
        finalIndexFile << converter.to_bytes(term) << " " << mergedPostings.size() << " ";
        for (const auto& p : mergedPostings) finalIndexFile << p.docID << " ";
        for (const auto& p : mergedPostings) finalIndexFile << p.frequency << " ";
        finalIndexFile << "\n";

        finalLexiconFile << converter.to_bytes(term) << "\n";
    }

    // Merge document lengths
    for (int i = 0; i < numChunks; i++) {
        std::ifstream dlFile(indexPath + "/doclengths_chunk_" + std::to_string(i) + ".dat");
        std::string line;
        while (std::getline(dlFile, line)) {
            finalDocLengthsFile << line << "\n";
        }
        dlFile.close();
    }

    // Merge docID-to-docno mapping
    for (int i = 0; i < numChunks; i++) {
        std::ifstream didFile(indexPath + "/docid_to_docno_chunk_" + std::to_string(i) + ".dat");
        std::string line;
        while (std::getline(didFile, line)) {
            finalDocIDToDocnoFile << line << "\n";
        }
        didFile.close();
    }

    std::cout << " Final index merge completed successfully.\n";
}






void InvertedIndex::loadIndex(const std::string& indexPath) {
    std::ifstream indexFile(indexPath + "/final_index.dat");
    std::ifstream lexiconFile(indexPath + "/final_lexicon.dat");
    std::ifstream docLengthsFile(indexPath + "/final_doclengths.dat");
    std::ifstream docIDToDocnoFile(indexPath + "/final_docid_to_docno.dat");

    if (!indexFile.is_open() || !lexiconFile.is_open() || !docLengthsFile.is_open() || !docIDToDocnoFile.is_open()) {
        std::cerr << " ERROR: One or more required index files are missing. Aborting index load.\n";
        return;
    }

    std::string line;
    while (std::getline(indexFile, line)) {
        if (line.empty()) continue;

        std::istringstream iss(line);
        std::string term;
        int numPostings;

        if (!(iss >> term >> numPostings)) continue;

        std::wstring wterm = utf8ToWstring(term);
        std::vector<int> docIDs(numPostings);
        std::vector<int> freqs(numPostings);

        // Read docIDs
        for (int i = 0; i < numPostings; ++i) {
            if (!(iss >> docIDs[i])) {
                std::cerr << " ERROR reading docID for term: " << term << std::endl;
                break;
            }
        }

        // Read frequencies
        for (int i = 0; i < numPostings; ++i) {
            if (!(iss >> freqs[i])) {
                std::cerr << " ERROR reading frequency for term: " << term << std::endl;
                break;
            }
        }

        // Combine into postings
        std::vector<Posting> postings;
        for (int i = 0; i < numPostings; ++i) {
            postings.push_back({docIDs[i], freqs[i]});
        }

        if (!postings.empty()) {
            index[wterm] = postings;
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

    // Load docID-to-docno mapping
    while (std::getline(docIDToDocnoFile, line)) {
        std::istringstream iss(line);
        int docID, docno;
        if (iss >> docID >> docno) {
            docIDToDocno[docID] = docno;
        }
    }

    std::cout << "Index successfully loaded from disk." << std::endl;
}





// -------------------- Optimized TF-IDF Search --------------------
std::vector<SearchResult> InvertedIndex::searchWithTFIDF(const std::wstring& query, bool conjunctive) const {
    std::vector<SearchResult> results;
    std::wstringstream wss(query);
    std::wstring word;
    std::vector<std::wstring> terms;

    // Preprocess query terms
    while (wss >> word) {
        word = preprocessWord(word);
        if (!word.empty()) {
            terms.push_back(word);
        }
    }

    if (terms.empty()) {
        std::wcout << L" Query resulted in no valid terms after preprocessing!" << std::endl;
        return results;
    }

    std::unordered_map<int, std::pair<int, double>> docScores;
    std::vector<std::unordered_set<int>> docSets;

    for (const auto& term : terms) {
        auto it = index.find(term);
        if (it == index.end()) {
            std::wcout << L" Term not found in index: " << term << std::endl;
            if (conjunctive) return {}; // If one term is missing, abort for conjunctive
            continue;
        }

        std::unordered_set<int> termDocIDs;

        for (const auto& posting : it->second) {
            termDocIDs.insert(posting.docID);

            // For disjunctive, accumulate scores directly
            if (!conjunctive) {
                double tfidf = computeTFIDF(posting.frequency, docLengths.at(posting.docID), it->second.size());
                docScores[posting.docID].first += posting.frequency;
                docScores[posting.docID].second += tfidf;
            }
        }

        docSets.push_back(std::move(termDocIDs));
    }

    // Handle conjunctive logic
    if (conjunctive && !docSets.empty()) {
        // Intersect all doc sets
        std::unordered_set<int> intersection = docSets[0];
        for (size_t i = 1; i < docSets.size(); ++i) {
            std::unordered_set<int> temp;
            for (int docID : intersection) {
                if (docSets[i].count(docID)) {
                    temp.insert(docID);
                }
            }
            intersection = std::move(temp);
        }

        // Compute TF-IDF for intersected documents only
        for (int docID : intersection) {
            int totalFreq = 0;
            double totalTFIDF = 0.0;

            for (const auto& term : terms) {
                const auto& postings = index.at(term);
                auto it = std::find_if(postings.begin(), postings.end(), [&](const Posting& p) {
                    return p.docID == docID;
                });

                if (it != postings.end()) {
                    totalFreq += it->frequency;
                    totalTFIDF += computeTFIDF(it->frequency, docLengths.at(docID), postings.size());
                }
            }

            docScores[docID] = {totalFreq, totalTFIDF};
        }
    }

    // Sort results by TF-IDF
    for (const auto& [docID, score] : docScores) {
        results.push_back({docID, score.first, score.second});
    }

    std::sort(results.begin(), results.end(), [](const auto& a, const auto& b) {
        return a.tfidf > b.tfidf;
    });

    std::wcout << L" TF-IDF Results Count: " << results.size() << std::endl;
    if (results.empty()) {
        std::wcout << L" No documents found matching the query!" << std::endl;
    }

    if (results.size() > 20) results.resize(20);

    return results;
}






void InvertedIndex::openList(const std::wstring& term) const {
    currentTerm = term;
    auto it = index.find(term);

    if (it != index.end() && !it->second.empty()) {
        currentPosting = it->second.begin();
        endPosting = it->second.end();
        //std::wcout << L" Term Found: " << term << L", Postings Size: " << it->second.size() << std::endl;
    } else {
        currentPosting = endPosting;
        std::wcout << L" Term Not Found in Index: " << term << std::endl;
    }
}


void InvertedIndex::closeList() const {
    currentTerm.clear();
    currentPosting = endPosting;
}

int InvertedIndex::next() const {
    if (currentPosting != endPosting) {
        int docID = currentPosting->docID;
        ++currentPosting; // Move to the next posting
        return docID;
    }
    return -1; // End of postings list
}

int InvertedIndex::getFreq() const {
    if (currentPosting != endPosting) {
        return currentPosting->frequency;
    }
    return 0; // Return 0 instead of -1 for empty postings
}

std::wstring InvertedIndex::preprocessWord(const std::wstring& word) const {
    std::wstring result = word;
    std::transform(result.begin(), result.end(), result.begin(), ::towlower);
    result.erase(std::remove_if(result.begin(), result.end(), ::iswpunct), result.end());

    //std::wcout << L" Preprocessed Word: " << word << L" → " << result << std::endl;
    return result;
}


double InvertedIndex::computeTF(int termFreq, int docLength) const {
    return (docLength == 0) ? 0 : static_cast<double>(termFreq) / docLength;
}

double InvertedIndex::computeIDF(int docCount) const {
    return (docCount == 0) ? 0 : std::log(static_cast<double>(docLengths.size()) / (docCount + 1));
}

double InvertedIndex::computeTFIDF(int termFreq, int docLength, int docCount) const {
    return computeTF(termFreq, docLength) * computeIDF(docCount);
}
