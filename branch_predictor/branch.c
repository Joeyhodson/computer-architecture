// Joey Hodson
// EEL 4768 - Computer Architecture
// Project 2 - Branch Predictor
// 11.30.2021

// Notes: 
// 1) M = entry table # of bits
//    N = global history register # of bits

/* README:

    command line:

    gcc ./branch.c -o ./main.out                        -----> this compiles
    ./main.out 5 4 ./trace_files/mcf_trace.txt          -----> this executes

    /sim gshare <GPB> <RB> <Trace_File>
    1) GPB (M) = The number of PC bits used to index the gshare table.
    2) RB (N) = The global history register bits used to index the gshare table.
    3) Trace_File = The trace file name along with its extension
    
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>


#define WEAKLY_NOT_TAKEN 1
#define STRONGLY_NOT_TAKEN 0
#define WEAKLY_TAKEN 2
#define STRONGLY_TAKEN 3

#define BYTE_OFFSET_BIT_COUNT 2

int printVerbose = 0;

int power(int base, int exponent) {

    if (exponent == 0) {
        return 1;
    }
    else {
        return base*power(base, exponent - 1);
    }
}

void initializeEntryTable(int* entryTable, int M) {

    for (int i = 0; i < power(2, M); i++) {
        entryTable[i] = WEAKLY_TAKEN;
    }
}


void output(FILE* outFile, char* traceFile, int M, int N, double mispredictionRate) {

    fprintf(outFile, "Trace file: %s\n", traceFile);
    fprintf(outFile, "Configuration: M = %d, N = %d\n", M, N);
    fprintf(outFile, "Misprediction Rate: %.2f%%\n", 100*mispredictionRate);

    printf("Trace file: %s\n", traceFile);
    printf("Configuration: M = %d, N = %d\n", M, N);
    printf("Misprediction Rate: %.2f%%\n", 100*mispredictionRate);
}


long long int getEntryTableIndex(long long int address, int* globalHistoryRegister, int M, int N) {

    unsigned int mask = (1 << M) - 1;
    long long int valueOfMBitField = (address >> BYTE_OFFSET_BIT_COUNT) & mask;

    long long int shiftedGHR = (*globalHistoryRegister << (M - N));

    long long int index = (valueOfMBitField ^ shiftedGHR);
    
    return index;
}


void updateGHR(char actualPath, int* globalHistoryRegister, int N) {

    *globalHistoryRegister = ((*globalHistoryRegister) >> 1);
    if (actualPath == 't') {
        int mask = (1 << (N-1));
        *globalHistoryRegister = ((*globalHistoryRegister) | mask);
    }
}


void updateEntryTable(int* entryTable, long long int entryTableIndex, char actualPath, int* mispredictions) {

    int currentPredictionAtEntry = entryTable[entryTableIndex];
    
    switch(currentPredictionAtEntry) {
        // Not taken
        case STRONGLY_NOT_TAKEN:
            if (actualPath == 't') {
                (*mispredictions)++;
                entryTable[entryTableIndex] = WEAKLY_NOT_TAKEN;
            }
            else {}
            break;

        case WEAKLY_NOT_TAKEN:
            if (actualPath == 't') {
                (*mispredictions)++;
                entryTable[entryTableIndex] = WEAKLY_TAKEN;
            }
            else {
                entryTable[entryTableIndex] = STRONGLY_NOT_TAKEN;
            }
            break;

        // Taken
        case WEAKLY_TAKEN:
            if (actualPath == 't') {
                entryTable[entryTableIndex] = STRONGLY_TAKEN;
            }
            else {
                (*mispredictions)++;
                entryTable[entryTableIndex] = WEAKLY_NOT_TAKEN;
            }
            break;

        case STRONGLY_TAKEN:
            if (actualPath == 't') {}
            else {
                (*mispredictions)++;
                entryTable[entryTableIndex] = WEAKLY_TAKEN;
            }
            break;

        default:
            printf("Entry table error.\n");
            break;
    }
}


double calculateMispredictionRate(FILE* inFile, int* entryTable, int M, int N) {

    char actualPath;
    long long int address, entryTableIndex;
    int globalHistoryRegister = 0, totalPredictions = 0, mispredictions = 0;

    while (!feof(inFile)) {

        fscanf(inFile, "%llx %c\n", &address, &actualPath);

        entryTableIndex = getEntryTableIndex(address, &globalHistoryRegister, M, N);
        updateEntryTable(entryTable, entryTableIndex, actualPath, &mispredictions);
        updateGHR(actualPath, &globalHistoryRegister, N);
        totalPredictions++;

        if (printVerbose) {
            printf("address=%llx, index=%llx, M=%d, N=%d\n", address, entryTableIndex, M, N);
            printf("actual_path=%c, current_prediction=%d, mispredictions=%d\n\n", actualPath, entryTable[entryTableIndex], mispredictions);
        }
    }

    return ((double)mispredictions / (double)totalPredictions);
}


int driver(int M, int N, char* traceFile) {

    FILE* inFile = fopen(traceFile, "r");
    if (inFile == NULL) {
        return -1;
    }

    FILE* outFile = fopen("out.txt", "w");

    int* entryTable = (int*)malloc(sizeof(int)*power(2, M));
    initializeEntryTable(entryTable, M);

    double mispredictionRate = calculateMispredictionRate(inFile, entryTable, M, N);

    output(outFile, traceFile, M, N, mispredictionRate);

    free(entryTable);

    fclose(inFile);
    fclose(outFile);
}


int main(int argc, char *argv[]) {

    if (argc != 4) {
        printf("Execution input error 1\n");
        return -1;
    }

    char* remaining = NULL;

    int M = strtol(argv[1], &remaining, 10);
    if (strlen(remaining) != 0 || M <= 0) { printf("Execution input error 2\n"); return -1; }

    int N = strtol(argv[2], &remaining, 10);
    if (strlen(remaining) != 0 || N <= 0) { printf("Execution input error 3\n"); return -1; }
    
    char* traceFile = (char*)malloc(100*sizeof(char));
    strcpy(traceFile, argv[3]);

    if (N > M) {
        printf("N can never be greater than M.\n");
        return -1;
    }

    return driver(M, N, traceFile);
}