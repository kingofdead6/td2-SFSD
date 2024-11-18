/*### a) Characteristics of a Sequential File Representing a Matrix

When representing a matrix in a sequential file:
- The file contains elements stored **row by row** sequentially.
- If the matrix has \(n\) rows and \(m\) columns, the elements are stored in the order:
  \[
  m_{1,1}, m_{1,2}, \dots, m_{1,m}, m_{2,1}, \dots, m_{2,m}, \dots, m_{n,1}, \dots, m_{n,m}
  \]
- **Block Capacity**: Each block in the file can hold \(b\) integers. If \(n \cdot m\) is not a multiple of \(b\), the last block will not be fully filled.
- **File Organization**:
  - Block \(k\) contains elements from index \(k \cdot b\) to \((k+1) \cdot b - 1\) (if these indices exist).

**Declaration**:
```c
#include <stdio.h>

#define B 1024  // Block size (number of integers per block)

typedef struct {
    int data[B];  // Block of integers
} Block;

typedef struct {
    FILE *file;    // Pointer to the sequential file
    int blockSize; // Capacity of each block (usually B)
    int rows;      // Number of rows in the matrix
    int cols;      // Number of columns in the matrix
} MatrixFile;
```

### b) Efficient Algorithm to Find the Address of an Element \(m_{ij}\)

The address of \(m_{ij}\) in the sequential file is determined by its **linear index** in the file:
\[
\text{Linear Index} = i \cdot m + j
\]
where \(i\) and \(j\) are 0-based indices.

Given the linear index:
1. **Block Number**:
   \[
   \text{Block Number} = \left\lfloor \frac{\text{Linear Index}}{b} \right\rfloor
   \]
2. **Record Number within the Block**:
   \[
   \text{Record Number} = \text{Linear Index} \mod b
   \]

**Algorithm**:
```c
#include <math.h>

void find_address(int i, int j, int rows, int cols, int blockSize, int *blockNum, int *recordNum) {
    int linearIndex = i * cols + j;
    *blockNum = linearIndex / blockSize;
    *recordNum = linearIndex % blockSize;
}
```

**Explanation**:
- Compute the linear index of \(m_{ij}\) using its row and column indices.
- Use division and modulo operations to compute the block number and record number.

### c) Efficient Algorithm to Transform a Matrix from Row-wise to Column-wise Storage

To transform a matrix \(M_1(n, m)\) stored row-wise into a new matrix \(M_2(n, m)\) stored column-wise:
- Iterate through the elements of \(M_2\), which are stored column-wise.
- For each element \(m_{ij}\) in \(M_2\), read the corresponding element from \(M_1\) using:
  \[
  M_2[j \cdot n + i] = M_1[i \cdot m + j]
  \]

**Algorithm**:
```c
#include <stdio.h>
#include <stdlib.h>

void transform_matrix(MatrixFile *rowFile, MatrixFile *colFile) {
    int rows = rowFile->rows;
    int cols = rowFile->cols;
    int blockSize = rowFile->blockSize;

    Block inputBlock, outputBlock;
    int *tempMatrix = malloc(rows * cols * sizeof(int));

    // Load matrix from row-wise file into memory
    fseek(rowFile->file, 0, SEEK_SET);
    int index = 0;
    while (fread(&inputBlock, sizeof(Block), 1, rowFile->file)) {
        for (int i = 0; i < blockSize && index < rows * cols; i++) {
            tempMatrix[index++] = inputBlock.data[i];
        }
    }

    // Write the matrix to the column-wise file
    fseek(colFile->file, 0, SEEK_SET);
    index = 0;
    for (int j = 0; j < cols; j++) {
        for (int i = 0; i < rows; i++) {
            int linearIndex = i * cols + j;
            outputBlock.data[index++] = tempMatrix[linearIndex];

            // Write block to file when full
            if (index == blockSize) {
                fwrite(&outputBlock, sizeof(Block), 1, colFile->file);
                index = 0;
            }
        }
    }

    // Write any remaining data in the last block
    if (index > 0) {
        fwrite(&outputBlock, sizeof(Block), 1, colFile->file);
    }

    free(tempMatrix);
}
```

**Explanation**:
1. **Step 1**: Load the matrix from the row-wise file into a temporary memory buffer (`tempMatrix`).
   - Read blocks sequentially and store elements in a linear array.
2. **Step 2**: Write the matrix to the column-wise file.
   - For each column of the matrix, iterate over its rows.
   - Use the formula \(M_2[j \cdot n + i] = M_1[i \cdot m + j]\) to compute the new index.
   - Write the result into the file block by block.
3. Ensure any leftover elements in the last block are written to the file.

This approach minimizes file I/O by processing blocks efficiently and reduces memory usage by loading only small portions of the matrix at a time.*/