#include <switch.h>
#include <stdio.h>
#include <cstdlib>
#include <cstring>
#include <iterator>
#include <algorithm>
#include <sys/stat.h>
#include <sys/types.h>
#include <cstdio>

#define BENCH_PATH "sdmc:/bench/temporary.bin"

double TicksToSeconds(u64 ticks) {
    return (double)ticks / (double)armGetSystemTickFreq();
}

constexpr size_t BenchSize = (128 * 1024 * 1024);
constexpr size_t SoakRuns  = 16;
constexpr size_t SoakChunk = (1 * 1024 * 1024);

double PerformWriteBench(size_t chunk, FILE *f, u8 *buffer) {
    u64 start = armGetSystemTick();

    size_t written = 0;

    while (written < BenchSize) {
        size_t to_write = std::min(BenchSize - written, chunk);
        size_t result = fwrite(buffer, 1, to_write, f);

        if (result != to_write) {
            fclose(f);
            return -1.0;
        }

        written += result;
    }

    if (fflush(f) != 0) {
        fclose(f);
        return -1.0;
    }

    fclose(f);
    u64 end = armGetSystemTick();

    double sec = TicksToSeconds(end - start);
    double mb = (double)BenchSize / (1024.0 * 1024.0);
    return sec > 0.0 ? mb / sec : -1.0;
}

double PerformReadBench(const size_t chunk, FILE *f, u8 *buffer) {
    u64 start = armGetSystemTick();

    size_t read_total = 0;

    while (read_total < BenchSize) {
        size_t to_read = std::min(BenchSize - read_total, chunk);
        size_t result = fread(buffer, 1, to_read, f);

        if (result != to_read) {
            fclose(f);
            return -1.0;
        }

        read_total += result;
    }

    fclose(f);
    u64 end = armGetSystemTick();

    double sec = TicksToSeconds(end - start);
    double mb = (double)BenchSize / (1024.0 * 1024.0);
    return sec > 0.0 ? mb / sec : -1.0;
}


void PrintSoakRow(size_t run, double mps) {
    printf(" %6.1f", mps);
    if ((run % 8) == 7) {
        printf("\n");
    }
    consoleUpdate(NULL);
}

void RunWriteSoak(bool fresh_files) {
    u8 *buffer = static_cast<u8 *>(malloc(SoakChunk));
    if (!buffer) {
        printf("Failed to allocate buffer\n");
        return;
    }

    memset(buffer, 0xAA, SoakChunk);

    printf("Write soak, %zu x %zu MiB @ %zu MiB, %s:\n", SoakRuns, BenchSize / (1024 * 1024), SoakChunk / (1024 * 1024), fresh_files ? "fresh file" : "same file");

    char path[64];
    double lo = 1.0e9;
    double hi = 0.0;
    size_t done = 0;

    for (size_t run = 0; run < SoakRuns; ++run) {
        if (fresh_files) {
            snprintf(path, sizeof(path), "sdmc:/bench/soak_%02zu.bin", run);
        } else {
            snprintf(path, sizeof(path), "%s", BENCH_PATH);
        }

        FILE *f = fopen(path, "wb");
        if (!f) {
            printf("\nFailed to open %s\n", path);
            break;
        }

        double mps = PerformWriteBench(SoakChunk, f, buffer);
        if (mps < 0.0) {
            printf("\nRun %zu failed\n", run);
            break;
        }

        if (mps < lo) lo = mps;
        if (mps > hi) hi = mps;
        done = run + 1;

        PrintSoakRow(run, mps);
    }

    if ((done % 8) != 0) {
        printf("\n");
    }
    if (done != 0) {
        printf("  min %.1f  max %.1f  drop %.0f%%\n\n", lo, hi, hi > 0.0 ? (100.0 - (lo * 100.0 / hi)) : 0.0);
    }
    consoleUpdate(NULL);

    if (fresh_files) {
        for (size_t run = 0; run < SoakRuns; ++run) {
            snprintf(path, sizeof(path), "sdmc:/bench/soak_%02zu.bin", run);
            remove(path);
        }
    }

    free(buffer);
}

