#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <zstd.h> // Requires libzstd-devel installed

// Helper to get time in nanoseconds
long long get_time_ns() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (long long)ts.tv_sec * 1000000000LL + ts.tv_nsec;
}

int main() {
    // 1. Setup Data: 10MB of Mixed Data
    size_t srcSize = 10 * 1024 * 1024;
    void* srcBuffer = malloc(srcSize);
    
    // Fill with pattern: 50% random, 50% repeated text
    char* ptr = (char*)srcBuffer;
    for (size_t i = 0; i < srcSize; i++) {
        if (i % 2 == 0) ptr[i] = rand() % 256; // Random noise
        else ptr[i] = "RepeatPattern"[i % 13]; // Predictable
    }

    // 2. Setup Destination
    size_t dstCapacity = ZSTD_compressBound(srcSize);
    void* dstBuffer = malloc(dstCapacity);

    // 3. Profile: Level 1 (Fast - Stresses IO/Hashing)
    long long start = get_time_ns();
    size_t cSize1 = ZSTD_compress(dstBuffer, dstCapacity, srcBuffer, srcSize, 1);
    long long end = get_time_ns();
    
    if (ZSTD_isError(cSize1)) {
        printf("Error: %s\n", ZSTD_getErrorName(cSize1));
        return 1;
    }
    printf("Level 1 (Fast Hash) Time:  %.2f ms\n", (end - start) / 1000000.0);

    // 4. Profile: Level 19 (High - Stresses Match Finder/Parser)
    // This effectively isolates the "Search" stage because Entropy coding 
    // cost is constant, but Search cost skyrockets here.
    start = get_time_ns();
    size_t cSize19 = ZSTD_compress(dstBuffer, dstCapacity, srcBuffer, srcSize, 19);
    end = get_time_ns();

    printf("Level 19 (Deep Search) Time: %.2f ms\n", (end - start) / 1000000.0);

    // 5. Cleanup
    free(srcBuffer);
    free(dstBuffer);
    return 0;
}
