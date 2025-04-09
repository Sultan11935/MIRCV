# MIRCV Inverted Index Search Engine

A C++ information retrieval engine for ranked TF-IDF search using an inverted index. Supports conjunctive (AND) and disjunctive (OR) query modes, custom iterator logic (`openList`, `next`, `getFreq`, `closeList`), stemming, and stopword removal.

## Features

This project was developed according to the MIRCV project specification and includes the following features:

### Mandatory Requirements (All Fulfilled)

- Written entirely in **C++**, using modern C++17 features
- Indexed dataset is read using **UTF-8/UNICODE**, supports multilingual content
- Handles malformed, empty, and punctuated lines gracefully
- Implements a custom **inverted index** (term → postings list) without using databases or high-level collections
- Implements:
  - A **lexicon**: maps each term to its postings
  - A **document table**: stores docno mappings and document lengths
- Stores the entire index (postings, lexicon, doc table) on **disk**, not in memory
- Uses a multi-phase indexing pipeline:
  - Parses documents and builds **SPIMI-based partial indexes**
  - Merges partial chunks into a final index
- Reads index data from disk at query time — only query terms are accessed, **not the entire index**
- Fully supports **TF-IDF scoring** with the formula:
  - `TF = freq / docLength`
  - `IDF = log(N / (df + 1))`
  - `TF-IDF = TF × IDF`
- Executes **ranked conjunctive (AND)** and **disjunctive (OR)** queries
- Follows the iterator-style interface:
  - `openList(term)`
  - `next()`
  - `getFreq()`
  - `closeList()`
- Provides a **command-line interface (CLI)** for user input:
  - Accepts queries interactively
  - Outputs top-20 results sorted by TF-IDF
- Includes a `README.md` and well-commented codebase

---

### Optional Requirements (Partially Fulfilled)

- **Stemming support** using the Snowball stemmer (enabled via `-DENABLE_STEMMING`)
- **Stopword removal** using a predefined stopword list (enabled via `-DENABLE_STOPWORDS`)
- Supports **compile-time flags** to enable/disable preprocessing options
- Supports **SPIMI chunking** and **merging** for large-scale document collections
- Handles document input from **MSMARCO dataset format**: `<pid>\t<text>`
- Document parsing and query execution are performed on UTF-8 encoded `.tsv` files

---

## Project Structure

. ├── main.cpp # Program entry point ├── DocumentParser.* # Tokenizes and processes documents ├── InvertedIndex.* # Builds and queries inverted index ├── QueryProcessor.* # Handles user query and interaction ├── utils.* # Preprocessing (e.g., lowercase, punctuation) ├── snowball/ # Optional stemming library ├── dataset/ # Source documents (.tsv, .txt) ├── index/ # Saved chunked indexes


## Build Instructions

Requirements:
- GCC with C++17 support
- Snowball stemming library

### Minimal build (no stemming or stopwords)


g++ -o InvertedIndex main.cpp DocumentParser.cpp InvertedIndex.cpp QueryProcessor.cpp utils.cpp \
    -std=c++17 -finput-charset=UTF-8 -fexec-charset=UTF-8
    
With stemming support

g++ -o InvertedIndex main.cpp DocumentParser.cpp InvertedIndex.cpp QueryProcessor.cpp utils.cpp \
    -L/home/sultan/MIRCV_Project/snowball -lstemmer \
    -I/home/sultan/MIRCV_Project/snowball/include \
    -std=c++17 -finput-charset=UTF-8 -fexec-charset=UTF-8 -DENABLE_STEMMING
    
With stopword removal

g++ -o InvertedIndex main.cpp DocumentParser.cpp InvertedIndex.cpp QueryProcessor.cpp utils.cpp \
    -L/home/sultan/MIRCV_Project/snowball -lstemmer \
    -I/home/sultan/MIRCV_Project/snowball/include \
    -std=c++17 -finput-charset=UTF-8 -fexec-charset=UTF-8 -DENABLE_STOPWORDS
    
With both stemming and stopword removal

g++ -o InvertedIndex main.cpp DocumentParser.cpp InvertedIndex.cpp QueryProcessor.cpp utils.cpp \
    -L/home/sultan/MIRCV_Project/snowball -lstemmer \
    -I/home/sultan/MIRCV_Project/snowball/include \
    -std=c++17 -finput-charset=UTF-8 -fexec-charset=UTF-8 -DENABLE_STEMMING -DENABLE_STOPWORDS
    
How to Run
Run the program with a dataset file:

./InvertedIndex /home/sultan/MIRCV_Project/dataset/final_dataset/collection_small.tsv
