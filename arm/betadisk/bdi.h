/*
 *  This file is a part of spec2009 project.
 */

#ifndef __BETADSK_H
#define __BETADSK_H

#define BDI_STAT_INTRQ		0x80
#define BDI_STAT_DRQ		0x40

#define BDI_STAT_DRQ_BIT	6

#define BDI_SYS_DRV			0x03
#define BDI_SYS_SIDE		0x10	// 1 - zero side, 0 - first side
#define BDI_SYS_HLT_EN		0x08
#define BDI_SYS_RST			0x04	// transition to 0 then to 1 is caused wd93 reset
#define BDI_SYS_MFM			0x40

#define BDI_SYS_MSK			0x1F

#if FDC_PORTABLE

extern 
PROGMEM 
struct FDC bdi;

#endif

#endif

