#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define B 10  // Max number of records per block

// Define a structure for a record
typedef struct {
    int key;
    char data[20];  // Example data, you can add more fields as needed
} T_rec;

// Define a block structure that holds an array of records
typedef struct {
    int nb;           // Number of records in the block
    T_rec data[B];    // Array of records (max B records per block)
} TBlock;

// Define a file structure containing blocks
typedef struct {
    int lastBlock;  // Last block index
    TBlock blocks[];  // Flexible array of blocks
} File;

// Define a function for reading a block (just a placeholder for demonstration)
void ReadBlock(File *F, TBlock *buf, int blockIndex) {
    if (blockIndex <= F->lastBlock) {
        *buf = F->blocks[blockIndex];
    }
}

// Define a function for writing a block (just a placeholder for demonstration)
void WriteBlock(File *F, TBlock *buf, int blockIndex) {
    if (blockIndex <= F->lastBlock) {
        F->blocks[blockIndex] = *buf;
    }
}

// Fragment the file into 3 separate files (F1, F2, F3) based on key ranges
void FragmentFile(File *F, File *F1, File *F2, File *F3, int C1, int C2) {
    // Buffers for F1, F2, F3
    TBlock buf1, buf2, buf3;
    int idx1 = 0, idx2 = 0, idx3 = 0;  // Indices for each buffer

    // Initialize buffers
    buf1.nb = buf2.nb = buf3.nb = 0;

    // Iterate through all records in the original file
    for (int i = 0; i <= F->lastBlock; i++) {
        TBlock buf;
        ReadBlock(F, &buf, i);
        for (int j = 0; j < buf.nb; j++) {
            T_rec record = buf.data[j];

            // Determine which file to put the record in based on key ranges
            if (record.key < C1) {
                // Insert into F1
                if (buf1.nb < B) {
                    buf1.data[buf1.nb++] = record;
                } else {
                    // Write to F1 and reset buffer
                    WriteBlock(F1, &buf1, idx1++);
                    buf1.data[0] = record;
                    buf1.nb = 1;
                }
            } else if (record.key >= C1 && record.key < C2) {
                // Insert into F2
                if (buf2.nb < B) {
                    buf2.data[buf2.nb++] = record;
                } else {
                    // Write to F2 and reset buffer
                    WriteBlock(F2, &buf2, idx2++);
                    buf2.data[0] = record;
                    buf2.nb = 1;
                }
            } else {
                // Insert into F3
                if (buf3.nb < B) {
                    buf3.data[buf3.nb++] = record;
                } else {
                    // Write to F3 and reset buffer
                    WriteBlock(F3, &buf3, idx3++);
                    buf3.data[0] = record;
                    buf3.nb = 1;
                }
            }
        }
    }

    // Write any remaining records in buffers to the corresponding files
    if (buf1.nb > 0) {
        WriteBlock(F1, &buf1, idx1++);
    }
    if (buf2.nb > 0) {
        WriteBlock(F2, &buf2, idx2++);
    }
    if (buf3.nb > 0) {
        WriteBlock(F3, &buf3, idx3++);
    }
}

