#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#define BLOCK_SIZE 3
#define FILE_NAME "data_file.dat"
#define INITIAL_CAPACITY 1

typedef struct {
    int key;
    char data[100];
    bool erased;
} Record;

typedef struct {
    int RecordCount;
    Record record[BLOCK_SIZE];
} Block;

void insertRecord(FILE *file, int key, const char *data) {
    Block block;
    int blockNumber = 0;
    int totalRecords = 0;
    int capacity = INITIAL_CAPACITY;
    Record *allRecords = malloc(capacity * sizeof(Record));

    if (!allRecords) {
        printf("Memory allocation failed!\n");
        return;
    }

    while (ReadBlock(file, blockNumber, &block)) {
        for (int i = 0; i < block.RecordCount; i++) {
            if (totalRecords >= capacity) {
                capacity *= 2;
                allRecords = realloc(allRecords, capacity * sizeof(Record));
                if (!allRecords) {
                    printf("Memory reallocation failed!\n");
                    return;
                }
            }
            allRecords[totalRecords++] = block.record[i];
        }
        blockNumber++;
    }

    Record newRecord;
    newRecord.key = key;
    strcpy(newRecord.data, data);
    if (totalRecords >= capacity) {
        capacity *= 2;
        allRecords = realloc(allRecords, capacity * sizeof(Record));
        if (!allRecords) {
            printf("Memory reallocation failed!\n");
            return;
        }
    }
    allRecords[totalRecords++] = newRecord;

    for (int i = 0; i < totalRecords - 1; i++) {
        for (int j = i + 1; j < totalRecords; j++) {
            if (allRecords[i].key > allRecords[j].key) {
                Record temp = allRecords[i];
                allRecords[i] = allRecords[j];
                allRecords[j] = temp;
            }
        }
    }

    blockNumber = 0;
    int recordIndex = 0;

    while (recordIndex < totalRecords) {
        ReadBlock(file, blockNumber, &block);

        int i = 0;
        while (i < BLOCK_SIZE && recordIndex < totalRecords) {
            block.record[i] = allRecords[recordIndex];
            recordIndex++;
            i++;
        }
        block.RecordCount = i;

        WriteBlock(file, blockNumber, &block);
        blockNumber++;
    }

    free(allRecords);

    printf("Record inserted and sorted successfully: Key = %d, Data = %s\n", key, data);
}

int ReadBlock(FILE *file, int blockNumber, Block *block) {
    fseek(file, blockNumber * sizeof(Block), SEEK_SET);
    size_t bytesRead = fread(block, sizeof(Block), 1, file);
    return bytesRead == 1;
}

void delete_logic(FILE *file, int key) {
    Block block, nextBlock;
    int BlockNumber = 0;
    bool found = false;

    while (ReadBlock(file, BlockNumber, &block)) {
        for (int i = 0; i < block.RecordCount; i++) {
            if (block.record[i].key == key && !block.record[i].erased) {
                block.record[i].erased = true;

                for (int j = i; j < block.RecordCount - 1; j++) {
                    block.record[j] = block.record[j + 1];
                }
                block.RecordCount--;
                WriteBlock(file, BlockNumber, &block);
                found = true;
                break;
            }
        }
        if (found) break;
        BlockNumber++;
    }

    if (!found) {
        printf("Record with key = %d was not found or already erased.\n", key);
    }

    int currentBlock = 0;
    while (ReadBlock(file, currentBlock, &block)) {
        if (block.RecordCount < BLOCK_SIZE) {
            int nextBlockNumber = currentBlock + 1;
            while (ReadBlock(file, nextBlockNumber, &nextBlock)) {
                while (nextBlock.RecordCount > 0 && block.RecordCount < BLOCK_SIZE) {
                    block.record[block.RecordCount] = nextBlock.record[0];
                    block.RecordCount++;

                    for (int k = 0; k < nextBlock.RecordCount - 1; k++) {
                        nextBlock.record[k] = nextBlock.record[k + 1];
                    }
                    nextBlock.RecordCount--;

                    WriteBlock(file, nextBlockNumber, &nextBlock);
                }
                if (block.RecordCount == BLOCK_SIZE) break;

                nextBlockNumber++;
            }
            WriteBlock(file, currentBlock, &block);
        }
        currentBlock++;
    }
}

void WriteBlock(FILE *file, int blockNumber, const Block *block) {
    fseek(file, blockNumber * sizeof(Block), SEEK_SET);
    fwrite(block, sizeof(Block), 1, file);
    fflush(file);
}

void display_File(FILE *file) {
    fseek(file, 0, SEEK_SET);
    Block block;
    int blockNumber = 0;

    while (ReadBlock(file, blockNumber, &block)) {
        printf("Block %d:\n", blockNumber);
        for (int i = 0; i < block.RecordCount; i++) {
            printf("  Record %d -> Key: %d, Data: %s\n", i, block.record[i].key, block.record[i].data);
        }
        blockNumber++;
    }
}

int main() {
    FILE *file = fopen(FILE_NAME, "rb+");
    if (!file) {
        file = fopen(FILE_NAME, "wb+");
        printf("Created a new file named: %s\n", FILE_NAME);
    }

    char name[100];
    for (int i = 5; i < 12; i++) {
        snprintf(name, sizeof(name), "Record %d", i);
        insertRecord(file, i, name);
    }
    insertRecord(file, 2, "Record 2");

    printf("\nFile contents after insertion:\n");
    display_File(file);
    delete_logic(file , 7);
    display_File(file);
    fclose(file);
    return 0;
}


/*Worst-Case Complexity:
Reading each block: O(nblk), where nblk is the number of blocks in the file.
Shifting records within a block: In the worst case, you may have to move all b records in each block, making the complexity O(b) for each block.
Shifting records between blocks: In the worst case, you may need to transfer records from each block to the next, making it O(b) for each transfer.
Thus, in the worst case, the time complexity for this operation is O(nblk * b), where nblk is the number of blocks and b is the maximum number of records per block.

Average-Case Complexity:
On average, assuming the records are distributed fairly evenly between blocks and the number of logically deleted records (nbDel) is not excessively high, the average number of shifts and transfers will be less than in the worst case. Therefore, the average case complexity will still be O(nblk * b), but with a lower constant factor due to fewer record deletions.
Space Complexity:
You only need two buffers (one for the current block and one for the next), so the space complexity is O(b), where b is the size of a block (number of records).
*/