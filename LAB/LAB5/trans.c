/* 20190650 Gwanho Kim */

/*
 * trans.c - Matrix transpose B = A^T
 *
 * Each transpose function must have a prototype of the form:
 * void trans(int M, int N, int A[N][M], int B[M][N]);
 *
 * A transpose function is evaluated by counting the number of misses
 * on a 1KB direct mapped cache with a block size of 32 bytes.
 */
#include "cachelab.h"
#include <stdio.h>

int is_transpose(int M, int N, int A[N][M], int B[M][N]);
void simple_trans(int M, int N, int A[N][M], int B[M][N], int block_size);
void complex_trans(int M, int N, int A[N][M], int B[M][N]);
void complex_trans_normal(int M, int N, int A[N][M], int B[M][N]);

void complex_trans(int M, int N, int A[N][M], int B[M][N]) {
    int tmp[8];
    for (int i = 0; i < N; i += 8) {
        for (int j = 0; j < M; j += 8) {
            // Perspective of spatial locality of A
            for (int k = i; k < i + 4; k++) {
                // Load kth r of A into tmp
                tmp[0] = A[k][j];
                tmp[1] = A[k][j + 1];
                tmp[2] = A[k][j + 2];
                tmp[3] = A[k][j + 3];
                tmp[4] = A[k][j + 4];
                tmp[5] = A[k][j + 5];
                tmp[6] = A[k][j + 6];
                tmp[7] = A[k][j + 7];
                // Complete Left-Upper part of block
                B[j][k] = tmp[0];
                B[j + 1][k] = tmp[1];
                B[j + 2][k] = tmp[2];
                B[j + 3][k] = tmp[3];
                // Use this Right-Upper part of block as "mocked cache"
                // Note that these are using temporal locality
                B[j][k + 4] = tmp[4];
                B[j + 1][k + 4] = tmp[5];
                B[j + 2][k + 4] = tmp[6];
                B[j + 3][k + 4] = tmp[7];
            }
            // Perspective of spatial locality of B
            for (int k = j; k < j + 4; k++) {
                // Load kth column of A into tmp
                tmp[0] = A[i + 4][k];
                tmp[1] = A[i + 5][k];
                tmp[2] = A[i + 6][k];
                tmp[3] = A[i + 7][k];
                // Load "mocked cache" of Right-Upper part of block before
                // overwrite Note that load r, not column of Right-Upper
                // part of block Because utilize spatial locality
                tmp[4] = B[k][i + 4];
                tmp[5] = B[k][i + 5];
                tmp[6] = B[k][i + 6];
                tmp[7] = B[k][i + 7];
                // Complete Right-Upper part of block
                B[k][i + 4] = tmp[0];
                B[k][i + 5] = tmp[1];
                B[k][i + 6] = tmp[2];
                B[k][i + 7] = tmp[3];
                // Complete Left-Lower part of block using "mocked cache"
                // Note that set index is same as before(=k), temporal locality
                B[k + 4][i] = tmp[4];
                B[k + 4][i + 1] = tmp[5];
                B[k + 4][i + 2] = tmp[6];
                B[k + 4][i + 3] = tmp[7];
            }
            // Complete Right-Lower part of block
            int tmp;
            for (int r = i + 4; r < i + 8; r++) {
                for (int c = j + 4; c < j + 8; c++) {
                    if (r == c) {
                        tmp = A[r][c];
                    } else {
                        B[c][r] = A[r][c];
                    }
                }
                if (i == j) {
                    B[r][r] = tmp;
                }
            }
        }
    }
}

