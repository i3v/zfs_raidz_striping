#ifndef ZFS_RAIDZ_STRIPING_LIBRAIDZDUMP_H
#define ZFS_RAIDZ_STRIPING_LIBRAIDZDUMP_H

/* * The following are taken straight from usr/src/uts/common/fs/zfs/vdev_raidz.c
 * * If they change there, they need to be changed here.
 * * a map of columns returned for a given offset and size
 *
 * It differs from https://github.com/openzfs/zfs/blob/zfs-0.7.9/include/sys/vdev_raidz_impl.h#L115 , but seem to still work.
 * */

typedef struct raidz_col {
    uint64_t rc_devidx;     /* child device index for I/O */
    uint64_t rc_offset;     /* device offset */
    uint64_t rc_size;       /* I/O size */
    void *rc_data;          /* I/O data */
    void *rc_gdata;         /* used to store the "good" version */
    int rc_error;           /* I/O error for this device */
    uint8_t rc_tried;       /* Did we attempt this I/O column? */
    uint8_t rc_skipped;     /* Did we skip this I/O column? */
} raidz_col_t;

typedef struct raidz_map {
    uint64_t rm_cols;       /* Regular column count */
    uint64_t rm_scols;      /* Count including skipped columns */
    uint64_t rm_bigcols;        /* Number of oversized columns */
    uint64_t rm_asize;      /* Actual total I/O size */
    uint64_t rm_missingdata;    /* Count of missing data devices */
    uint64_t rm_missingparity;  /* Count of missing parity devices */
    uint64_t rm_firstdatacol;   /* First data column/parity count */
    uint64_t rm_nskip;      /* Skipped sectors for padding */
    uint64_t rm_skipstart;  /* Column index of padding start */
    void *rm_datacopy;      /* rm_asize-buffer of copied data */
    uintptr_t rm_reports;       /* # of referencing checksum reports */
    uint8_t rm_freed;       /* map no longer has referencing ZIO */
    uint8_t rm_ecksuminjected;  /* checksum error was injected */
    raidz_col_t rm_col[];      /* Flexible array of I/O columns */
} raidz_map_t;

raidz_map_t * vdev_raidz_map_get(uint64_t size, uint64_t offset, uint64_t unit_shift, uint64_t dcols, uint64_t nparity);


#endif //ZFS_RAIDZ_STRIPING_LIBRAIDZDUMP_H
