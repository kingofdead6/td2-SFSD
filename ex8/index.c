#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>

#define BLOCK_SIZE 256
#define MAX_KEY_LENGTH 10
#define DELIMITER "|"
#define MAX_RECORDS (BLOCK_SIZE / sizeof(Record))

typedef struct {
    char key[MAX_KEY_LENGTH + 1];  // Include space for null terminator
    char logical_deletion;        // Logical deletion marker ('0' or '1')
    char other_fields[BLOCK_SIZE - 14];
} Record;

typedef struct {
    char data[BLOCK_SIZE];
    int free_pos;
    int record_count;
    char key1[MAX_KEY_LENGTH + 1];
    char key2[MAX_KEY_LENGTH + 1];
} Block;

typedef struct {
    int Number_of_Blocks;
    int Number_of_Records;
} Header;

typedef struct {
    FILE *file;
    Header header;
} File;

void Record_to_String(Record rec, char *recordStr) {
    snprintf(recordStr, BLOCK_SIZE, "%s%c%s", rec.key, rec.logical_deletion, rec.other_fields);
}

void readBlock(FILE *file, int blockNumber, Block *block) {
    fseek(file, sizeof(Header) + blockNumber * sizeof(Block), SEEK_SET);
    fread(block, sizeof(Block), 1, file);
}

void writeBlock(FILE *file, int blockNumber, Block *block) {
    fseek(file, sizeof(Header) + blockNumber * sizeof(Block), SEEK_SET);
    fwrite(block, sizeof(Block), 1, file);
}

void setHeader(FILE *file, Header *header) {
    fseek(file, 0, SEEK_SET);
    fwrite(header, sizeof(Header), 1, file);
}

Header getHeader(FILE *file) {
    Header header = {0, 0};
    fseek(file, 0, SEEK_SET);
    fread(&header, sizeof(Header), 1, file);
    return header;
}

File *Open(const char *filename, const char *mode) {
    FILE *file = fopen(filename, mode);
    if (!file && mode[0] == 'r') {
        file = fopen(filename, "wb+");
        if (file) {
            Header header = {0, 0};
            setHeader(file, &header);
            printf("File Created Successfully\n");
        }
        if (!file) {
            printf("File could not be created\n");
            return NULL;
        }
    }

    File *FIle = (File *)malloc(sizeof(File));
    if (!FIle) {
        fprintf(stderr, "Memory allocation failed for File structure.\n");
        fclose(file);
        return NULL;
    }

    FIle->file = file;
    FIle->header = getHeader(file);
    printf("File is open\n");
    return FIle;
}

void Close(File *file) {
    if (!file) return;
    setHeader(file->file, &file->header);
    fclose(file->file);
    free(file);
}

Block *AllocBlock(File *file) {
    if (!file) return NULL;

    Block *block = (Block *)malloc(sizeof(Block));
    if (!block) {
        fprintf(stderr, "Memory allocation failed.\n");
        return NULL;
    }

    memset(block->data, 0, BLOCK_SIZE);
    block->free_pos = 0;
    block->record_count = 0;
    memset(block->key1, '\0', MAX_KEY_LENGTH + 1);
    memset(block->key2, '\0', MAX_KEY_LENGTH + 1);

    file->header.Number_of_Blocks++;
    setHeader(file->file, &file->header);

    return block;
}

