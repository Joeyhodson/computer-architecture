// Joey Hodson
// EEL 4768 - Computer Architecture
// Project 1 - Cache Simulator
// 10.29.2021

// Notes: 
// 1) Assume 64B block size for all configurations
// 2) replacement policy: 0 means LRU, 1 means FIFO
// 3) Write-back policy: 0 means write-through, 1 means write-back
// 4) address can be up to 6 bytes

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define BLOCK_SIZE 64
#define BLOCK_OFFSET_BIT_COUNT 6

#define FIFO_REPLACEMENT_POLICY 1
#define WRITE_BACK_REPLACEMENT_POLICY 1
#define LRU_REPLACEMENT_POLICY 0
#define WRITE_THROUGH_REPLACEMENT_POLICY 0

#define NOT_DIRTY 0
#define DIRTY 1

#define WRITE_OPERATION 'W'
#define READ_OPERATION 'R'

int memoryWrites = 0, hits = 0, misses = 0;
int writeBackPolicy, replacementPolicy;

int printVerbose = 0;

typedef struct cacheBlock {
    long long int tag;
    int dirty;
} cacheBlock;

cacheBlock* createCacheBlock(long long int tag, int dirty) {
    
    cacheBlock* block = (cacheBlock*)malloc(sizeof(cacheBlock));
    block->tag = tag;
    block->dirty = dirty;

    return block;
}

void bubbleElementUp(cacheBlock*** cache, int setIndex, int indexToBubbleUp) {
    
    for (int i = indexToBubbleUp; i >= 1; i--) {
        cacheBlock* temp = cache[setIndex][i];
        cache[setIndex][i] = cache[setIndex][i-1];
        cache[setIndex][i-1] = temp;
    }
}

void printCacheElement(cacheBlock* cacheBlock) {
    
    if (cacheBlock == NULL) {
        printf("[], ");
    }
    else {
        printf("[tag=%lld,dirty=%d], ", cacheBlock->tag, cacheBlock->dirty);
    }
}

void printCache(cacheBlock*** cache, int setIndex, int associativity) {
    
    for (int i = 0; i < associativity; ++i) {
        printCacheElement(cache[setIndex][i]);
    }

    printf("\n");
}

int getIndexOfLastElement(cacheBlock*** cache, int setIndex, int associativity) {

    for (int i = associativity-1; i >= 0; i--) {
        if (cache[setIndex][i] != NULL) {
            return i;
        }
    }

    return -1;
}

void updateLRU(
    cacheBlock*** cache,
    int setIndex,
    long long int tag,
    int associativity,
    int blockIndex,
    int inCache,
    char operation) {

    if (printVerbose) {
        printCache(cache, setIndex, associativity);
    }

    int indexToBubbleUp;
    if (inCache) {
        indexToBubbleUp = blockIndex;
    }
    else {

        int indexOfLastElement = getIndexOfLastElement(cache, setIndex, associativity);

        // if the cache is full and the address is not in cache
        if (indexOfLastElement == (associativity - 1)) {

            if (cache[setIndex][associativity - 1]->dirty) {
                cache[setIndex][associativity - 1]->dirty = NOT_DIRTY;
                memoryWrites++;
            }
            cache[setIndex][associativity - 1]->tag = tag;
            indexToBubbleUp = associativity - 1;
        }
        else {

            // else the cache is not full and the address is not in cache
            cache[setIndex][indexOfLastElement + 1] = createCacheBlock(tag, NOT_DIRTY);
            indexToBubbleUp = indexOfLastElement + 1;
        }
    }

    if (writeBackPolicy == WRITE_BACK_REPLACEMENT_POLICY &&
        operation == WRITE_OPERATION) {

        cache[setIndex][indexToBubbleUp]->dirty = DIRTY;
    }
    bubbleElementUp(cache, setIndex, indexToBubbleUp);
}

void makeBlockDirty(cacheBlock*** cache, int setIndex, int block) {
    
    if (cache[setIndex][block] == NULL) {
        printf("makeBlockDirty encountered an unexpectedly null cache block.");
    }
    else {
        cache[setIndex][block]->dirty = DIRTY;
    }
}

