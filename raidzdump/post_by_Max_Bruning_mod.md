The idea of this file
---------------------
This file reflects the changes I had to made to reproduce the [post_by_Max_Bruning.md](post_by_Max_Bruning.md).
I tried to keep everything as close as possible for now.
My system is CentOS 7.5.1804, zfs 0.7.9.

ZFS RaidZ Striping
-----------------
Recently on the ZFS mailing list (see http://wiki.illumos.org/display/illumos/illumos+Mailing+Lists ), there was some discussion about how ZFS distributes data across disks. I thought I might show some work I've done to better understand this.

The disk blocks for a `raidz`/`raidz2`/`raidz3` vdev are across all vdevs in the pool. So, for instance, the data for block offset `0xc00000` with size `0x20000` (as reported by [`zdb(1M)`](http://illumos.org/man/1m/zdb)) could be striped at different locations and various sizes on the individual disks within the raidz volume. In other words, the offsets and sizes are absolute with respect to the volume, not to the individual disks.

The work of mapping a raidz pool offset and size to individual disks within the pool is done by [`vdev_raidz_map_alloc()`](http://src.illumos.org/source/xref/illumos-gate/usr/src/uts/common/fs/zfs/vdev_raidz.c#435). (Note that this routine has been changed SmartOS in support of allowing system crash dumps to be written to raidz volumes. A change that will eventually be pushed upstream to illumos.)

Let's go through an example. First, we'll set up a raidz pool and put some data into it.


```
# for i in {0..4}; do dd if=/dev/zero of=/var/tmp/f$i bs=1M count=100; done
# zpool create rzpool raidz /var/tmp/f0 /var/tmp/f1 /var/tmp/f2 /var/tmp/f3 /var/tmp/f4
# dd if=/usr/share/dict/words of=/rzpool/words bs=206674 count=1
```
And now let's see the blocks assigned to the `/rzpool/words` file:
```
# zdb  -bbbvvv rzpool -O words

    Object  lvl   iblk   dblk  dsize  dnsize  lsize   %full  type
         2    2   128K   128K   259K     512   256K  100.00  ZFS plain file (K=inherit) (Z=inherit)
                                               176   bonus  System attributes
        dnode flags: USED_BYTES USERUSED_ACCOUNTED USEROBJUSED_ACCOUNTED
        dnode maxblkid: 1
        uid     0
        gid     0
        atime   Fri Mar 15 15:18:54 2024
        mtime   Fri Mar 15 15:18:54 2024
        ctime   Fri Mar 15 15:18:54 2024
        crtime  Fri Mar 15 15:18:54 2024
        gen     27
        mode    100644
        size    206674
        parent  34
        links   1
        pflags  40800000004
        xattr   3
Indirect blocks:
               0 L1  DVA[0]=<0:a4400:800> DVA[1]=<0:6054000:800> [L1 ZFS plain file] fletcher4 lz4 LE contiguous unique double size=20000L/400P birth=27L/27P fill=2 cksum=8c03663609:5b6c64d0a0df:1fdf41401ac80b:7d5e8f8bf93bed2
               0  L0 DVA[0]=<0:54400:28000> [L0 ZFS plain file] fletcher4 uncompressed LE contiguous unique single size=20000L/20000P birth=27L/27P fill=1 cksum=2f713a36a432:bde8e91629f53c8:857c10df2518343c:1e81009edc6da7bf
           20000  L0 DVA[0]=<0:7c400:28000> [L0 ZFS plain file] fletcher4 uncompressed LE contiguous unique single size=20000L/20000P birth=27L/27P fill=1 cksum=1c342839f006:a02eb9dc774debb:12afed9ecbe595eb:8390d19083c5e62c

                segment [0000000000000000, 0000000000040000) size  256K
```


So, there are two blocks, one at offset `0x54400` and the other at offset `0x7c400`, both of them `0x28000` bytes. Of the `0x28000` bytes, `0x8000` is parity. The real data size is `0x20000`. The question is, where does this data physically reside, i.e., which disk(s), and where on those disks?

I wrote a small program that uses the code from the `vdev_raidz_map_alloc()` routine to tell me the mapping that is set up. Here it is:

```
<<< see main.cpp >>>
```

The program takes an offset, size, number of disks in the pool, and optionally the number of parity (1 for raidz, 2 for raidz2 and 3 for raidz3) and the sector size (shift), and outputs the location of the blocks on the underlying disks. Let's try it.

```
# ./raidzdump 54400 20000 5
cols = 5, firstdatacol = 1
4:10c00:8000
0:10e00:8000
1:10e00:8000
2:10e00:8000
3:10e00:8000

```
The parity for the block is on disk4 (`/var/tmp/f4`), the first 32k of data on disk0 (`/var/tmp/f0`), the second on disk1, etc. <br>
We could use `zdb(1M)` to check this:
```
# zdb -R rzpool 0.0:10e00:8000:r > read_v1.txt
```
This says to go to vdev 0 (`/var/tmp/f0`, child of the root vdev 0), at location `0x10e00`, and read `0x8000` (32k) of data and display it.

Let's try a different way. We'll add the `0x10e00` to the end of the disk label of 4MB at the beginning of every disk to get an absolute byte offset within the disk, then we'll use dd to look at the data.

```
# dd if=/var/tmp/f0 bs=1 skip=$((0x10e00+4*1024*1024)) count=32k > read_v2.txt
```

Create a reference-result and compare the contents:
```
# dd if=/rzpool/words of=read_v0.txt bs=32k count=1 > read_v0.txt
# diff read_v0.txt read_v1.txt
# diff read_v0.txt read_v2.txt
```


And there is the first 32k of the words file. To get the next 32k,use the offset from the third line of output from the raidzdump utility, and so on. This gets more interesting with smaller blocks, but that is left as an exercise for the reader. For instance, a write of a 512-byte file will stripe across 2 disks, one for the data, the other for parity. 


-----------------------------------------------------------------

A test for raidz2.
-----------------

```
# for i in {0..5}; do dd if=/dev/zero of=/var/tmp/f$i bs=1M count=100; done
# zpool create rzpool raidz2 /var/tmp/f0 /var/tmp/f1 /var/tmp/f2 /var/tmp/f3 /var/tmp/f4 /var/tmp/f5
# dd if=/usr/share/dict/words of=/rzpool/words bs=206674 count=1
```

```
# zdb  -bbbvvv rzpool -O words

    Object  lvl   iblk   dblk  dsize  dnsize  lsize   %full  type
         2    2   128K   128K   260K     512   256K  100.00  ZFS plain file (K=inherit) (Z=inherit)
                                               176   bonus  System attributes
        dnode flags: USED_BYTES USERUSED_ACCOUNTED USEROBJUSED_ACCOUNTED
        dnode maxblkid: 1
        uid     0
        gid     0
        atime   Fri Mar 15 17:34:29 2024
        mtime   Fri Mar 15 17:34:29 2024
        ctime   Fri Mar 15 17:34:29 2024
        crtime  Fri Mar 15 17:34:29 2024
        gen     18
        mode    100644
        size    206674
        parent  34
        links   1
        pflags  40800000004
        xattr   3
Indirect blocks:
               0 L1  DVA[0]=<0:ca200:c00> DVA[1]=<0:7069c00:c00> [L1 ZFS plain file] fletcher4 lz4 LE contiguous unique double size=20000L/400P birth=18L/18P fill=2 cksum=8bb26676f0:5b1c07113667:1fb73a8f5de237:7c8916888fdf522
               0  L0 DVA[0]=<0:6a200:30000> [L0 ZFS plain file] fletcher4 uncompressed LE contiguous unique single size=20000L/20000P birth=18L/18P fill=1 cksum=2f713a36a432:bde8e91629f53c8:857c10df2518343c:1e81009edc6da7bf
           20000  L0 DVA[0]=<0:9a200:30000> [L0 ZFS plain file] fletcher4 uncompressed LE contiguous unique single size=20000L/20000P birth=18L/18P fill=1 cksum=1c342839f006:a02eb9dc774debb:12afed9ecbe595eb:8390d19083c5e62c

                segment [0000000000000000, 0000000000040000) size  256K
```

```
# ./raidzdump 6a200 20000 6 2
cols = 6, firstdatacol = 2
3:11a00:8000
4:11a00:8000
5:11a00:8000
0:11c00:8000
1:11c00:8000
2:11c00:8000
```

```
# dd if=/var/tmp/f5 bs=1 skip=$((0x11a00+4*1024*1024)) count=$((0x8000)) > read_v4.txt
# diff read_v4.txt read_v0.txt
```

Thus, this seems to work OK with raidz2 too.
