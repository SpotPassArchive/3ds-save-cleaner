# 3ds-save-cleaner

Savegames on the Nintendo 3DS internally use a FAT-like file system. For most titles including system titles, the save data is stored in the following structure:

- A [DISA](https://www.3dbrew.org/wiki/DISA_and_DIFF) container with one (or two) partitions.
    - A [SAVE](https://www.3dbrew.org/wiki/Inner_Fat) image within the partition(s).

Titles using save data ask the `fs` module to create save data with a certain maximum size.

Here is where the problems begin: the OS does **not** zero out the data region during the creation of the SAVE image. What this means is that the memory contents of the kernel at the time of creation of the image will leak into the unused blocks of the data region. This data can include, but is not limited to:

- Random (garbage) data from the NAND
- Savegames of other system modules/applications, which have been found to include:
    - `act`: Nintendo Network ID information **including email addresses and hashed passwords!**
    - `cfg`: Various bits of system information, **including plaintext WiFi SSIDs and passwords!**
    - `ptm`
    - `fs`

Sharing SAVE images with anyone will unknowingly give them this information along with the save data that was the only data intended to be shared.

This tool is used to detect and erase (zero-out) these blocks of unused data, so as to prevent sensitive user information being present when sharing these files.
