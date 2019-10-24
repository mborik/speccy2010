# BS-DOS and BS-ROM modified a customized for Speccy2010
- sources are adapted and formatted for [SjASMPlus](https://github.com/z00m128/sjasmplus)
- grabbed from [z00m128/mb-02plus](https://github.com/z00m128/mb-02plus) repository

## bsdos308s
- original BS-DOS 308 © Busy Software 1992-1996 _(released 1996-07-28)_
- FDC BIOS replaced by custom Speccy2010 BIOS communicating bidirectionally on port #53 with Speccy2010 firmware
- integrated `dos_26znak` patch displaying full disk and directory name in catalog
- Sinclair BASIC color set (`#38`)

## bsrom140s
- modified Sinclair ZX Spectrum ROM [© Amstrad](Amstrad.license.md)
- read the full description of modifications [here](https://z00m.speccy.cz/files/BSROM140-en.txt)
- included enhancements needed for BS-DOS
- added silencer for 2xAY in extended 128k reset and NMI menu (by z00m)
- integrated `dirpatch` replacing `COPY` token by `DIR`
- Sinclair BASIC color set (`#38`)
