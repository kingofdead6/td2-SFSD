#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define B 10  // Block size (in bytes)

typedef struct {
    char data[B];  // Array holding raw block data (max B bytes)
    int nb;        // Number of valid records in the block
} Block;

typedef struct {
    int lastBlock;
    int firstFreePos;
    int nbRecords;
    int nbDeleted;
} Header;

typedef struct {
    Header header;
    Block blocks[];  // Flexible array of blocks
} File;

// Mock functions to simulate reading and writing blocks to the file
void ReadBlock(File *F, void *buf, int blockIndex) {
    if (blockIndex == 0) {
        // Read header block
        memcpy(buf, &F->header, sizeof(Header));
    } else {
        // Read block data
        memcpy(buf, &F->blocks[blockIndex - 1], sizeof(Block));
    }
}

void WriteBlock(File *F, void *buf, int blockIndex) {
    if (blockIndex == 0) {
        // Write to header block
        memcpy(&F->header, buf, sizeof(Header));
    } else {
        // Write to block data
        memcpy(&F->blocks[blockIndex - 1], buf, sizeof(Block));
    }
}

void CompactFile(File *F) {
    Header header;
    ReadBlock(F, &header, 0);  // Read the header block

    Block Buf1, Buf2;
    int writeIndex = 1;        // Start writing from block 1
    int buf2Pos = 0;           // Position in Buf2

    for (int i = 1; i <= header.lastBlock; i++) {
        ReadBlock(F, &Buf1, i);  // Read block i into Buf1
        int pos = 0;

        while (pos < B) {
            // Extract the deletion flag and record length
            char delFlag = Buf1.data[pos];
            if (pos + 5 >= B) break;  // Prevent overflow
            int recLen = atoi(&Buf1.data[pos + 1]);  // Parse length

            if (delFlag == '0') {  // If not deleted
                // Check if the record fits into Buf2
                if (buf2Pos + recLen + 6 > B) {
                    // Write Buf2 to the file
                    WriteBlock(F, &Buf2, writeIndex++);
                    buf2Pos = 0;  // Reset Buf2 position
                }

                // Copy the record to Buf2
                memcpy(&Buf2.data[buf2Pos], &Buf1.data[pos], recLen + 6);
                buf2Pos += recLen + 6;  // Advance Buf2 position
            }

            pos += recLen + 6;  // Advance to the next record
        }
    }

    // Write any remaining data in Buf2
    if (buf2Pos > 0) {
        WriteBlock(F, &Buf2, writeIndex++);
    }

    // Update the header
    header.lastBlock = writeIndex - 1;
    header.firstFreePos = buf2Pos;
    WriteBlock(F, &header, 0);
}
