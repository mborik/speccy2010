# Speccy2010

- **Speccy2010** is FPGA development board built for the implementation of various gaming computers (but originally focused on **ZX Spectrum and its clones**).
- This project is **next iteration of the firmware** primarily aimed on implementation of various disk interfaces which was common in Central European region. [DivMMC](https://spectrumforeveryone.com/2017/04/history-esxdos-divmmc-divmmc-enjoy/) interface and objectively most advanced disk system [MB-02](https://z00m.speccy.cz/?file=mb-02) has been already implemeted in addition to build-it Betadisk interface.
- It brings you an improved **File Manager** which look like old good DOS commanders and let you to autoload snapshots, TAPs or disk images into the current machine configuration. Provides enhanced capabilities when the [ESXDOS](http://www.esxdos.org) is loaded into DivMMC to autoload TAP and TRD files directly, without accessing NMI menu.
- It also includes a **full-featured debugger** built over the emulation core.
- More info how to control this Speccy2010 firmware you can find [in wiki](https://github.com/mborik/speccy2010/wiki).

### Harware Key Features:
* FPGA: EP2C8Q208C8N
* Microcontroller: STR755FV2T6 (or STR750FV2T6)
* Memory: SDRAM 16 Mb K4S281632J-UC75 (or 32 Mb K4S561632J-UC75)
* RTC: DS1338Z-33
* Slot for SD card
* 2x PS/2 port (keyboard, mouse)
* 2x Canon 9-pin port for joysticks
* USB type B - virtual com for debugging and programming the microcontroller
* Upgrading of firmware for microcontroller and FPGA is done automatically from the root directory of the SD card
* Video: three R-2R matrixes, 8 bits each (output to VGA, Composite, S-Video)
* Audio TDA1543 or two R-2R matrixes, 8 bits each
* Power supply 5V/1A

### ZX Spectrum implementation features:
* Basic models and clones (including timing):
  - **ZX Spectrum 48k**
  - **ZX Spectrum 128k**
  - **Pentagon** 128k/512k/1024k
  - **Scorpion** 256k
* Disk interfaces
  - **Betadisk** (simple floppy disk controller emulation, supported TRD, SCL and FDI disk images)
  - **DivMMC** (basically [DivIDE](https://divide.speccy.cz/files/pgm_model.txt) control register and memory model with all 64 x 8kB SRAM pages)
  - **MB-02** (without Z80DMA or real floppy disk controller but specifically modified original BIOS and simplified data transfer with new BS-DOS 308s firmware)
* 2xAY (Turbosound), Covox/Soundrive
* TAP / TZX support
* full SNA support
* S-Video and Composite output
* VGA output (50, 60 or 75 Hz)
* Turbo (7, 14 and 28 MHz)
* Kempston and sinclair joysticks
* Kempston mouse
* Real-Time Clock (read-only)
  - [Gluk specification](http://bit.do/glukrtc)
  - MB-02 specification (ports `#0n03`, n = 0..F)

## Acknowledgements:
* **Syd** - thank you for bringing this project to life!
* **poopisan** & **axc** - former improvements into FPGA core
* **z00m** & **ikon** - undying support, beta-testing and production of a new batches
* **RomBor** - support while my first steps in VHDL when DivMMC has been implemented
* **Busy** - intensive support while MB-02 implementation and customizing BS-DOS
* **ub880d** - improvements, suggestions and support while ESXDOS autoloader and RTC implementation
* **lordcoxis** - ESXDOS support and access to API documentation
* **Akio** - supply of stylish bold 6x8 font
* **azesmbog** - snapshot testing and timing suggestions
* **Sorgelig** - inspiration from his ULA implementation and timings


## Toolchain:

- [Sourcery CodeBench Lite 2011.09-69 for ARM EABI](https://sourcery.mentor.com/GNUToolchain/release2032)
- [Quartus II Web Edition 13.0 SP1](http://dl.altera.com/13.0sp1/?edition=web&platform=windows&download_manager=direct)
- [SjASMPlus](https://github.com/z00m128/sjasmplus)
