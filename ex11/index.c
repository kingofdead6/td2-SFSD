#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#define B 10  // Size of each block

typedef struct {
    int id;           
    char name[50];    
    float value;
} T_rec;

typedef struct {
    int nb;         
    T_rec data[B];      
    int next;            
} TBlock;

typedef struct {
    int head;       
    int tail;      
    int free;            
    int nBlocks;         
} FileHeader;

typedef struct {
    FILE *file;
    FileHeader header;
} File;

void CreateQueue(File *F) {
    F->header.head = -1;  
    F->header.tail = -1;  
    F->header.free = -1;  
    F->header.nBlocks = 0;
}

bool IsQueueEmpty(File *F) {
    return F->header.head == -1;
}

void ReadBlock(File *F, TBlock Buf, int blockIndex) {
    fseek(F->file, blockIndex * sizeof(TBlock), SEEK_SET);
    fread(&Buf, sizeof(TBlock), 1, F->file);
}

void WriteBlock(File *F, TBlock Buf, int blockIndex) {
    fseek(F->file, blockIndex * sizeof(TBlock), SEEK_SET);
    fwrite(&Buf, sizeof(TBlock), 1, F->file);
}

void Enqueue(File *F, T_rec e) {
    TBlock Buf1;

    int newBlock = F->header.free;
    if (newBlock != -1) {
        ReadBlock(F, Buf1, newBlock);
        F->header.free = Buf1.next;
    } else {
        newBlock = F->header.nBlocks++;
    }

    if (F->header.head == -1) {
        F->header.head = newBlock;
        F->header.tail = newBlock;
        Buf1.nb = 0;
        Buf1.next = -1;
    } else {
        TBlock BufTail;
        ReadBlock(F, BufTail, F->header.tail);
        if (BufTail.nb < B) {
            BufTail.data[BufTail.nb++] = e;
            WriteBlock(F, BufTail, F->header.tail);
            return;
        }
        BufTail.next = newBlock;
        WriteBlock(F, BufTail, F->header.tail);
        F->header.tail = newBlock;
        Buf1.nb = 0;
        Buf1.next = -1;
    }

    Buf1.data[Buf1.nb++] = e;
    WriteBlock(F, Buf1, newBlock);
}

void Dequeue(File *F, T_rec *e) {
    if (IsQueueEmpty(F)) {
        printf("Queue is empty\n");
        return;
    }

    TBlock BufHead;
    ReadBlock(F, BufHead, F->header.head);

    *e = BufHead.data[0];

    for (int i = 1; i < BufHead.nb; i++) {
        BufHead.data[i - 1] = BufHead.data[i];
    }
    BufHead.nb--;

    if (BufHead.nb == 0) {
        int oldHead = F->header.head;
        F->header.head = BufHead.next;
        BufHead.next = F->header.free;
        F->header.free = oldHead;
        WriteBlock(F, BufHead, oldHead);
    } else {
        WriteBlock(F, BufHead, F->header.head);
    }

    if (F->header.head == -1) {
        F->header.tail = -1;
    }
}

int main() {
    // Example of how to use the Queue
    File F;
    CreateQueue(&F);

    // Open the file
    F.file = fopen("queue.dat", "wb+");
    if (!F.file) {
        printf("Error opening file.\n");
        return 1;
    }

    // Enqueue some data
    T_rec e1 = {1, "First", 10.0};
    Enqueue(&F, e1);

    T_rec e2 = {2, "Second", 20.0};
    Enqueue(&F, e2);

    // Dequeue some data
    T_rec dequeued;
    Dequeue(&F, &dequeued);
    printf("Dequeued: %d, %s, %.2f\n", dequeued.id, dequeued.name, dequeued.value);

    fclose(F.file);
    return 0;
}
