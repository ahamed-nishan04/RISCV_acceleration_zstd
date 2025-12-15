#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <zstd.h>

// 512 MB Test Size (Smaller than 1GB to prevent RAM swapping, but large enough for window tests)
#define TEST_SIZE (512ULL * 1024 * 1024)

long long get_time_ns() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (long long)ts.tv_sec * 1000000000LL + ts.tv_nsec;
}

void generate_graduated_data(char* buffer, size_t size) {
    printf("[-] Generating 'Graduated Difficulty' Data...\n");
    
    // 1. Fill background with "Semi-Random" data
    // We use % 100 so it has SOME entropy, but isn't pure noise.
    // This allows Huff0 (Entropy) to compress it slightly (~20-30%),
    // giving us a baseline to measure match-finding against.
    printf("    ...Filling background noise (Base entropy)...\n");
    for (size_t i = 0; i < size; i++) {
        buffer[i] = (char)(rand() % 100); 
    }

    // 2. Inject Matches at specific distances
    // Zstd Levels differ mainly in how FAR back they look (Window Size) 
    // and how HARD they search (Hash Chain vs Binary Tree).
    
    size_t copy_len = 50 * 1024; // 50KB match blocks
    
    // DISTANCE 1: 1 MB (Easy)
    // Level 1 should catch this.
    printf("    ...Injecting matches at -1MB distance (Easy)\n");
    for (size_t i = 1024*1024 + copy_len; i < size / 4; i += 1024*1024) {
        // Copy data from 1MB ago
        memcpy(&buffer[i], &buffer[i - (1024*1024)], copy_len);
    }

    // DISTANCE 2: 8 MB (Medium)
    // Low levels might miss this due to small hash tables.
    printf("    ...Injecting matches at -8MB distance (Medium)\n");
    for (size_t i = size/4; i < size/2; i += 8*1024*1024) {
        if (i > 8*1024*1024)
            memcpy(&buffer[i], &buffer[i - (8*1024*1024)], copy_len);
    }

    // DISTANCE 3: 64 MB (Hard)
    // Requires large WindowLog. Level 1 will fail. Level 19 should succeed.
    printf("    ...Injecting matches at -64MB distance (Hard)\n");
    for (size_t i = size/2; i < size; i += 64*1024*1024) {
        if (i > 64*1024*1024)
            memcpy(&buffer[i], &buffer[i - (64*1024*1024)], copy_len);
    }
}

int main() {
    printf("[1] Allocating 512MB Memory...\n");
    void* srcBuffer = malloc(TEST_SIZE);
    if (!srcBuffer) { perror("Alloc Failed"); return 1; }

    generate_graduated_data((char*)srcBuffer, TEST_SIZE);

    size_t dstCapacity = ZSTD_compressBound(TEST_SIZE);
    void* dstBuffer = malloc(dstCapacity);
    if (!dstBuffer) { free(srcBuffer); return 1; }

    printf("\n[2] Running Gradient Test (Watch the 'Ratio' column change!)\n");
    printf("%-6s | %-12s | %-12s | %-10s | %-15s\n", "Level", "Time (s)", "Speed (MB/s)", "Ratio", "Size (MB)");
    printf("-------|--------------|--------------|------------|----------------\n");

    // We test specific levels to show the jumps
    int levels[] = {1, 3, 5, 7, 10, 15, 19, 0}; 

    for (int i = 0; levels[i] != 0; i++) {
        int lvl = levels[i];
        
        long long start = get_time_ns();
        size_t cSize = ZSTD_compress(dstBuffer, dstCapacity, srcBuffer, TEST_SIZE, lvl);
        long long end = get_time_ns();

        if (ZSTD_isError(cSize)) {
            printf("Lvl %d Error: %s\n", lvl, ZSTD_getErrorName(cSize));
            continue;
        }

        double time_sec = (end - start) / 1000000000.0;
        double speed = (TEST_SIZE / (1024.0 * 1024.0)) / time_sec;
        double ratio = (double)TEST_SIZE / cSize;
        double compressed_mb = cSize / (1024.0 * 1024.0);

        printf("Lvl %-2d | %12.4f | %12.2f | %10.2f | %15.2f\n", 
               lvl, time_sec, speed, ratio, compressed_mb);
        fflush(stdout);
    }

    free(srcBuffer);
    free(dstBuffer);
    return 0;
}