void updateFIFO(cacheBlock*** cache, int setIndex, long long int tag, int associativity, int blockIndex, int inCache, char operation) {

    if (printVerbose) {
        printCache(cache, setIndex, associativity);
    }

    int addedOrUpdatedElement;
    if (inCache) {
        addedOrUpdatedElement = blockIndex;
    }
    else {

        int indexOfLastElement = getIndexOfLastElement(cache, setIndex, associativity);

        // if the cache is full and the address is not in cache
        if (indexOfLastElement == (associativity - 1)) {

            if (cache[setIndex][associativity - 1]->dirty) {
                cache[setIndex][associativity - 1]->dirty = NOT_DIRTY;
                memoryWrites++;
            }
            cache[setIndex][associativity - 1]->tag = tag;
            addedOrUpdatedElement = associativity - 1;
        }
        else {

            // else the cache is not full and the address is not in cache
            cache[setIndex][indexOfLastElement + 1] = createCacheBlock(tag, NOT_DIRTY);
            addedOrUpdatedElement = indexOfLastElement + 1;
        }

        bubbleElementUp(cache, setIndex, addedOrUpdatedElement);
    }

    
    if (writeBackPolicy == WRITE_BACK_REPLACEMENT_POLICY &&
        operation == WRITE_OPERATION) {

        cache[setIndex][addedOrUpdatedElement]->dirty = DIRTY;
    }
}

int isInCache(cacheBlock*** cache, int setIndex, long long int tag, int associativity) {
    
    int blockIndex = -1;
    for (int i = 0; i < associativity; i++) {
        if (cache[setIndex][i] == NULL) {
            break;
        }
        if (cache[setIndex][i]->tag == tag) {
            blockIndex = i;
            break;
        }
    }

    return blockIndex;
}

void updateCacheAndRecordStatistics(cacheBlock*** cache, int setIndex, long long int tag, int associativity, char operation) {
    
    int blockIndex = isInCache(cache, setIndex, tag, associativity);
    int inCache = blockIndex != -1;
    if (inCache) {
        hits++;
    }
    else {
        misses++;
    }

    // update cache according to replacement policy
    if (replacementPolicy == FIFO_REPLACEMENT_POLICY) {
        updateFIFO(cache, setIndex, tag, associativity, blockIndex, inCache, operation);
    }
    else {
        updateLRU(cache, setIndex, tag, associativity, blockIndex, inCache, operation);
    }
}

void read(cacheBlock*** cache, int setIndex, long long int tag, int associativity) {
    
    updateCacheAndRecordStatistics(cache, setIndex, tag, associativity, READ_OPERATION);
}

void write(cacheBlock*** cache, int setIndex, long long int tag, int associativity) {

    // write through policy
    if (writeBackPolicy != WRITE_BACK_REPLACEMENT_POLICY) {
        memoryWrites++;
    }

    updateCacheAndRecordStatistics(cache, setIndex, tag, associativity, WRITE_OPERATION);
}

int logTwo(unsigned int num) {

    int base = -1;
    while(num != 0) {
        num = num >> 1;
        base++;
    }

    return base;
}

long long int getTag(long long int address, int numSets) {

    int setIndexBitCount = logTwo(numSets);
    long long int tag = (address>>BLOCK_OFFSET_BIT_COUNT)>>setIndexBitCount;

    return tag;
}

long long int getSetIndex(long long int address, int numSets) {

    int setIndexBitCount = logTwo(numSets);
    unsigned int mask = (1 << setIndexBitCount) - 1;
    long long int setIndex = (address >> BLOCK_OFFSET_BIT_COUNT) & mask;

    return setIndex;
}

int simulate(cacheBlock*** cache, int numSets, int associativity, FILE* inFile) {

    if (cache == NULL) {
        return -1;
    }

    char op;
    long long int address, tag;
    int setIndex;

    while (!feof(inFile)) {

        fscanf(inFile, "%c %llx\n", &op, &address);

        tag = getTag(address, numSets);
        setIndex = getSetIndex(address, numSets);

        if (printVerbose) {
            printf("address=%llx,tag=%llx,setIndex=%d,op=%c\n", address, tag, setIndex, op);
        }

        if (op == READ_OPERATION) {
            read(cache, setIndex, tag, associativity);
        }
        else if (op == WRITE_OPERATION) {
            write(cache, setIndex, tag, associativity);
        }
        else {
            printf("Operation is unexpected!\n");
            return -1;
        }
    }

    return 0;
}

