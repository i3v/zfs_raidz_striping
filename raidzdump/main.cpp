/*
 * All the following code is a copy-paste from
 * https://www.tritondatacenter.com/blog/zfs-raidz-striping
 * (Post written by Mr. Max Bruning)
 * This file is just a re-formatted, easier-to-read version.
 */

/* * Given an offset, size, number of disks in the raidz pool,
 * * the number of parity "disks" (1, 2, or 3 for raidz, raidz2, raidz3),
 * * and the sector size (shift),
 * * print a set of stripes. */

#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstdio>
#include "libraidzdump.h"

int main(int argc, char *argv[]) {
    uint64_t nparity = 1;
    uint64_t unit_shift = 9;  /* shouldn't be hard-coded.  sector size */
    raidz_map_t *rzm;
    raidz_col_t *cols;
    int i;
    if (argc < 4) {
        fprintf(stderr, "Usage: %s offset size ndisks [nparity [ashift]]\n", argv[0]);
        fprintf(stderr, "  ndisks is number of disks in raid pool, including parity\n");
        fprintf(stderr, "  nparity defaults to 1 (raidz1)\n");
        fprintf(stderr, "  ashift defaults to 9 (512-byte sectors)\n");
        exit(1);
    }
    /* XXX - check return values */
    uint64_t offset = strtoull(argv[1], nullptr, 16);
    uint64_t size = strtoull(argv[2], nullptr, 16);
    uint64_t dcols = strtoull(argv[3], nullptr, 16);
    if (size == 0 || dcols == 0) {
        /* should check size multiple of ashift...*/
        fprintf(stderr, "size and/or number of columns must be > 0\n");
        exit(1);
    }
    if (argc > 4) nparity = strtoull(argv[4], nullptr, 16);
    if (argc == 6) unit_shift = strtoull(argv[5], nullptr, 16);
    rzm = vdev_raidz_map_get(size, offset, unit_shift, dcols, nparity);
    printf("cols = %ld, firstdatacol = %ld\n", rzm->rm_cols, rzm->rm_firstdatacol);
    for (i = 0, cols = &rzm->rm_col[0]; uint64_t(i) < rzm->rm_cols; i++, cols++)
        printf("%ld:%lx:%lx\n", cols->rc_devidx, cols->rc_offset, cols->rc_size);
    exit(0);
}