void complex_trans_normal(int M, int N, int A[N][M], int B[M][N]) {
    // This function works but not optimized enough
    // Its miss count is 1,668 which is less than 2,000 but more than 1,300
    int tmp1, tmp2, tmp3, tmp4;
    int tmp5, tmp6, tmp7, tmp8;
    for (int i = 0; i < N; i += 8) {
        for (int j = 0; j < M; j += 8) {
            // Load one row of A into tmp
            tmp1 = A[i][j + 4];
            tmp2 = A[i][j + 5];
            tmp3 = A[i][j + 6];
            tmp4 = A[i][j + 7];
            for (int r = i; r < i + 4; r++) {
                for (int c = j; c < j + 4; c++) {
                    B[c][r] = A[r][c];
                }
            }
            // Load one row of A into tmp
            tmp5 = A[i + 4][j + 4];
            tmp6 = A[i + 4][j + 5];
            tmp7 = A[i + 4][j + 6];
            tmp8 = A[i + 4][j + 7];
            for (int r = i + 4; r < i + 8; r++) {
                for (int c = j; c < j + 4; c++) {
                    B[c][r] = A[r][c];
                }
            }
            // Start at r = i + 1, not r = i
            for (int r = i + 1; r < i + 4; r++) {
                for (int c = j + 4; c < j + 8; c++) {
                    B[c][r] = A[r][c];
                }
            }
            // Store tmp into B using temporal locality of B and cached data
            B[j + 4][i] = tmp1;
            B[j + 5][i] = tmp2;
            B[j + 6][i] = tmp3;
            B[j + 7][i] = tmp4;
            // Start at r = i + 5, not r = i + 4
            for (int r = i + 5; r < i + 8; r++) {
                for (int c = j + 4; c < j + 8; c++) {
                    B[c][r] = A[r][c];
                }
            }
            // Store tmp into B using temporal locality of B and cached data
            B[j + 4][i + 4] = tmp5;
            B[j + 5][i + 4] = tmp6;
            B[j + 6][i + 4] = tmp7;
            B[j + 7][i + 4] = tmp8;
        }
    }
}

void simple_trans(int M, int N, int A[N][M], int B[M][N], int block_size) {
    int tmp;
    for (int i = 0; i < N; i += block_size) {
        for (int j = 0; j < M; j += block_size) {
            for (int r = i; r < i + block_size && r < N; r++) {
                for (int c = j; c < j + block_size && c < M; c++) {
                    if (r != c) {
                        B[c][r] = A[r][c];
                    } else {
                        tmp = A[r][c];
                    }
                }
                if (i == j) {
                    B[r][r] = tmp;
                }
            }
        }
    }
}
/*
 * transpose_submit - This is the solution transpose function that you
 *     will be graded on for Part B of the assignment. Do not change
 *     the description string "Transpose submission", as the driver
 *     searches for that string to identify the transpose function to
 *     be graded.
 */
char transpose_submit_desc[] = "Transpose submission";
void transpose_submit(int M, int N, int A[N][M], int B[M][N]) {
    // Case 64X64
    if (M == 64) {
        // Miss count: 1,179
        complex_trans(M, N, A, B); // Better than later one
        // complex_trans_normal(M, N, A, B);
    } else {
        // Case 32X32
        if (M == 32) {
            // Miss count: 287
            simple_trans(M, N, A, B, 8);
        }
        // Case 61X67
        else {
            // Miss count: 1,989
            simple_trans(M, N, A, B, 16);
        }
    }
}

/*
 * registerFunctions - This function registers your transpose
 *     functions with the driver.  At runtime, the driver will
 *     evaluate each of the registered functions and summarize their
 *     performance. This is a handy way to experiment with different
 *     transpose strategies.
 */
void registerFunctions() {
    /* Register your solution function */
    registerTransFunction(transpose_submit, transpose_submit_desc);
}

/*
 * is_transpose - This helper function checks if B is the transpose of
 *     A. You can check the correctness of your transpose by calling
 *     it before returning from the transpose function.
 */
int is_transpose(int M, int N, int A[N][M], int B[M][N]) {
    int i, j;

    for (i = 0; i < N; i++) {
        for (j = 0; j < M; ++j) {
            if (A[i][j] != B[j][i]) {
                return 0;
            }
        }
    }
    return 1;
}
