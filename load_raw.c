// Copyright(C) 2018 Iti Shree

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <math.h>

static char *cmd_name = NULL;

static uint32_t map_base = 0x18000000;
static uint32_t map_size = 0x08000000;

static uint32_t map_addr = 0x00000000;

static char *dev_mem = "/dev/mem";

static bool opt_raw = false;
static bool opt_pixel = false;
static bool opt_overlay = false;

static uint32_t num_tpat = 0;

/*static inline
uint64_t calc_tpat(unsigned num, unsigned col, unsigned row) {
    switch(num) {
         }
    }
}*/


static
void write_line(uint8_t *bp, uint64_t *dp, unsigned row) {
    uint64_t val = 0;
    uint64_t mask = 0;
    unsigned inc;

    inc = 3;  /* Step-size for RGB */

    mask = ~0xFFFF;

    for (unsigned col = 0; col < 2048; col++) {
        /* copying data from the source */

        val = bp[0];
        val = (val << 8) | bp[1];
        val = (val << 8) | bp[2];
        val = (val << 8) | bp[3];
        val = (val << 8) | bp[4];
        val = (val << 8) | bp[5];
        val = (val << 16);

        *dp = (*dp & ~mask) | (val & mask);
        dp++;
        bp += inc;
    }
    printf("%u\n", dp);
}

int main(int argc, char **argv) {
    extern int optind;
    extern char *optarg;
    int c, err_flag = 0;

#define    OPTIONS "hr"

    cmd_name = argv[0];
    while ((c = getopt(argc, argv, OPTIONS)) != EOF) {
        switch (c) {
            case 'h':
                fprintf(stderr,
                        "options are:\n"
                        "-h        print this help message\n"
                        "-r        load raw data\n", cmd_name);
                exit(0);
                break;

            case 'r':
                opt_raw = true;
                break;
            default:
                err_flag++;
                break;
        }
    }

    if (err_flag) {
        fprintf(stderr,
                "Usage: %s -[" OPTIONS "] [file]\n"
                "%s -h for help.\n",
                cmd_name, cmd_name);
        exit(2);
    }


    int fd = open(dev_mem, O_RDWR | O_SYNC);
    if (fd == -1) {
        fprintf(stderr,
                "error opening >%s<.\n%s\n",
                dev_mem, strerror(errno));
        exit(1);
    }

    /* Mapping of frame buffer starts from here */
    if (map_addr == 0)
        map_addr = map_base;

    void *base = mmap((void *) map_addr, map_size,
                      PROT_READ | PROT_WRITE, MAP_SHARED,
                      fd, map_base);
    if (base == (void *) -1) {
        fprintf(stderr,
                "error mapping 0x%08lX+0x%08lX @0x%08lX.\n%s\n",
                (long) map_base, (long) map_size, (long) map_addr,
                strerror(errno));
        exit(2);
    }

    fprintf(stderr,
            "mapped 0x%08lX+0x%08lX to 0x%08lX.\n",
            (long unsigned) map_base, (long unsigned) map_size,
            (long unsigned) base);   /* Mapping ends */

    // Checking if filename is supplied if yes, open as stdin

    if (argc > optind) {
        close(0);
        open(argv[optind], O_RDONLY, 0);
    }

    /* line 464-466
     * fprintf(stderr,
        "read buffer = 0x%08lX\n",
         (unsigned long)buf_base[0]);*/

    // Iteration over every row count of 1536 for raw image format

    for (unsigned row = 0; row < 1536; row++) {

        uint8_t buf[2048 * 6];    /* Creating local buffer which can hold one row */
        uint8_t *bp = buf;        /* bp stands for buffer pointer */
        size_t buf_size;

        buf_size = 2048 * 3;

        /* Checking if we want to load the data from the file */
        if ((num_tpat == 0) && !opt_pixel) {
            size_t total = 0;
            while (total < buf_size) {
                size_t len = read(0, bp, buf_size - total);

                if (len == 0)
                    exit(1);
                if (len < 0)
                    exit(2);

                total += len;
                bp += len;
            }
        }

        bp = buf;

        // Only calling write_line() when we have to write active raw buffer

        uint32_t dp_base = map_addr + (buf_base[0] - map_base);
        uint64_t *dp = (uint64_t *) (dp_base + row * 16384);  /* dp stands for data pointer */

        write_line(bp, dp, row);
    }

    exit((err_flag) ? 1 : 0);
}
