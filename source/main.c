#include <switch.h>
#include <stdio.h>
#include <string.h>

#include <sys/stat.h>
#include <sys/types.h>

#define BENCH_PATH "sdmc:/bench/temporary.bin"
#define BENCH_SIZE (128 * 1024 * 1024)
#define CHUNK      (512 * 1024)

static u8 buf[CHUNK];
static u8 buf2[CHUNK];

double ticks_to_seconds(u64 ticks)
{
    return (double)ticks / (double)armGetSystemTickFreq();
}

int main(int argc, char* argv[])
{
    consoleInit(NULL);
    printf("SDBench 1.0.0\n");
    printf("Press + to exit after results\n\n");
    consoleUpdate(NULL);

    mkdir("sdmc:/bench", 0777);

    memset(buf,  0xAA, sizeof(buf));
    memset(buf2, 0x00, sizeof(buf2));

    FILE* f = fopen(BENCH_PATH, "wb");
    if (!f) {
        printf("Failed to open file for write\n");
        consoleUpdate(NULL);
    } else {
        u64 start = armGetSystemTick();

        size_t written = 0;
        while (written < BENCH_SIZE) {
            size_t to_write = ((BENCH_SIZE - written) > CHUNK) ? CHUNK : (BENCH_SIZE - written);
            fwrite(buf, 1, to_write, f);
            written += to_write;
        }

        fflush(f);
        fclose(f);

        u64 end = armGetSystemTick();
        double sec = ticks_to_seconds(end - start);
        double mb = (double)BENCH_SIZE / (1024.0 * 1024.0);
        double mbps = mb / sec;

        printf("Write completed in %.3fs (%.2f MB/s)\n", sec, mbps);
        consoleUpdate(NULL);
    }

    f = fopen(BENCH_PATH, "rb");
    if (!f) {
        printf("Failed to open file for read\n");
        consoleUpdate(NULL);
    } else {
        u64 start = armGetSystemTick();

        size_t read_total = 0;
        while (read_total < BENCH_SIZE) {
            size_t to_read = ((BENCH_SIZE - read_total) > CHUNK) ? CHUNK : (BENCH_SIZE - read_total);
            fread(buf2, 1, to_read, f);
            read_total += to_read;
        }

        fclose(f);

        u64 end = armGetSystemTick();
        double sec = ticks_to_seconds(end - start);
        double mb = (double)BENCH_SIZE / (1024.0 * 1024.0);
        double mbps = mb / sec;

        printf("Read completed in %.3fs (%.2f MB/s)\n", sec, mbps);
        consoleUpdate(NULL);
    }

    remove(BENCH_PATH);

    printf("\nDone.\n");
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
