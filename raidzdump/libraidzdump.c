
#include <sys/param.h>
#include <stdint.h>
#include <malloc.h>
#include <stdlib.h>
#include "libraidzdump.h"

#define BROKEN_OFFSETOF_MACRO
#ifdef BROKEN_OFFSETOF_MACRO
    #undef offsetof
    #define offsetof(type, member)   ((size_t)((char *)&(*(type *)0).member - \
                                               (char *)&(*(type *)0)))
#endif

/*
 * All the following code is a copy-paste from
 * https://www.tritondatacenter.com/blog/zfs-raidz-striping
 * (Post written by Mr. Max Bruning)
 * with minor adjustments.
 */

/* *  vdev_raidz_map_get() is hacked from vdev_raidz_map_alloc() in
 * *  usr/src/uts/common/fs/zfs/vdev_raidz.c.
 * * If that routine changes,
 * *  this might also need changing. */

raidz_map_t* vdev_raidz_map_get(uint64_t size, uint64_t offset, uint64_t unit_shift, uint64_t dcols, uint64_t nparity) {
    raidz_map_t *rm;
    uint64_t b = offset >> unit_shift;
    uint64_t s = size >> unit_shift;
    uint64_t f = b % dcols;
    uint64_t o = (b / dcols) << unit_shift;
    uint64_t q, r, c, bc, col, acols, scols, coff, devidx, asize, tot;
    q = s / (dcols - nparity);
    r = s - q * (dcols - nparity);
    bc = (r == 0 ? 0 : r + nparity);
    tot = s + nparity * (q + (r == 0 ? 0 : 1));
    if (q == 0) {
        acols = bc;
        scols = MIN(dcols, roundup(bc, nparity + 1));
    }
    else {
        acols = dcols;
        scols = dcols;
    }
    rm = malloc(offsetof(raidz_map_t, rm_col[scols]));
    if (rm == NULL) {
        fprintf(stderr, "malloc failed\n");
        exit(1);
    }
    rm->rm_cols = acols;
    rm->rm_scols = scols;
    rm->rm_bigcols = bc;
    rm->rm_skipstart = bc;
    rm->rm_missingdata = 0;
    rm->rm_missingparity = 0;
    rm->rm_firstdatacol = nparity;
    rm->rm_datacopy = NULL;
    rm->rm_reports = 0;
    rm->rm_freed = 0;
    rm->rm_ecksuminjected = 0;
    asize = 0;
    for (c = 0; c < scols; c++) {
        col = f + c;
        coff = o;
        if (col >= dcols) {
            col -= dcols;
            coff += 1ULL << unit_shift;
        }
        rm->rm_col[c].rc_devidx = col;
        rm->rm_col[c].rc_offset = coff;
        rm->rm_col[c].rc_data = NULL;
        rm->rm_col[c].rc_gdata = NULL;
        rm->rm_col[c].rc_error = 0;
        rm->rm_col[c].rc_tried = 0;
        rm->rm_col[c].rc_skipped = 0;
        if (c >= acols) rm->rm_col[c].rc_size = 0;
        else if (c < bc)
            rm->rm_col[c].rc_size = (q + 1) << unit_shift;
        else rm->rm_col[c].rc_size = q << unit_shift;
        asize += rm->rm_col[c].rc_size;
    }
    rm->rm_asize = roundup(asize, (nparity + 1) << unit_shift);
    rm->rm_nskip = roundup(tot, nparity + 1) - tot;
    if (rm->rm_firstdatacol == 1 && (offset & (1ULL << 20))) {
        devidx = rm->rm_col[0].rc_devidx;
        o = rm->rm_col[0].rc_offset;
        rm->rm_col[0].rc_devidx = rm->rm_col[1].rc_devidx;
        rm->rm_col[0].rc_offset = rm->rm_col[1].rc_offset;
        rm->rm_col[1].rc_devidx = devidx;
        rm->rm_col[1].rc_offset = o;
        if (rm->rm_skipstart == 0) rm->rm_skipstart = 1;
    }
    return (rm);
}
