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
    return 1000000; // Manually set chunk size to 200 documents
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

    // Write partial index
    // Ensure term postings are unique
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

        indexFile << converter.to_bytes(term) << " ";
        for (const auto& posting : uniquePostings) {
            indexFile << posting.docID << ":" << posting.frequency << " ";
        }
        indexFile << "\n";
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
    std::cout << "🔄 Merging " << numChunks << " index chunks into final index...\n";

    // Open final index output files
    std::ofstream finalIndexFile(indexPath + "/final_index.dat");
    std::ofstream finalDocLengthsFile(indexPath + "/final_doclengths.dat");
    std::ofstream finalDocIDToDocnoFile(indexPath + "/final_docid_to_docno.dat");
    std::ofstream finalLexiconFile(indexPath + "/final_lexicon.dat");

    if (!finalIndexFile || !finalDocLengthsFile || !finalDocIDToDocnoFile || !finalLexiconFile) {
        std::cerr << " ERROR: Failed to open final index files for writing!" << std::endl;
        return;
    }

    struct ChunkState {
        std::ifstream file;
        std::string currentTerm;
        std::queue<std::string> postings;
    };

    std::vector<ChunkState> chunks(numChunks);
    std::priority_queue<MergeNode, std::vector<MergeNode>, std::greater<MergeNode>> minHeap;

    // Initialize chunks and read first line
    for (int i = 0; i < numChunks; ++i) {
        std::string chunkPath = indexPath + "/index_chunk_" + std::to_string(i) + ".dat";
        chunks[i].file.open(chunkPath);
        if (!chunks[i].file) {
            std::cerr << " WARNING: Missing index chunk file: " << chunkPath << std::endl;
            continue;
        }

        std::string line;
        if (std::getline(chunks[i].file, line)) {
            std::istringstream iss(line);
            if (iss >> chunks[i].currentTerm) {
                std::string posting;
                while (iss >> posting) {
                    chunks[i].postings.push(posting);
                }
                minHeap.push({utf8ToWstring(chunks[i].currentTerm), i});
            }
        }
    }

    
    // Inside mergeIndexes function
    std::unordered_map<std::wstring, std::vector<std::string>> termPostings;
    std::unordered_set<std::wstring> uniqueTerms;

    for (int i = 0; i < numChunks; ++i) {
        std::ifstream chunkFile(indexPath + "/index_chunk_" + std::to_string(i) + ".dat");
        if (!chunkFile) {
            std::cerr << " WARNING: Missing index chunk file: " << indexPath + "/index_chunk_" + std::to_string(i) + ".dat" << std::endl;
            continue;
        }

        std::string line;
        while (std::getline(chunkFile, line)) {
            if (line.empty()) continue;
            std::istringstream iss(line);
            std::string term;
            if (!(iss >> term)) continue;

            std::wstring wterm = utf8ToWstring(term);
            std::vector<std::string> postings;
            std::string posting;
            while (iss >> posting) {
                postings.push_back(posting);
            }

            auto &existingPostings = termPostings[wterm];
            existingPostings.insert(existingPostings.end(), postings.begin(), postings.end());
        }
    }

    for (auto &[term, postings] : termPostings) {
        std::sort(postings.begin(), postings.end());
        postings.erase(std::unique(postings.begin(), postings.end()), postings.end());

        finalIndexFile << wstringToUtf8(term);
        for (const auto &posting : postings) {
            finalIndexFile << " " << posting;
        }
        finalIndexFile << "\n";

        if (uniqueTerms.insert(term).second) {
            finalLexiconFile << wstringToUtf8(term) << "\n";
        }
    }

    // Merge document lengths
    for (int i = 0; i < numChunks; i++) {
        std::ifstream dlFile(indexPath + "/doclengths_chunk_" + std::to_string(i) + ".dat");
        if (dlFile) {
            std::string line;
            while (std::getline(dlFile, line)) {
                //std::cout << "Writing to final_doclengths: " << line << std::endl;
                finalDocLengthsFile << line << "\n";
            }
            dlFile.close();
        }
    }

    // Merge docID to docno mapping
    for (int i = 0; i < numChunks; i++) {
        std::ifstream didFile(indexPath + "/docid_to_docno_chunk_" + std::to_string(i) + ".dat");
        if (didFile) {
            std::string line;
            while (std::getline(didFile, line)) {
                //std::cout << "Writing to final_docid_to_docno: " << line << std::endl;
                finalDocIDToDocnoFile << line << "\n";
            }
            didFile.close();
        }
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
        if (!(iss >> term)) continue;

        std::wstring wterm = utf8ToWstring(term);
        std::vector<Posting> postings;
        std::string postingStr;
        while (iss >> postingStr) {
            size_t colonPos = postingStr.find(':');
            if (colonPos == std::string::npos) continue;
            try {
                int docID = std::stoi(postingStr.substr(0, colonPos));
                int freq = std::stoi(postingStr.substr(colonPos + 1));
                postings.push_back({docID, freq});
            } catch (...) {
                std::cerr << " ERROR parsing posting: " << postingStr << std::endl;
            }
        }
        if (!postings.empty()) index[wterm] = postings;
    }
}




// -------------------- Optimized TF-IDF Search --------------------
std::vector<SearchResult> InvertedIndex::searchWithTFIDF(const std::wstring& query, bool conjunctive) const {
    std::vector<SearchResult> results;
    std::wstringstream wss(query);
    std::wstring word;
    std::vector<std::wstring> terms;

    std::wcout << L"🔍 Received Query: " << query << std::endl;

    // Process the query words
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
    bool firstTerm = true;

    for (const auto& term : terms) {
        std::wcout << L"🔎 Searching for term: " << term << std::endl;
        openList(term);

        if (currentPosting == endPosting) {
            std::wcout << L" Term not found in index: " << term << std::endl;
            closeList();
            if (conjunctive) return {}; // If conjunctive and a term is missing, return empty
            continue;
        }

        int docID, freq = getFreq();
        while ((docID = next()) != -1) {
            if (freq == -1) continue; // Skip invalid postings

            auto docLengthIt = docLengths.find(docID);
            if (docLengthIt != docLengths.end()) {
                double tfidf = computeTFIDF(freq, docLengthIt->second, terms.size());
                docScores[docID].first += freq;
                docScores[docID].second += tfidf;

                // Debug TF-IDF calculations
                std::wcout << L" DocID: " << docID << L", Freq: " << freq
                           << L", Doc Length: " << docLengthIt->second
                           << L", TF-IDF: " << tfidf << std::endl;
            }
            freq = getFreq();
        }

        closeList();

        if (conjunctive && !firstTerm) {
            for (auto it = docScores.begin(); it != docScores.end();) {
                if (std::find_if(index[term].begin(), index[term].end(),
                                 [&](const Posting& p) { return p.docID == it->first; }) == index[term].end()) {
                    it = docScores.erase(it);
                } else {
                    ++it;
                }
            }
        }

        firstTerm = false;
    }

    // Sort results by TF-IDF
    for (const auto& score : docScores) {
        results.push_back({score.first, score.second.first, score.second.second});
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
        std::wcout << L" Term Found: " << term << L", Postings Size: " << it->second.size() << std::endl;
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