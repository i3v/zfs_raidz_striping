ZFS RaidZ Striping
-----------------
Recently on the ZFS mailing list (see http://wiki.illumos.org/display/illumos/illumos+Mailing+Lists ), there was some discussion about how ZFS distributes data across disks. I thought I might show some work I've done to better understand this.

The disk blocks for a `raidz`/`raidz2`/`raidz3` vdev are across all vdevs in the pool. So, for instance, the data for block offset `0xc00000` with size `0x20000` (as reported by [`zdb(1M)`](http://illumos.org/man/1m/zdb)) could be striped at different locations and various sizes on the individual disks within the raidz volume. In other words, the offsets and sizes are absolute with respect to the volume, not to the individual disks.

The work of mapping a raidz pool offset and size to individual disks within the pool is done by [`vdev_raidz_map_alloc()`](http://src.illumos.org/source/xref/illumos-gate/usr/src/uts/common/fs/zfs/vdev_raidz.c#435). (Note that this routine has been changed SmartOS in support of allowing system crash dumps to be written to raidz volumes. A change that will eventually be pushed upstream to illumos.)

Let's go through an example. First, we'll set up a raidz pool and put some data into it.


```
# mkfile 100m /var/tmp/f0 /var/tmp/f1 /var/tmp/f2 /var/tmp/f3 /var/tmp/f4
# zpool create rzpool raidz /var/tmp/f0 /var/tmp/f1 /var/tmp/f2 /var/tmp/f3 /var/tmp/f4
# cp /usr/dict/words /rzpool/words
#
```
And now let's see the blocks assigned to the `/rzpool/words` file.

```
# zdb -dddddddd rzpool 
...    
      Object  lvl   iblk   dblk  dsize  lsize   %full  type         
           8    2    16K   128K   259K   256K  100.00  ZFS plain file (K=inherit) (Z=inherit)                                                   
                                         168   bonus   System attributes       
      dnode flags: USED_BYTES USERUSED_ACCOUNTED      
      dnode maxblkid: 1       
      path    /words  
      uid     0       
      gid     0       
      atime   Thu Apr 11 11:46:09 2013        
      mtime   Thu Apr 11 11:46:09 2013        
      ctime   Thu Apr 11 11:46:09 2013        
      crtime  Thu Apr 11 11:46:09 2013        
      gen     7       
      mode    100444  
      size    206674  
      parent  4       
      links   1       
      pflags  40800000004
      
      Indirect blocks:
                     0 L1  0:64c00:800 0:5c14c00:800 4000L/400P F=2 B=7/7
                     0  L0 0:14c00:28000 20000L/20000P F=1 B=7/7
                 20000  L0 0:3cc00:28000 20000L/20000P F=1 B=7/7              
                 
                      segment [0000000000000000, 0000000000040000) size  256K
```
So, there are two blocks, one at offset `0x14c00` and the other at offset `0x3cc00`, both of them `0x28000` bytes. Of the `0x28000` bytes, `0x8000` is parity. The real data size is `0x20000`. The question is, where does this data physically reside, i.e., which disk(s), and where on those disks?

I wrote a small program that uses the code from the `vdev_raidz_map_alloc()` routine to tell me the mapping that is set up. Here it is:

```
<<< see main.cpp (old revisions)>>>
```

The program takes an offset, size, number of disks in the pool, and optionally the number of parity (1 for raidz, 2 for raidz2 and 3 for raidz3) and the sector size (shift), and outputs the location of the blocks on the underlying disks. Let's try it.

```
# gcc -m64 raidzdump.c -o raidzdump

# ./raidzdump 14c00 20000 5
cols = 5, firstdatacol = 1
1:4200:8000
2:4200:8000
3:4200:8000
4:4200:8000
0:4400:8000
```
The parity for the block is on disk1 (`/var/tmp/f1`), the first 32k of data on disk2 (`/var/tmp/f2`), the second on disk3, etc. We could use `zdb(1M)` to check this, except there is a bug (see [Bug #3659](https://www.illumos.org/issues/3659)). But the following works on older versions of illumos and on Solaris11.

```
# zdb -R rzpool 0.2:4200:8000:r
Found vdev: /var/tmp/f2
assertion failed for thread 0xfffffd7fff172a40, 
thread-id 1: vd->vdev_parent == (pio->io_vd ? pio->io_vd : pio->io_spa->spa_root_vdev), 
file ../../../uts/common/fs/zfs/zio.c, line 827
Abort (core dumped)
#
```
This says to go to vdev 2 (`/var/tmp/f2`, child of the root vdev 0), at location `0x4200`, and read `0x8000` (32k) of data and display it.

Since `zdb(1M)` is currently broken for this, let's try a different way. We'll add the `0x4200` to the end of the disk label of 4MB at the beginning of every disk to get an absolute byte offset within the disk, then we'll use dd to look at the data.

```
# mdb4200+400000=E    4211200$q#
# dd if=/var/tmp/f2 bs=1 iseek=4211200 count=32k
10th1st2nd3rd4th5th6th7th8th9thaAAAAAASAarhus...
```
And there is the first 32k of the words file. To get the next 32k,use the offset from the third line of output from the raidzdump utility, and so on. This gets more interesting with smaller blocks, but that is left as an exercise for the reader. For instance, a write of a 512-byte file will stripe across 2 disks, one for the data, the other for parity. Note that I have not tested with raidz2 or raidz3. I expect the first data column to be 2 and 3 respectively, but the code should work... You need to specify the parity (2 or 3) to raidzdump as an argument.

Have fun!



Post written by Mr. Max Bruning