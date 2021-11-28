// Joey Hodson
// EEL 4768 - Computer Architecture
// Project 2 - Branch Predictor
// 11.30.2021

// Notes: 
// 1) M = entry table # of bits
//    N = global history register # of bits

/* README:

    command line:

    /sim gshare <GPB> <RB> <Trace_File>
    1) GPB (M) = The number of PC bits used to index the gshare table.
    2) RB (N) = The global history register bits used to index the gshare table.
    3) Trace_File = The trace file name along with its extension
    
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>


#define WEAKLY_NOT_TAKEN 1
#define STRONGLY_NOT_TAKEN 0
#define WEAKLY_TAKEN 2
#define STRONGLY_TAKEN 3

#define BYTE_OFFSET_BIT_COUNT 2

int printVerbose = 1;


void initializeEntryTable(int* entryTable, int M) {

    for (int i = 0; i < pow(2, M); i++) {
        entryTable[i] = WEAKLY_TAKEN;
    }
}


void output(FILE* outFile, int M, int N, double* mispredictionRate) {

    fprintf(outFile, "Configuration: M = %d, N = %d\n", M, N);
    fprintf(outFile, "Misprediction Rate: %.2f\n", *mispredictionRate);
}


long long int getEntryTableIndex(long long int address, int* globalHistoryRegister, int M, int N) {

    unsigned int mask = (1 << M) - 1;
    long long int valueOfMBitField = (address >> BYTE_OFFSET_BIT_COUNT) & mask;
    long long int shiftedGHR = (*globalHistoryRegister << (M - N));

    long long int index = (valueOfMBitField ^ shiftedGHR);
    
    return index;
}

double simulate(FILE* inFile, int* entryTable, int M, int N, double* mispredictionRate) {

    char actualPath;
    long long int address, entryTableIndex;
    int globalHistoryRegister;

    while (!feof(inFile)) {

        fscanf(inFile, "%llx %c\n", &address, &actualPath);

        entryTableIndex = getEntryTableIndex(address, &globalHistoryRegister, M, N);

        if (printVerbose) {
            printf("address=%llx, index=%llx, actual=%c\n", address, entryTableIndex, actualPath, M, N);
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
}


int driver(int M, int N, char* traceFile) {

    FILE* inFile = fopen(traceFile, "r");
    if (inFile == NULL) {
        return -1;
    }

    FILE* outFile = fopen("out.txt", "w");

    int* entryTable = (int*)malloc(sizeof(int)*pow(2, M));

    initializeEntryTable(entryTable, M);

    double mispredictionRate = simulate(inFile, entryTable, M, N);

    output(outFile, M, N, mispredictionRate);
    free(entryTable);

    fclose(inFile);
    fclose(outFile);
}


int main(int argc, char *argv[]) {

    if (argc != 4) {
        return -1;
    }

    // M = entry table # of bits
    // N = global history register # of bits
    // /sim gshare <GPB> <RB> <Trace_File>

    // ./main.out 32768 4 0 1 trace_files/minife.t 

    char* remaining = NULL;

    int M = strtol(argv[1], &remaining, 10);
    if (strlen(remaining) != 0 || M <= 0) { return -1; }

    int N = strtol(argv[2], &remaining, 10);
    if (strlen(remaining) != 0 || N <= 0) { return -1; }
    
    char* traceFile = (char*)malloc(100*sizeof(char));
    strcpy(traceFile, argv[3]);

    return driver(M, N, traceFile);
}