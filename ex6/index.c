#include <stdio.h>
#include <stdlib.h>

#define B 4 // Block size 

typedef struct {
    FILE *file;       
    int lastBlockNum; 
} TOFFile;

typedef struct {
    int data[B]; 
} Buffer;

void Delete(TOFFile *F, int i, int j) {
    Buffer Buf, LastBuf;
    int lastBlock = F->lastBlockNum;
    int lastRecordIndex;

    fseek(F->file, lastBlock * sizeof(Buffer), SEEK_SET);
    fread(&LastBuf, sizeof(Buffer), 1, F->file);
    lastRecordIndex = B - 1;
    while (lastRecordIndex >= 0 && LastBuf.data[lastRecordIndex] == -1) { // Assume -1 indicates empty
        lastRecordIndex--;
    }

    if (lastRecordIndex < 0) {
        fprintf(stderr, "File is empty, nothing to delete.\n");
        return;
    }

    fseek(F->file, i * sizeof(Buffer), SEEK_SET);
    fread(&Buf, sizeof(Buffer), 1, F->file);

    Buf.data[j] = LastBuf.data[lastRecordIndex];

    fseek(F->file, i * sizeof(Buffer), SEEK_SET);
    fwrite(&Buf, sizeof(Buffer), 1, F->file);

    LastBuf.data[lastRecordIndex] = -1; // Mark as empty
    fseek(F->file, lastBlock * sizeof(Buffer), SEEK_SET);
    fwrite(&LastBuf, sizeof(Buffer), 1, F->file);

    if (lastRecordIndex == 0) {
        F->lastBlockNum--;
    }
}

void initializeFile(const char *filename, int numBlocks) {
    FILE *file = fopen(filename, "wb");
    Buffer Buf;

    int value = 1;
    for (int block = 0; block < numBlocks; block++) {
        for (int i = 0; i < B; i++) {
            Buf.data[i] = value++;
        }
        fwrite(&Buf, sizeof(Buffer), 1, file);
    }

    fclose(file);
}

void printFile(const char *filename, int numBlocks) {
    FILE *file = fopen(filename, "rb");
    Buffer Buf;

    printf("Contents of the file:\n");
    for (int block = 0; block < numBlocks; block++) {
        fread(&Buf, sizeof(Buffer), 1, file);
        printf("Block %d: ", block);
        for (int i = 0; i < B; i++) {
            printf("%d ", Buf.data[i]);
        }
        printf("\n");
    }

    fclose(file);
}

int main() {
    const char *filename = "matrix.tof";
    int numBlocks = 3;

    initializeFile(filename, numBlocks);

    TOFFile tofFile;
    tofFile.file = fopen(filename, "r+b");
    tofFile.lastBlockNum = numBlocks - 1;

    printf("Before deletion:\n");
    printFile(filename, numBlocks);

    printf("\nDeleting record at block 1, position 2...\n");
    Delete(&tofFile, 1, 1);

    printf("After deletion:\n");
    printFile(filename, numBlocks);

    fclose(tofFile.file);
    return 0;
}

/*Cost Analysis
Best-Case Scenario
Access Cost:
Reading block 
𝑖
i: 1 block read.
Writing block 
𝑖
i: 1 block write.
No need to update the last block or adjust the block count if 
𝑖
=
lastBlock
i=lastBlock and 
𝑗
j is the last record.
Total Cost: 1 block read + 1 block write.
Worst-Case Scenario
Access Cost:
Reading block 
𝑖
i: 1 block read.
Reading the last block: 1 block read.
Writing block 
𝑖
i: 1 block write.
Writing the last block: 1 block write.
If the last block is empty after deletion, decrementing the block count involves no I/O cost.
Total Cost: 2 block reads + 2 block writes.
*/