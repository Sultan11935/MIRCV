#include <iostream>
#include <filesystem>  // For directory handling
#include "DocumentParser.h"
#include "InvertedIndex.h"
#include "QueryProcessor.h"

int main(int argc, char* argv[]) {
    if (argc < 2 || argc > 3) {
        std::cerr << "Usage: " << argv[0] << " <dataset_path> [num_docs]" << std::endl;
        return 1;
    }

    std::string datasetPath = argv[1];
    int numDocs = (argc == 3) ? std::stoi(argv[2]) : -1;
    std::string indexPath = "/home/sultan/MIRCV_Project/main/index_files";

    std::cout << " Using dataset path: " << datasetPath << std::endl;

    //  Parse documents
    DocumentParser parser(datasetPath, numDocs);
    parser.parseDocuments();

    //  Initialize Inverted Index
    InvertedIndex index;
    int chunkSize = index.estimateChunkSize();
    std::cout << " Estimated Chunk Size: " << chunkSize << std::endl;

    //  Ensure index directory exists
    if (!std::filesystem::exists(indexPath)) {
        std::filesystem::create_directories(indexPath);
    }

    //  Build SPIMI Index
    std::cout << " Building Index using SPIMI..." << std::endl;
    index.buildIndexSPIMI(parser.getDocuments(), chunkSize, indexPath);

    //  Automatically detect number of chunk files for merging
    int numChunks = 0;
    for (const auto& entry : std::filesystem::directory_iterator(indexPath)) {
        if (entry.path().string().find("index_chunk_") != std::string::npos) {
            numChunks++;
        }
    }

    if (numChunks == 0) {
        std::cerr << " ERROR: No index chunks found. Index merging cannot proceed." << std::endl;
        return 1;
    }

    std::cout << " Merging " << numChunks << " index chunks into final index..." << std::endl;
    index.mergeIndexes(numChunks, indexPath);

    //  Load the final merged index
    std::cout << " Loading index from disk..." << std::endl;
    index.loadIndex(indexPath);
    std::cout << " Index loaded successfully!" << std::endl;

    //  Start Query Processing
    QueryProcessor qp(index);
    std::cout << " Starting query processing..." << std::endl;
    qp.processQueries();
    std::cout << " Finished query processing." << std::endl;

    return 0;
}
