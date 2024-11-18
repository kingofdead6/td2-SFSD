#include <stdio.h>
#include <stdlib.h>

#define B 4 
// File structure 
typedef struct {
    FILE *file;    
    int numBlocks; 
} TOFFile;

typedef struct {
    int data[B];
} Buffer;

void Reorganize(TOFFile *F, int VP) {
    Buffer Buf1, Buf2;
    int leftBlock = 0, rightBlock = F->numBlocks - 1;
    int leftIndex, rightIndex;

    while (leftBlock <= rightBlock) {
        fseek(F->file, leftBlock * sizeof(Buffer), SEEK_SET);
        fread(&Buf1, sizeof(Buffer), 1, F->file);

        fseek(F->file, rightBlock * sizeof(Buffer), SEEK_SET);
        fread(&Buf2, sizeof(Buffer), 1, F->file);

        leftIndex = 0;
        rightIndex = B - 1;

        while (leftIndex < B && rightIndex >= 0) {
            while (leftIndex < B && Buf1.data[leftIndex] <= VP) {
                leftIndex++;
            }

            
            while (rightIndex >= 0 && Buf2.data[rightIndex] > VP) {
                rightIndex--;
            }

            if (leftIndex < B && rightIndex >= 0) {
                int temp = Buf1.data[leftIndex];
                Buf1.data[leftIndex] = Buf2.data[rightIndex];
                Buf2.data[rightIndex] = temp;

                leftIndex++;
                rightIndex--;
            }
        }

        fseek(F->file, leftBlock * sizeof(Buffer), SEEK_SET);
        fwrite(&Buf1, sizeof(Buffer), 1, F->file);

        fseek(F->file, rightBlock * sizeof(Buffer), SEEK_SET);
        fwrite(&Buf2, sizeof(Buffer), 1, F->file);

        if (leftIndex == B) {
            leftBlock++;
        }
        if (rightIndex < 0) {
            rightBlock--;
        }
    }
}

void InitializeFile(const char *filename, int *values, int numValues) {
    FILE *file = fopen(filename, "wb");
    if (!file) {
        perror("Failed to create file");
        exit(1);
    }

    Buffer Buf;
    int i, j = 0;

    while (j < numValues) {
        for (i = 0; i < B; i++) {
            if (j < numValues) {
                Buf.data[i] = values[j++];
            } else {
                Buf.data[i] = -1; 
            }
        }
        fwrite(&Buf, sizeof(Buffer), 1, file);
    }

    fclose(file);
}

void PrintFile(const char *filename, int numBlocks) {
    FILE *file = fopen(filename, "rb");
    if (!file) {
        perror("Failed to open file");
        exit(1);
    }

    Buffer Buf;
    for (int i = 0; i < numBlocks; i++) {
        fread(&Buf, sizeof(Buffer), 1, file);
        printf("Block %d: ", i);
        for (int j = 0; j < B; j++) {
            printf("%d ", Buf.data[j]);
        }
        printf("\n");
    }

    fclose(file);
}

int main() {
    const char *filename = "testfile.bin";
    int values[] = {3, 8, 1, 9, 5, 6, 10, 4, 7, 2, 11, 12};
    int numValues = sizeof(values) / sizeof(values[0]);
    int numBlocks = (numValues + B - 1) / B; 
    int VP = 7; 

    InitializeFile(filename, values, numValues);

    printf("Before reorganization:\n");
    PrintFile(filename, numBlocks);

    TOFFile F;
    F.file = fopen(filename, "rb+");
    if (!F.file) {
        perror("Failed to open file");
        exit(1);
    }
    F.numBlocks = numBlocks;

    Reorganize(&F, VP);

    fclose(F.file);

    printf("\nAfter reorganization:\n");
    PrintFile(filename, numBlocks);

    return 0;
}