void RunReadSoak(void) {
    u8 *buffer = static_cast<u8 *>(malloc(SoakChunk));
    if (!buffer) {
        printf("Failed to allocate buffer\n");
        return;
    }

    memset(buffer, 0x00, SoakChunk);

    printf("Read soak, %zu x %zu MiB @ %zu MiB, same file:\n", SoakRuns, BenchSize / (1024 * 1024), SoakChunk / (1024 * 1024));

    double lo = 1.0e9;
    double hi = 0.0;
    size_t done = 0;

    for (size_t run = 0; run < SoakRuns; ++run) {
        FILE *f = fopen(BENCH_PATH, "rb");
        if (!f) {
            printf("\nFailed to open file for read\n");
            break;
        }

        double mps = PerformReadBench(SoakChunk, f, buffer);
        if (mps < 0.0) {
            printf("\nRun %zu failed\n", run);
            break;
        }

        if (mps < lo) lo = mps;
        if (mps > hi) hi = mps;
        done = run + 1;

        PrintSoakRow(run, mps);
    }

    if ((done % 8) != 0) {
        printf("\n");
    }
    if (done != 0) {
        printf("  min %.1f  max %.1f  drop %.0f%%\n\n", lo, hi, hi > 0.0 ? (100.0 - (lo * 100.0 / hi)) : 0.0);
    }
    consoleUpdate(NULL);

    free(buffer);
}

int main(int argc, char *argv[]) {
    consoleInit(NULL);
    printf("SDBench 1.1.0\n");
    printf("Press + to exit after results\n\n");
    consoleUpdate(NULL);

    mkdir("sdmc:/bench", 0777);

    const size_t chunks[] = {
        // 4 * 1024,
        // 16 * 1024,
        64 * 1024,
        256 * 1024,
        512 * 1024,
        1   * 1024 * 1024,
        2   * 1024 * 1024,
    };

    for (u32 i = 0; i < std::size(chunks); ++i) {
        u8 *buffer = static_cast<u8 *>(malloc(chunks[i]));
        if (!buffer) {
            printf("Failed to allocate buffer\n");
            break;
        }

        memset(buffer, 0xAA, chunks[i]);

        FILE *f = fopen(BENCH_PATH, "wb");
        if (!f) {
            printf("Failed to open file for write\n");
            free(buffer);
            break;
        }

        double mps = PerformWriteBench(chunks[i], f, buffer);

        if (mps >= 0.0) {
            if (chunks[i] >= 1024 * 1024) {
                printf("Write [%zu MiB]: %.2f MiB/s\n", chunks[i] / (1024 * 1024), mps);
            } else {
                printf("Write [%zu KiB]: %.2f MiB/s\n", chunks[i] / 1024, mps);
            }
        } else {
            printf("Write failed\n");
        }

        consoleUpdate(NULL);
        free(buffer);
    }

    printf("\n");
    consoleUpdate(NULL);

    for (u32 i = 0; i < std::size(chunks); ++i) {
        u8 *buffer = static_cast<u8 *>(malloc(chunks[i]));
        if (!buffer) {
            printf("Failed to allocate buffer\n");
            break;
        }

        memset(buffer, 0x00, chunks[i]);

        FILE *f = fopen(BENCH_PATH, "rb");
        if (!f) {
            printf("Failed to open file for read\n");
            free(buffer);
            break;
        }

        double mps = PerformReadBench(chunks[i], f, buffer);

        if (mps >= 0.0) {
            if (chunks[i] >= 1024 * 1024) {
                printf("Read [%zu MiB]: %.2f MiB/s\n", chunks[i] / (1024 * 1024), mps);
            } else {
                printf("Read [%zu KiB]: %.2f MiB/s\n", chunks[i] / 1024, mps);
            }
        } else {
            printf("Read failed\n");
        }

        consoleUpdate(NULL);
        free(buffer);
    }

    printf("\n");
    consoleUpdate(NULL);

    RunWriteSoak(false);
    RunWriteSoak(true);
    RunReadSoak();

    remove(BENCH_PATH);

    printf("Done.\n");
    printf("Press + to exit.\n");
    consoleUpdate(NULL);

    PadState pad;
    padConfigureInput(1, HidNpadStyleSet_NpadStandard);
    padInitializeDefault(&pad);

    for (;;) {
        padUpdate(&pad);
        if (padGetButtonsDown(&pad) & HidNpadButton_Plus)
            break;
    }

    consoleExit(NULL);
    return 0;
}
