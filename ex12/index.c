#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define B 3  // Size of each block (for simplicity)

typedef struct {
    int id;           // ID of the record
    char name[50];    // Name of the record
    float value;      // Value associated with the record
} T_rec;

typedef struct {
    int nb;           // Number of elements currently in the block
    T_rec data[B];    // Array of elements (max B per block)
} TBlock;

typedef struct {
    int head;         // Index of the block containing the first element
    int tail;         // Index of the block containing the last element
    int nElements;    // Total number of elements in the queue
    int nBlocks;      // Total number of blocks
} FileHeader;

typedef struct {
    FileHeader header;
    TBlock blocks[];  // Flexible array for blocks
} File;

void CreateQueue(File *F, int N) {
    F->header.head = 0;       // First block starts at 0
    F->header.tail = 0;       // Tail starts at the same position as head
    F->header.nElements = 0;  // Initially, the queue is empty
    F->header.nBlocks = N;    // Set the total number of blocks

    for (int i = 0; i < N; i++) {
        F->blocks[i].nb = 0;  // All blocks are initially empty
    }
}

int NbElement(File *F) {
    return F->header.nElements;
}

void EnqueueGroup(File *F, int n, T_rec T[]) {
    int index = 0; // Start at the first element of T

    while (n > 0) {
        TBlock *tailBlock = &F->blocks[F->header.tail];
        int space = B - tailBlock->nb; // Available space in the current tail block

        // Fill the current block or add as many as possible
        int toAdd = (n < space) ? n : space;
        for (int i = 0; i < toAdd; i++) {
            tailBlock->data[tailBlock->nb++] = T[index++];
        }

        n -= toAdd;
        F->header.nElements += toAdd;

        // Move to the next block if the current one is full
        if (tailBlock->nb == B) {
            F->header.tail = (F->header.tail + 1) % F->header.nBlocks;
            if (F->header.tail == F->header.head) {
                printf("Queue overflow: No space left!\n");
                return;
            }
        }
    }
}

void DequeueGroup(File *F, int n, T_rec T[]) {
    if (F->header.nElements < n) {
        printf("Queue underflow: Not enough elements to dequeue!\n");
        return;
    }

    int index = 0; // Start filling T from the first position

    while (n > 0) {
        TBlock *headBlock = &F->blocks[F->header.head];
        int toRemove = (n < headBlock->nb) ? n : headBlock->nb;

        // Remove elements from the current block
        for (int i = 0; i < toRemove; i++) {
            T[index++] = headBlock->data[i];
        }

        // Shift remaining elements in the block
        for (int i = toRemove; i < headBlock->nb; i++) {
            headBlock->data[i - toRemove] = headBlock->data[i];
        }
        headBlock->nb -= toRemove;
        n -= toRemove;
        F->header.nElements -= toRemove;

        // Move to the next block if the current one is empty
        if (headBlock->nb == 0) {
            F->header.head = (F->header.head + 1) % F->header.nBlocks;
        }
    }
}

// Function to print the records in a group
void PrintRecords(T_rec *T, int n) {
    for (int i = 0; i < n; i++) {
        printf("ID: %d, Name: %s, Value: %.2f\n", T[i].id, T[i].name, T[i].value);
    }
}

int main() {
    // Create a file with a total of 3 blocks
    File *F = malloc(sizeof(File) + 3 * sizeof(TBlock)); // Allocate memory for 3 blocks
    CreateQueue(F, 3);

    // Enqueue group of records
    T_rec group1[] = {
        {1, "First", 10.0},
        {2, "Second", 20.0},
        {3, "Third", 30.0}
    };
    EnqueueGroup(F, 3, group1);

    T_rec group2[] = {
        {4, "Fourth", 40.0},
        {5, "Fifth", 50.0}
    };
    EnqueueGroup(F, 2, group2);

    // Print the queue elements after enqueuing
    printf("Queue after enqueuing:\n");
    printf("Total elements in queue: %d\n", NbElement(F));

    // Dequeue a group of records
    T_rec dequeued[3];
    DequeueGroup(F, 3, dequeued);

    // Print the dequeued records
    printf("\nDequeued records:\n");
    PrintRecords(dequeued, 3);

    // Print the queue status after dequeuing
    printf("\nQueue after dequeuing:\n");
    printf("Total elements in queue: %d\n", NbElement(F));

    // Clean up
    free(F);

    return 0;
}
