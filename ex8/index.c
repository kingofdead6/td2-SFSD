#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define MAX_BLOCK_SIZE 4096  // Size of the block (in bytes)
#define MAX_RECORD_SIZE 1024 // Max size for each record (in bytes)
#define MAX_KEY_SIZE 10      // Fixed size for the key (10 characters)
#define MAX_BLOCKS 1000      // Max number of blocks in the file
#define MAX_RECORDS 50       // Max records per block

// Define the structure for a block in the file
typedef struct {
    char tab[MAX_BLOCK_SIZE];  // Array of characters containing the records
    int free_pos;              // Last free position in tab
    char key1[MAX_KEY_SIZE];   // Smallest key in the block
    char key2[MAX_KEY_SIZE];   // Largest key in the block
} Tblock;

// Function to read a block from the file
void read_block(FILE *file, int block_num, Tblock *block) {
    fseek(file, block_num * sizeof(Tblock), SEEK_SET);
    fread(block, sizeof(Tblock), 1, file);
}

// Function to extract the key from a record (first 10 characters are the key)
void extract_key(const char *record, char *key) {
    strncpy(key, record + 3, MAX_KEY_SIZE);  // Skip the first 3 characters (size)
    key[MAX_KEY_SIZE] = '\0';  // Null-terminate the key
}

// Linear search inside a block to find a record by key
// Linear search inside a block to find a record by key
int search_within_block(const Tblock *block, const char *key) {
    int record_pos = 0;

    // Iterate over the records in the block
    while (record_pos < block->free_pos) {
        // Extract the size of the record (first 3 characters)
        int record_size = (block->tab[record_pos] << 16) + (block->tab[record_pos + 1] << 8) + block->tab[record_pos + 2];

        // Extract the logical deletion character (it's just after the size)
        char logical_deletion = block->tab[record_pos + 3];

        // Skip if the record is logically deleted (assuming 0 means not deleted)
        if (logical_deletion != 0) {
            record_pos += record_size;  // Skip this record
            continue;
        }

        // Extract the key from the record (key starts after size (3 bytes) and deletion byte (1 byte))
        char record_key[MAX_KEY_SIZE + 1];  // +1 for null terminator
        strncpy(record_key, block->tab + record_pos + 4, MAX_KEY_SIZE);
        record_key[MAX_KEY_SIZE] = '\0';  // Null-terminate the key

        // Debugging: print the key being compared
        printf("Checking record key: '%s'\n", record_key);

        // Compare the extracted key with the search key
        if (strcmp(record_key, key) == 0) {
            return record_pos; // Found the record
        }

        // Move to the next record by advancing the record position by the full record size
        record_pos += record_size;
    }

    return -1;  // Not found
}


// Binary search on the blocks of the file
int binary_search(FILE *file, int total_blocks, const char *key) {
    int left = 0, right = total_blocks - 1;
    Tblock block;

    while (left <= right) {
        int mid = left + (right - left) / 2;

        // Read the middle block
        read_block(file, mid, &block);

        // Debugging: print the keys for the middle block
        printf("Searching block %d: Key1 = %s, Key2 = %s\n", mid, block.key1, block.key2);

        // Compare with the smallest and largest keys in the block
        int cmp_start = strcmp(key, block.key1);
        int cmp_end = strcmp(key, block.key2);

        if (cmp_start < 0) {
            right = mid - 1;  // Search in the left half
        } else if (cmp_end > 0) {
            left = mid + 1;   // Search in the right half
        } else {
            // Key lies between key1 and key2 in this block, perform a linear search within the block
            return search_within_block(&block, key);
        }
    }

    return -1;  // Key not found in the file
}

// Function to simulate the creation of the TOV file (just for demonstration)
void create_sample_file(const char *filename) {
    FILE *file = fopen(filename, "wb");
    if (!file) {
        perror("Failed to open file");
        exit(1);
    }

    Tblock block;
    // Example records (key + other fields)
    const char *keys[] = {"apple", "banana", "cherry", "date", "fig", "grape"};
    int total_records = sizeof(keys) / sizeof(keys[0]);

    // Create two blocks with some records
    for (int i = 0; i < 2; i++) {
        block.free_pos = 0;
        
        // Set the smallest and largest keys in the block
        strncpy(block.key1, keys[0], MAX_KEY_SIZE); // Smallest key in the block
        strncpy(block.key2, keys[total_records - 1], MAX_KEY_SIZE); // Largest key in the block

        for (int j = 0; j < total_records; j++) {
            // Record size (first 3 characters)
            int record_size = strlen(keys[j]) + 3;
            block.tab[block.free_pos] = (record_size >> 16) & 0xFF;
            block.tab[block.free_pos + 1] = (record_size >> 8) & 0xFF;
            block.tab[block.free_pos + 2] = record_size & 0xFF;

            // Logical deletion character (assume 0 means not deleted)
            block.tab[block.free_pos + 3] = 0;

            // Copy the key
            strncpy(block.tab + block.free_pos + 4, keys[j], MAX_KEY_SIZE);

            block.free_pos += record_size;
        }

        fwrite(&block, sizeof(Tblock), 1, file);
    }

    fclose(file);
}

// Function to print the file content (for debugging purposes)
void print_file(const char *filename, int total_blocks) {
    FILE *file = fopen(filename, "rb");
    if (!file) {
        perror("Failed to open file");
        exit(1);
    }

    Tblock block;
    for (int i = 0; i < total_blocks; i++) {
        fread(&block, sizeof(Tblock), 1, file);
        printf("Block %d:\n", i);
        printf("  Key1: %s, Key2: %s\n", block.key1, block.key2);
        int record_pos = 0;
        while (record_pos < block.free_pos) {
            // Extract the record size (first 3 characters)
            int record_size = (block.tab[record_pos] << 16) + (block.tab[record_pos + 1] << 8) + block.tab[record_pos + 2];
            printf("  Record (size %d): ", record_size);
            for (int j = 0; j < record_size - 3; j++) {
                printf("%c", block.tab[record_pos + 3 + j]);
            }
            printf("\n");

            // Move to the next record
            record_pos += record_size;
        }
    }

    fclose(file);
}

// Main function to test the binary search algorithm
int main() {
    const char *filename = "tovfile.bin";
    create_sample_file(filename);

    FILE *file = fopen(filename, "rb");
    if (!file) {
        perror("Failed to open file");
        exit(1);
    }

    const char *key_to_search = "banana";  // The key to search for
    int total_blocks = 2;  // We know we have 2 blocks in the test file

    printf("Searching for key '%s'...\n", key_to_search);
    int result = binary_search(file, total_blocks, key_to_search);

    if (result != -1) {
        printf("Record found at position %d\n", result);
    } else {
        printf("Record not found.\n");
    }

    fclose(file);

    // Print the file contents for debugging purposes
    print_file(filename, total_blocks);

    return 0;
}
