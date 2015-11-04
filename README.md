# png_undelete
A small C program to undelete PNG files

# Overview

This program will seek through the file at the path specified
in search of PNG file signatures. When run on a block device such as 
'/dev/sda' under Linux, this has the effect of searching through the
entire device, pulling out any PNG file it finds (deleted or not)

It works with any file argument. To test it, you can run something like:

```
cat file1.png file2.png file3.png > mashup.pngs

png_undelete mashup.pngs
```

When completed a directory called "recovered_pngs" is created, containing
files 1.png, 2.png, and 3.png. These should have the same contents 
as the originals that were concatenated.

# Usage

To comile, run make in the project directory.

To undelete PNGs from block device 'sda', run the following:

```
sudo ./png_undelete /dev/sda
```

Any PNG files it finds will be outputted to a new directory 
"recovered_pngs" from where you ran the program.

# Note

Ideally, you would want to run this program from a directory 
on another device to prevent files that are being recovered from 
overwriting other PNG files on the disk.

# Todo

-Portability improvements (-std=gnu11)

-Add a few command line options

-More make options (clean, install...)
