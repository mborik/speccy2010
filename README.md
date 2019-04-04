# Speccy2010

- **Speccy2010** is FPGA development board built for the implementation of various gaming computers (but originally focused on **ZX Spectrum and its clones**).
- This project is **next iteration of the firmware** primarily aimed on implementation of various disk interfaces which was common in Central European region. [DivMMC](https://spectrumforeveryone.com/2017/04/history-esxdos-divmmc-divmmc-enjoy/) interface and objectively most advanced disk system [MB-02](http://www.benophetinternet.nl/hobby/mb02/) has been already implemeted in addition to build-it Betadisk interface.

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
  - **Scorpion** 256 modes
* Disk interfaces
  - **Betadisk** (real floppy disk controller emulation, supported TRD, SCL and FDI disk images)
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

## Toolchain:

- [Sourcery CodeBench Lite 2011.09-69 for ARM EABI](https://sourcery.mentor.com/GNUToolchain/release2032)
- [Quartus II Web Edition 13.0 SP1](http://dl.altera.com/13.0sp1/?edition=web&platform=windows&download_manager=direct)
