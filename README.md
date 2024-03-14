Contents
========

This is effectively a copy-paste from
[a post written by Mr. Max Bruning]([https://www.tritondatacenter.com/blog/zfs-raidz-striping]), probably around 2013-03-26.

This is just a re-formatted, easier-to-read version of that great but awfully  formatted post.


I do can compile the attached `main.cpp` (I had to slightly modify it), and the resulting `raidzdump` executable produces the same result as in the original post, namely 

```
>> raidzdump ./raidzdump 14c00 20000 5
cols = 5, firstdatacol =1 
1:4200:8000
2:4200:8000
3:4200:8000
4:4200:8000
0:4400:8000
```
