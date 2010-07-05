/*
 *  This file is a part of spec2009 project.
 */

#ifndef __WD1793_H
#define __WD1793_H

#define WD17XX_WE			0x10
#define WD1793_RE			0x08
#define WD17XX_CS			0x04
#define WD17XX_A1			0x02
#define WD17XX_A0			0x01

#define WD17XX_DRQ			0x02
#define WD17XX_INTRQ		0x01

#define WD17XX_STAT_NOTRDY      0x80  // drive not ready
#define WD17XX_STAT_WP          0x40  // diskette is write protected
#define WD17XX_STAT_WRERR       0x20  // write error
#define WD17XX_STAT_RECTYPE		0x20
#define WD17XX_STAT_HLD         0x20  // head is on
#define WD17XX_STAT_NOTFOUND    0x10  // addr marker not found on track
#define WD17XX_STAT_SEEKERR		0x10
#define WD17XX_STAT_CRCERR      0x08
#define WD17XX_STAT_TRK0		0x04  // head on cyl 0
#define WD17XX_STAT_LOST        0x04  // data underrun
#define WD17XX_STAT_DRQ         0x02  // data request for read/write
#define WD17XX_STAT_INDEX       0x02  // set on index marker
#define WD17XX_STAT_BUSY        0x01  // operation in progress

#define CMD_RESTORE				0x00
#define CMD_SEEK				0x10
#define CMD_STEP				0x20
#define CMD_STEP_U				0x30
#define CMD_STEP_IN				0x40		// trk += 1
#define CMD_STEP_IN_U			0x50		// trk += 1
#define CMD_STEP_OUT			0x60		// trk -= 1
#define CMD_STEP_OUT_U			0x70		// trk -= 1
#define CMD_RD_SEC				0x80
#define CMD_RD_SEC_M			0x90
#define CMD_WR_SEC				0xa0
#define CMD_WR_SEC_M			0xb0
#define CMD_RD_AM				0xc0
#define CMD_RD_TRK				0xe0
#define CMD_WR_TRK				0xf0
#define CMD_INTERRUPT			0xd0

#define WDR_STAT			0x00
#define WDR_CMD				0x00
#define WDR_TRK				0x01
#define WDR_SEC				0x02
#define WDR_DATA			0x03

#define TYPEI_MASK			0x80
#define TYPEII_MASK			0x40
#define TYPEIV_MASK			0xf0
#define TYPEI				0x00
#define TYPEII				0x00
#define TYPEIII				0x40
#define TYPEIV				0xd0

#define TYPEI_STEP_MASK		0x60
#define TYPEI_STEPWD_MASK   0x40
#define TYPEI_STEPWD		0x40
#define TYPEI_SEEK_MASK     0x10
#define TYPEI_SEEK			0x10
#define TYPEI_SDIR_MASK		0x20

#define TYPEI_BIT_U			0x10
#define TYPEI_BIT_H			0x08
#define TYPEI_BIT_V			0x04
#define TYPEI_BIT_R			0x03

#define TYPEII_WR_MASK		0x20
#define TYPEII_WR			0x20
#define TYPEII_READ			0x00

#define TYPEII_BIT_M		0x10
#define TYPEII_BIT_S		0x08
#define TYPEII_BIT_E		0x04
#define TYPEII_BIT_C		0x02

#define TYPEIII_AM_MASK		0x20
#define TYPEIII_AM			0x00
#define TYPEIII_WR_MASK		0x10
#define TYPEIII_WR			0x10

#define TYPEIII_BIT_E		0x04

#define INTR_MASK_I0		0x01
#define INTR_MASK_I1		0x02
#define INTR_MASK_I2		0x04
#define INTR_MASK_I3		0x08
#define INTR_MASK			0x0f

void wd17xx_write(word addr, byte data);
byte wd17xx_read(word addr);
void wd17xx_dispatch(void);
void wd17xx_init(void);

#endif
