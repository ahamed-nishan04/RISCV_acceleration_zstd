#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <zstd.h>

// Helper: High-resolution timing
long long get_time_ns() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (long long)ts.tv_sec * 1000000000LL + ts.tv_nsec;
}

// Helper: Load specific file from disk
size_t load_wav_file(const char* filename, void** buffer) {
    printf("[-] Reading file: %s ...\n", filename);
    
    FILE* f = fopen(filename, "rb");
    if (!f) {
        perror("    Error opening file");
        return 0;
    }

    // Get file size
    fseek(f, 0, SEEK_END);
    size_t fsize = ftell(f);
    fseek(f, 0, SEEK_SET);

    if (fsize == 0) {
        printf("    Error: File is empty.\n");
        fclose(f);
        return 0;
    }

    // Sanity check: WAV header is usually 44 bytes. 
    // If it's smaller, it's not a valid audio file.
    if (fsize < 44) {
        printf("    Warning: File is too small to be a valid WAV.\n");
    }

    printf("    File Size: %.2f MB\n", fsize / (1024.0 * 1024.0));

    // Allocate memory
    *buffer = malloc(fsize);
    if (!*buffer) {
        perror("    Malloc failed (File too big?)");
        fclose(f);
        return 0;
    }

    // Read data
    size_t readSize = fread(*buffer, 1, fsize, f);
    fclose(f);

    if (readSize != fsize) {
        printf("    Warning: Read partial data (%zu of %zu bytes)\n", readSize, fsize);
    }

    return readSize;
}

int main() {
    // --- CONFIGURATION ---
    const char* inputFilename = "File.wav"; 
    // ---------------------

    void* srcBuffer = NULL;
    size_t srcSize = load_wav_file(inputFilename, &srcBuffer);

    if (!srcBuffer || srcSize == 0) {
        printf("Exiting...\n");
        return 1;
    }

    // Calculate destination size (Zstd might need more space than input in worst case)
    size_t dstCapacity = ZSTD_compressBound(srcSize);
    void* dstBuffer = malloc(dstCapacity);
    if (!dstBuffer) {
        perror("Malloc Dest Failed");
        free(srcBuffer);
        return 1;
    }

    printf("\n[2] Running Compression Test on Audio Data\n");
    printf("%-6s | %-12s | %-12s | %-10s | %-15s\n", "Level", "Time (s)", "Speed (MB/s)", "Ratio", "Size (MB)");
    printf("-------|--------------|--------------|------------|----------------\n");

    // We test a spread of levels to see the difference
    int levels[] = {1, 3, 5, 7, 10, 15, 19, 0}; 

    for (int i = 0; levels[i] != 0; i++) {
        int lvl = levels[i];
        long long start = get_time_ns();
        
        size_t cSize = ZSTD_compress(dstBuffer, dstCapacity, srcBuffer, srcSize, lvl);
        
        long long end = get_time_ns();

        if (ZSTD_isError(cSize)) {
            printf("Lvl %d Error: %s\n", lvl, ZSTD_getErrorName(cSize));
            continue;
        }

        double time_sec = (end - start) / 1000000000.0;
        double mb_size = srcSize / (1024.0 * 1024.0);
        double speed = mb_size / time_sec;
        double compressed_mb = cSize / (1024.0 * 1024.0);
        double ratio = (double)srcSize / cSize;

        printf("Lvl %-2d | %12.4f | %12.2f | %10.2f | %15.2f\n", 
               lvl, time_sec, speed, ratio, compressed_mb);
        fflush(stdout);
    }

    printf("\n[3] Cleanup.\n");
    free(srcBuffer);
    free(dstBuffer);
    return 0;
}
