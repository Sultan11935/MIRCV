#include <iostream>
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

    std::cout << "Using dataset path: " << datasetPath << std::endl;

    DocumentParser parser(datasetPath, numDocs);
    parser.parseDocuments();

    InvertedIndex index;
    index.buildIndex(parser.getDocuments());
    index.saveIndex("/home/sultan/MIRCV_Project/main/index_files");

    // Load the index from disk
    std::cout << "Loading index from disk..." << std::endl;
    index.loadIndex("/home/sultan/MIRCV_Project/main/index_files");
    std::cout << "Index loaded." << std::endl;

    QueryProcessor qp(index);

    std::cout << "Starting query processing..." << std::endl;
    qp.processQueries();
    std::cout << "Finished query processing." << std::endl;

    return 0;
}
