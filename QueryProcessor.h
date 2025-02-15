#ifndef QUERY_PROCESSOR_H
#define QUERY_PROCESSOR_H

#include "InvertedIndex.h"
#include <string>

class QueryProcessor {
public:
    QueryProcessor(const InvertedIndex& index);
    void processQueries() const;

private:
    const InvertedIndex& index;
};

#endif