void freeAll(cacheBlock*** cache, int numSets, int associativity) {

    for (int i = 0; i < numSets; i++) {
        for (int j = 0; j < associativity; j++) {
            if (cache[i][j] != NULL) {
                free(cache[i][j]);
            }
        }
        free(cache[i]);
    }
    free(cache);
}

void output(cacheBlock*** cache, FILE* outFile, int cacheSize, int associativity, int replacementPolicy, int writeBackPolicy) {

    char replacementPolicyString[18];
    char writeBackPolicyString[18];
    
    if (writeBackPolicy == WRITE_BACK_REPLACEMENT_POLICY) {
        strcpy(writeBackPolicyString, "back");
    }
    else {
        strcpy(writeBackPolicyString,"through");
    }

    if (replacementPolicy == FIFO_REPLACEMENT_POLICY) {
        strcpy(replacementPolicyString, "FIFO");
    }
    else {
        strcpy(replacementPolicyString, "LRU");
    }

    printf(
        "%fKB cache -- %d-way associative -- %s replacement -- write-%s\n", 
        (double)cacheSize/1024,
        associativity,
        replacementPolicyString,
        writeBackPolicyString);

    printf("Cache Hits: %d\n", hits);
    printf("Cache Misses: %d\n", misses);
    printf("Miss Ratio: %0.6lf\n", ((double)misses)/(hits+misses));

    printf("Memory Reads: %d\n", misses);
    printf("Memory Writes: %d\n", memoryWrites);


    fprintf(
        outFile,
        "%fKB cache -- %d-way associative -- %s replacement -- write-%s\n", 
        (double)cacheSize/1024,
        associativity,
        replacementPolicyString,
        writeBackPolicyString);

    fprintf(outFile, "Cache Hits: %d\n", hits);
    fprintf(outFile, "Cache Misses: %d\n", misses);
    fprintf(outFile, "Miss Ratio: %0.6lf\n", ((double)misses)/(hits+misses));

    fprintf(outFile, "Memory Reads: %d\n", misses);
    fprintf(outFile, "Memory Writes: %d\n", memoryWrites);
}

int driver(int cacheSize, int associativity, int replacement, int writeBack, char *traceFile) {

    writeBackPolicy = writeBack;
    replacementPolicy = replacement;

    FILE* inFile = fopen(traceFile, "r");
    if (inFile == NULL) {
        return -1;
    }

    FILE* outFile = fopen("out.txt", "w");

    int numSets = cacheSize/(BLOCK_SIZE*associativity);

    // cache[numSets][associativity]
    struct cacheBlock*** cache = (cacheBlock***)malloc(sizeof(cacheBlock**)*numSets);
    for (int i = 0; i < numSets; i++) {
        cache[i] = (cacheBlock**)malloc(sizeof(cacheBlock*)*associativity);

        for (int j = 0; j < associativity; j++) {
            cache[i][j] = NULL;
        }
    }

    simulate(cache, numSets, associativity, inFile);

    output(cache, outFile, cacheSize, associativity, replacementPolicy, writeBackPolicy);

    freeAll(cache, numSets, associativity);

    fclose(inFile);
    fclose(outFile);
}

int main(int argc, char *argv[]) {

    if (argc != 6) {
        return -1;
    }

    char* remaining = NULL;

    int cacheSize = strtol(argv[1], &remaining, 10);
    if (strlen(remaining) != 0 || cacheSize <= 0) { return -1; }

    int associativity = strtol(argv[2], &remaining, 10);
    if (strlen(remaining) != 0 || associativity < 1) { return -1; }

    int replacement = strtol(argv[3], &remaining, 10);
    if (strlen(remaining) || (replacement != 0 && replacement != 1)) { return -1; }

    int writeBack = strtol(argv[4], &remaining, 10);
    if (strlen(remaining) != 0 || (writeBack != 0 && writeBack != 1)) { return -1; }
    
    char *traceFile = (char*)malloc(100*sizeof(char));
    strcpy(traceFile, argv[5]);

    return driver(cacheSize, associativity, replacement, writeBack, traceFile);
}