void insertRecord_TOVS(File *file, Record rec) {
    char recordStr[BLOCK_SIZE];
    Record_to_String(rec, recordStr);
    int recordLen = strlen(recordStr);

    if (recordLen + strlen(DELIMITER) > BLOCK_SIZE) {
        printf("Error: Record size exceeds block size.\n");
        return;
    }

    Block block;
    bool inserted = false;

    for (int blockNumber = 0; blockNumber < file->header.Number_of_Blocks; blockNumber++) {
        readBlock(file->file, blockNumber, &block);

        if (block.free_pos + recordLen + strlen(DELIMITER) <= BLOCK_SIZE) {
            strcat(block.data, recordStr);
            strcat(block.data, DELIMITER);
            block.free_pos += recordLen + strlen(DELIMITER);
            block.record_count++;
            writeBlock(file->file, blockNumber, &block);
            inserted = true;
            break;
        }
    }

    if (!inserted) {
        Block *newBlock = AllocBlock(file);
        strcat(newBlock->data, recordStr);
        strcat(newBlock->data, DELIMITER);
        newBlock->free_pos += recordLen + strlen(DELIMITER);
        newBlock->record_count++;
        writeBlock(file->file, file->header.Number_of_Blocks - 1, newBlock);
        free(newBlock);
    }

    file->header.Number_of_Records++;
    setHeader(file->file, &file->header);
}

void initialLoad_TOVS(File *file, int min, int max) {
    for (int i = min; i <= max; i++) {
        Record rec;
        snprintf(rec.key, sizeof(rec.key), "%010d", i); // Properly formatted key
        snprintf(rec.other_fields, sizeof(rec.other_fields), "Record number %d", i);
        rec.logical_deletion = '0';

        insertRecord_TOVS(file, rec);
    }
    printf("Initial load completed with %d records.\n", max - min + 1);
}

// Function to parse the block into individual records
void parseBlock(Block *block, Record *records, int *recordCount) {
    char *token = strtok(block->data, DELIMITER);
    *recordCount = 0;

    while (token != NULL) {
        strncpy(records[*recordCount].key, token, MAX_KEY_LENGTH);
        records[*recordCount].key[MAX_KEY_LENGTH] = '\0'; // Null terminate the key
        token += MAX_KEY_LENGTH + 1; // Skip the key part

        records[*recordCount].logical_deletion = *token++;
        strncpy(records[*recordCount].other_fields, token, BLOCK_SIZE - 14);
        records[*recordCount].other_fields[BLOCK_SIZE - 14] = '\0'; // Null terminate other fields

        (*recordCount)++;
        token = strtok(NULL, DELIMITER); // Move to next record
    }
}

// Corrected Binary Search
int binarySearch(Record *records, int recordCount, const char *key) {
    int left = 0;
    int right = recordCount - 1;

    while (left <= right) {
        int mid = left + (right - left) / 2;

        int comparison = strcmp(records[mid].key, key);

        if (comparison == 0) {
            return mid; // Key found
        }
        if (comparison < 0) {
            left = mid + 1; // Look in the right half
        } else {
            right = mid - 1; // Look in the left half
        }
    }

    return -1; // Key not found
}

void searchRecordByKey(File *file, const char *key) {
    for (int blockNumber = 0; blockNumber < file->header.Number_of_Blocks; blockNumber++) {
        Block block;
        readBlock(file->file, blockNumber, &block);

        Record records[MAX_RECORDS];
        int recordCount = 0;
        parseBlock(&block, records, &recordCount);

        int index = binarySearch(records, recordCount, key);
        if (index != -1) {
            printf("Record with key %s found in block %d at index %d.\n", key, blockNumber, index);
            return;
        }
    }

    printf("Record with key %s not found.\n", key);
}

int main() {
    File *file = Open("tovs_file.dat", "rb+");
    if (!file) {
        fprintf(stderr, "Failed to open or create the TOVS file.\n");
        return 1;
    }

    int minKey = 1;
    int maxKey = 20;
    printf("Loading records with keys from %d to %d...\n", minKey, maxKey);
    initialLoad_TOVS(file, minKey, maxKey);

    // Test binary search by searching for a record
    const char *searchKey = "0000000015";  // Example key to search
    printf("Searching for record with key: %s\n", searchKey);
    searchRecordByKey(file, searchKey);

    printf("\nFile Header Information:\n");
    printf("Total Blocks: %d\n", file->header.Number_of_Blocks);
    printf("Total Records: %d\n", file->header.Number_of_Records);

    Close(file);
    return 0;
}
