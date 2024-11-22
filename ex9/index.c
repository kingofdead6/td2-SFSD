//**Structure Definitions**

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BLOCK_SIZE 256
#define MAX_RECORDS (BLOCK_SIZE / sizeof(Record))
#define MAX_KEY_LENGTH 10
#define INFINITE_KEY "ZZZZZZZZZZ" // Used for the fictitious last record

// Record structure
typedef struct {
    char key[MAX_KEY_LENGTH];  // Key for the record
    char logical_deletion;     // '0' for active, '1' for deleted
    char data[BLOCK_SIZE - MAX_KEY_LENGTH - 1]; // Record data
} Record;

// Block structure
typedef struct {
    Record records[MAX_RECORDS];  // Records in the block
    int record_count;             // Number of valid records in the block
    int overflow_link;            // Points to the first block in the overflow zone (or -1 if none)
} Block;

// File header
typedef struct {
    int primary_blocks;           // Number of blocks in the primary zone
    int overflow_blocks;          // Number of blocks in the overflow zone
    int total_records;            // Total number of records in the file
} FileHeader;

// File structure
typedef struct {
    FILE *file;                   // File pointer
    FileHeader header;            // File metadata
} File;


//** Locate a Record**

int Locate(File *file, const char *key, int *block_idx, int *record_idx) {
    Block block;
    int left = 0, right = file->header.primary_blocks - 1;

    // Perform binary search in the primary zone
    while (left <= right) {
        int mid = (left + right) / 2;

        // Read the middle block
        fseek(file->file, sizeof(FileHeader) + mid * sizeof(Block), SEEK_SET);
        fread(&block, sizeof(Block), 1,  file->file);

        // Check key range
        if (strcmp(key, block.records[0].key) >= 0 && strcmp(key, block.records[block.record_count - 1].key) <= 0) {
            // Locate the record within the block
            for (int i = 0; i < block.record_count; i++) {
                if (strcmp(block.records[i].key, key) == 0) {
                    *block_idx = mid;
                    *record_idx = i;
                    return 1; // Record found
                }
            }
            break;
        } else if (strcmp(key, block.records[0].key) < 0) {
            right = mid - 1;
        } else {
            left = mid + 1;
        }
    }

    // Record not found
    *block_idx = left;  // Where the record should be
    *record_idx = -1;
    return 0;
}

// **Algorithm (b): Interval Query**

void List(File *file, const char *key_a, const char *key_b) {
    Block block;

    // Iterate through the primary zone
    for (int i = 0; i < file->header.primary_blocks; i++) {
        fseek(file->file, sizeof(FileHeader) + i * sizeof(Block), SEEK_SET);
        fread(&block, sizeof(Block), 1,  file->file);

        // Display records within the range
        for (int j = 0; j < block.record_count; j++) {
            if (strcmp(block.records[j].key, key_a) >= 0 && strcmp(block.records[j].key, key_b) <= 0) {
                printf("Record in Block %d, Position %d: Key = %s, Data = %s\n",
                       i, j, block.records[j].key, block.records[j].data);
            }
        }

        // Check the overflow zone for this block
        int overflow_block_idx = block.overflow_link;
        while (overflow_block_idx != -1) {
            fseek(file->file, sizeof(FileHeader) + (file->header.primary_blocks + overflow_block_idx) * sizeof(Block), SEEK_SET);
            fread(&block, sizeof(Block), 1,  file->file);

            for (int j = 0; j < block.record_count; j++) {
                if (strcmp(block.records[j].key, key_a) >= 0 && strcmp(block.records[j].key, key_b) <= 0) {
                    printf("Record in Overflow Block %d, Position %d: Key = %s, Data = %s\n",
                           overflow_block_idx, j, block.records[j].key, block.records[j].data);
                }
            }
            overflow_block_idx = block.overflow_link;
        }
    }
}

// **Algorithm (c): Reorganize the File**

void Reorganize(File *file, const char *new_name, float rate) {
    FILE *new_file = fopen(new_name, "wb+");
    if (!new_file) {
        fprintf(stderr, "Failed to create new file.\n");
        return;
    }

    FileHeader new_header = {0, 0, 0};
    fwrite(&new_header, sizeof(FileHeader), 1, new_file);

    Block block, new_block = {0};
    int fill_limit = (int)(rate * MAX_RECORDS);

    for (int i = 0; i < file->header.primary_blocks + file->header.overflow_blocks; i++) {
        fseek(file->file, sizeof(FileHeader) + i * sizeof(Block), SEEK_SET);
        fread(&block, sizeof(Block), 1,  file->file);

        // Copy logically non-deleted records to the new file
        for (int j = 0; j < block.record_count; j++) {
            if (block.records[j].logical_deletion == '0') {
                new_block.records[new_block.record_count++] = block.records[j];
                new_header.total_records++;

                // Write the block when full
                if (new_block.record_count == fill_limit) {
                    fwrite(&new_block, sizeof(Block), 1, new_file);
                    new_block = (Block){0};
                    new_header.primary_blocks++;
                }
            }
        }
    }

    // Write the last partially filled block
    if (new_block.record_count > 0) {
        fwrite(&new_block, sizeof(Block), 1, new_file);
        new_header.primary_blocks++;
    }

    // Update the header of the new file
    fseek(new_file, 0, SEEK_SET);
    fwrite(&new_header, sizeof(FileHeader), 1, new_file);

    fclose(new_file);
    printf("Reorganization complete. New file: %s\n", new_name);
}
