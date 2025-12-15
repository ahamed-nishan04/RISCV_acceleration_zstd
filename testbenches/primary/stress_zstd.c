#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <zstd.h>

// 1 GB Test Size
#define TEST_SIZE (1ULL * 1024 * 1024 * 1024)

// Helper: High-resolution timing
long long get_time_ns() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (long long)ts.tv_sec * 1000000000LL + ts.tv_nsec;
}

// Helper: Fill buffer with data designed to torture specific Zstd components
void generate_stress_data(char* buffer, size_t size) {
    printf("[-] Generating %zu bytes of 'Zstd Killer' data...\n", size);
    
    size_t chunk = size / 4;
    size_t i;

    // SECTION 1: High Entropy (Random) -> Stress Huff0
    printf("    ...Filling Section 1: Random Noise (Huff0 Stress)\n");
    for (i = 0; i < chunk; i++) {
        buffer[i] = rand() % 256;
    }

    // SECTION 2: Short Repeating Patterns -> Stress Hash Chain
    printf("    ...Filling Section 2: Short Patterns (LZ77 Stress)\n");
    const char* pattern = "ZstdIsFastZstdIsFast";
    size_t patLen = strlen(pattern);
    for (; i < chunk * 2; i++) {
        buffer[i] = pattern[i % patLen];
    }

    // SECTION 3: Structured Repetition -> Stress Repcodes
    printf("    ...Filling Section 3: Structured Tables (Repcode Stress)\n");
    for (; i < chunk * 3; i++) {
        if (i % 100 < 10) buffer[i] = 'A'; 
        else buffer[i] = (char)(i % 255);
    }

    // SECTION 4: Long Distance Matches -> Stress Window/Cache
    printf("    ...Filling Section 4: Long Distance Copies (Window Stress)\n");
    memcpy(buffer + (chunk * 3), buffer, chunk);
}

int main() {
    // 1. Setup Data
    printf("[1] Allocating Memory...\n");
    void* srcBuffer = malloc(TEST_SIZE);
    if (!srcBuffer) { perror("Malloc Source Failed"); return 1; }

    generate_stress_data((char*)srcBuffer, TEST_SIZE);

    size_t dstCapacity = ZSTD_compressBound(TEST_SIZE);
    void* dstBuffer = malloc(dstCapacity);
    if (!dstBuffer) { perror("Malloc Dest Failed"); free(srcBuffer); return 1; }

    // 2. RUN THE GAUNTLET: Levels 1 to 19
    printf("\n[2] Starting Full Spectrum Stress Test (Levels 1-19)...\n");
    printf("%-6s | %-12s | %-12s | %-10s | %-15s\n", "Level", "Time (s)", "Speed (MB/s)", "Ratio", "Size (MB)");
    printf("-------|--------------|--------------|------------|----------------\n");

    for (int level = 1; level <= 19; level++) {
        long long start = get_time_ns();
        
        // The core compression call
        size_t cSize = ZSTD_compress(dstBuffer, dstCapacity, srcBuffer, TEST_SIZE, level);
        
        long long end = get_time_ns();

        if (ZSTD_isError(cSize)) {
            printf("Level %d Error: %s\n", level, ZSTD_getErrorName(cSize));
            continue;
        }

        double time_sec = (end - start) / 1000000000.0;
        double mb_size = TEST_SIZE / (1024.0 * 1024.0);
        double speed = mb_size / time_sec;
        double compressed_mb = cSize / (1024.0 * 1024.0);
        double ratio = (double)TEST_SIZE / cSize;

        printf("Lvl %-2d | %12.4f | %12.2f | %10.2f | %15.2f\n", 
               level, time_sec, speed, ratio, compressed_mb);
        
        // Flush stdout so you see results immediately
        fflush(stdout); 
    }

    // 3. Cleanup
    printf("\n[3] Test Complete.\n");
    free(srcBuffer);
    free(dstBuffer);
    return 0;
}
