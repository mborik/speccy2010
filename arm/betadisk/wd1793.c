/*
 *  This file is a part of spec2009 project.
 *
 *  wd1793 support
 *
 */

#include "sysinc.h"
#include "stdinc.h"
#include "floppy.h"
#include "wd1793.h"
#include "fdc.h"
#include "dskdebug.h"

#define WD_CLOCK	   		2
#define RESTORE_ON_RESET	1
#define WD_STEP_INTERVAL	100

#if WD_TRACE_FLAG
#define WD_TRACE(x)			TRACE(x)
#else
#define WD_TRACE(x)
#endif

#if WDH_TRACE_FLAG
#define WDH_TRACE(x)		TRACE(x)
#else
#define WDH_TRACE(x)
#endif


#define DATA_DELAY     		(30 / wd.clk)

#define WD_CLR_INT()		do { fdc_set(FDC_INTRQ, 0); } while (0)
#define WD_SET_INT()		do { fdc_set(FDC_INTRQ, 1); } while (0)
#define WD_CLR_DRQ()		do { fdc_set(FDC_DRQ, 0); } while (0)
#define WD_SET_DRQ()		do { fdc_set(FDC_DRQ, 1); } while (0)

#define WD_CLR_BUSY()		do { wd.str &= ~WD17XX_STAT_BUSY; wd.icnt = 0; } while (0)
#define WD_SET_BUSY()		do { wd.str |= WD17XX_STAT_BUSY; } while (0)

#define WD_DRQ()			(fdc_query(FDC_DRQ))
#define WD_BUSY()			((wd.str & WD17XX_STAT_BUSY) != 0)

#define WD_RESET()			(wd.state == wd_reset)

#define STATE(x)  			do { wd.state = wd_##x; } while (0)
#define ENTRY(x) 			state_##x: STATE(x)
#define JUMP(x) 			do { goto state_##x; } while (0)
#define ADDR(x) 			&&state_##x

#define SET_TIMER(x) 		do { wd.delta_time = (x) * (1000 / CNTR_INTERVAL); wd.start_time = get_ticks(); } while (0)
#define SET_TIMERU(x) 		do { wd.delta_time = (x) / CNTR_INTERVAL; wd.start_time = get_ticks(); } while (0)
#define SET_TIMER_DCNT(x)	do { wd.delta_time = floppy_byte_time() * x; wd.start_time = get_ticks(); } while (0)

#define TIMER_EXP() 		(( (get_ticks() - wd.start_time) ) >= wd.delta_time )
#define WAIT_TIMER() 		if (!TIMER_EXP()) WAIT()



enum wd_state {
	wd_t0 = 0,
	wd_t0_cmd,
	wd_t1,
	wd_t1_set,
	wd_t1_trkupd,
	wd_t1_trk0,
	wd_t1_step,
	wd_t1_waits,  	// delay according to r1,r0
	wd_t1_vrfy,
	wd_t1_waitd,	// has 15ms expired
	wd_t1_hlt,		// is hlt = 1
	wd_t1_rdam,  	// have 3 index holes passwd
	wd_t2,
	wd_t2_waitd,
	wd_t2_hlt,
	wd_t2_loop,
	wd_t2_amc,
	wd_t2_rd,
	wd_t2_rdblk,
	wd_t2_mchk,
	wd_t2_wr,
	wd_t3,
	wd_t3_wait,
	wd_t3_hlt,
	wd_t3_rdam,
	wd_t4,
	wd_reset,
	wd_done,
};

// wd1793 registers
struct wd17xx {
 	byte clk;

	// statuses
 	byte index: 1;
 	byte ready: 1;

	byte tr;
	byte sr;
	byte str;
	byte cr;
	byte cr_c;
 	byte dr;
 	byte dsr;
 	word dcnt;
 	byte icnt;
 	word crc;
 	// state
 	byte state;

	dword start_time;
	dword delta_time;
};

static struct wd17xx wd;

///////////////////////////////////////////////////////////////////////

static inline word wd_sect_size(word sz_id)
{
	return 0x80 << (sz_id & 3);
}

static inline byte wd_step_rate(byte rate_code)
{
	static byte rates[4] = {6, 12, 20, 30};

    if( fast_mode ) return 1;
	else return rates[rate_code & 3] / wd.clk;				// division
}

///////////////////////////////////////////////////////////////////////

static void wd_cmd_start(void)
{
	WD_CLR_DRQ();
	WD_CLR_INT();
	if ((wd.cr & TYPEIV_MASK) != TYPEIV) {
		WD_TRACE(("wd: set busy signal\n"));
		WD_SET_BUSY();
	}
	STATE(t0_cmd);
}

///////////////////////////////////////////////////////////////////////

static void wd_rst(void)
{
	WD_TRACE(("wd: performing reset\n"));
	wd.cr = 0;
#if RESTORE_ON_RESET
	WD_TRACE(("wd: performing restore\n"));
	wd_cmd_start();
#else
	fdc_set(FDC_STEP, 0);
	WD_CLR_DRQ();
	WD_CLR_INT();
	WD_TRACE(("wd: clr busy signal (#0)\n"));
	WD_CLR_BUSY();
	STATE(t0);
#endif
}

///////////////////////////////////////////////////////////////////////

static byte wd_floppy_read()
{
	wd.dsr = floppy_read();
	wd.crc = crc_add(wd.crc, wd.dsr);
	return wd.dsr;
}

static void wd_floppy_write()
{
	floppy_write(wd.dsr);
	wd.crc = crc_add(wd.crc, wd.dsr);
}

static void wd_crc_init(void)
{
	wd.crc = crc_init();
}

static byte wd_is_hld(void)
{
	return fdc_query(FDC_HLT) && fdc_query(FDC_HLD);
}

///////////////////////////////////////////////////////////////////////

#define WAIT() return
#define DONE() return

static byte wd_rd_am(byte *am)
{
    byte i;
    wd_crc_init();
    for (i = 0; i < 6; i++) {
		am[i] = wd_floppy_read();
    }
	if (wd.crc) {
		wd.str |= WD17XX_STAT_CRCERR;
		return 0;
	} else {
		wd.str &= ~WD17XX_STAT_CRCERR;
		return 1;
	}
}

#define JUMP_IF_SET(b,l)    do { if ((b) != 0) JUMP(l); } while(0)
#define JUMP_IF_CLEAR(b,l)  do { if ((b) == 0) JUMP(l); } while(0)
#define SET(b)				do { b = 1; } while (0)
#define CLEAR(b)			do { b = 0; } while (0)
#define LOAD(r)
#define STORE(r)

static void wd_proc(void)
{
	static byte am[6];
	byte i_met;
	word stat = 0;

	static /*PROGMEM */void * entries[] = {
		ADDR(t0),
		ADDR(t0_cmd),

		ADDR(t1), ADDR(t1_set), ADDR(t1_trkupd),
		ADDR(t1_trk0), ADDR(t1_step), ADDR(t1_waits),
		ADDR(t1_vrfy), ADDR(t1_waitd), ADDR(t1_hlt),
		ADDR(t1_rdam),

		ADDR(t2), ADDR(t2_waitd), ADDR(t2_hlt),
		ADDR(t2_loop), ADDR(t2_amc), ADDR(t2_rd),
		ADDR(t2_rdblk), ADDR(t2_mchk), ADDR(t2_wr),

		ADDR(t3), ADDR(t3_wait), ADDR(t3_hlt), ADDR(t3_rdam),

		ADDR(t4),

		ADDR(reset),

		ADDR(done)
	};

	goto *entries[wd.state];
	/***************/ ENTRY(t0); /***************/
	WAIT();
	/***************/ ENTRY(t0_cmd); /***************/
	WD_TRACE(("wd: t0_cmd: %s, cr=0x%02x\n", wd_command_decode(wd.cr), wd.cr));

	if ((wd.cr & TYPEIV_MASK) == TYPEIV) {
        BDI_ResetWrite();
        BDI_ResetRead( 0 );

        if ((wd.cr & INTR_MASK) != 0)
        {
            wd.ready = fdc_query(FDC_READY);
            SET_TIMERU( 64 );
            JUMP(t4);
        }

		if (!WD_BUSY())
		{
			wd.cr_c = 0;
		}
		WD_TRACE(("wd: clr busy signal (#1)\n"));
		WD_CLR_BUSY();
		JUMP(t0);
	} else if ((wd.cr & TYPEI_MASK) == TYPEI) {
		wd.cr_c = wd.cr;
  		wd.str &= ~(WD17XX_STAT_CRCERR|WD17XX_STAT_NOTFOUND);
  		fdc_set(FDC_HLD, (wd.cr_c & TYPEI_BIT_H) != 0);
   		JUMP(t1);
	} else {
		wd.cr_c = wd.cr;
		wd.str &= ~(WD17XX_STAT_RECTYPE|WD17XX_STAT_WRERR|WD17XX_STAT_CRCERR|WD17XX_STAT_LOST|WD17XX_STAT_WP);

		if (!fdc_query(FDC_READY)) {
			JUMP(done);
		}

		fdc_set(FDC_HLD, 1);

		if ((wd.cr_c & TYPEII_MASK) == TYPEII) {
			JUMP(t2);
		} else {
			if ((wd.cr_c & TYPEIII_AM_MASK) != TYPEIII_AM) {
				wd.str |= WD17XX_STAT_WP;
				JUMP(done);
			}
			JUMP(t3);
		}
	}
	WAIT();
	//=========================================================
	// type 1
	//=========================================================
	/***************/ ENTRY(t1); /***************/
	WD_TRACE(("wd: t1: dr=%x, hld=%u\n", wd.dr, fdc_query(FDC_HLD)));

	if ((wd.cr_c & TYPEI_STEP_MASK) != 0) {
		if ((wd.cr_c & TYPEI_STEPWD_MASK) == TYPEI_STEPWD) {
			fdc_set(FDC_SDIR, (wd.cr_c & TYPEI_SDIR_MASK) != 0);
		}
		if ((wd.cr_c & TYPEI_BIT_U)) {
			JUMP(t1_trkupd);
		} else {
			JUMP(t1_step);
		}
	} else {
		if ((wd.cr_c & TYPEI_SEEK_MASK) != TYPEI_SEEK) {
			// restore
			wd.tr = 0xff;
			wd.dr = 0;
		}
	}
	/***************/ ENTRY(t1_set); /***************/
	WD_TRACE(("wd: t1_set, time=%s\n", ticks_str(get_ticks())));

	wd.dsr = wd.dr;
	if (wd.dsr == wd.tr) {
		JUMP(t1_vrfy);
	}
	fdc_set(FDC_SDIR, wd.dsr > wd.tr);
	/***************/ ENTRY(t1_trkupd); /***************/
	WD_TRACE(("wd: t1_trkupd, sdir=%u, tr=%u, time=%s\n", fdc_query(FDC_SDIR), wd.tr, ticks_str(get_ticks())));

	if (fdc_query(FDC_SDIR)) {
		wd.tr += 1;
	} else {
		wd.tr -= 1;
	}
	/***************/ ENTRY(t1_trk0); /***************/
	WD_TRACE(("wd: t1_trk0, sdir=%u, time=%s\n", fdc_query(FDC_SDIR), ticks_str(get_ticks())));

	if (!fdc_query(FDC_TRK0) && !fdc_query(FDC_SDIR)) {
		WD_TRACE(("wd: t1_trk0: trk0\n"));
		wd.tr = 0;
		JUMP(t1_vrfy);
	} else {
		fdc_set(FDC_STEP, 1);
		SET_TIMERU(WD_STEP_INTERVAL);
	}
	/***************/ ENTRY(t1_step); /***************/
	WDH_TRACE(("wd: t1_step time=%s\n", ticks_str(get_ticks())));
	WAIT_TIMER();

	fdc_set(FDC_STEP, 0);

	SET_TIMER(wd_step_rate(wd.cr_c & TYPEI_BIT_R));
	/***************/ ENTRY(t1_waits); /***************/
	WDH_TRACE(("wd: t1_waits time=%s\n", ticks_str(get_ticks())));

    // delay according to r1,r0 field
    WAIT_TIMER();

	if ((wd.cr_c & 0xe0) == 0) {
		JUMP(t1_set);
	}
	/***************/ ENTRY(t1_vrfy); /***************/
	WD_TRACE(("wd: t1_vrfy time=%s\n", ticks_str(get_ticks())));

    // is V = 1?
	if ((wd.cr_c & TYPEI_BIT_V) == 0) {
		JUMP(done);
	}
	// set hld
	fdc_set(FDC_HLD, 1);
	// setup delay
	SET_TIMER(DATA_DELAY);
	/***************/ ENTRY(t1_waitd); /***************/
	WDH_TRACE(("wd: t1_waitd time=%s\n", ticks_str(get_ticks())));

	// has 15 ms expired
    WAIT_TIMER();
	/***************/ ENTRY(t1_hlt); /***************/
	WD_TRACE(("wd: t1_hlt time=%s\n", ticks_str(get_ticks())));

	// is hlt = 1
    if (!wd_is_hld()) {
		WAIT();
    }
	wd.icnt = 0;
	/***************/ ENTRY(t1_rdam); /***************/
	WD_TRACE(("wd: t1_rdam, icnt=%u, time=%s\n", wd.icnt, ticks_str(get_ticks())));

	if (wd.icnt >= 6) {
		wd.str |= WD17XX_STAT_SEEKERR;
		JUMP(done);
	}
	stat = floppy_stat();
	if ((stat & FLP_STAT_AM) == 0) {
		WAIT();
	}
	if (wd_rd_am(am)) {
		if (wd.tr == am[0]) {
			JUMP(done);
		}
	}
	WAIT();
	//=========================================================
	// type 2
	//=========================================================
	/***************/ ENTRY(t2); /***************/
	WD_TRACE(("wd: t2 tr=%u, sr=%u, side=%u, cr=%x, time=%s\n", wd.tr, wd.sr, wd.cr_c & TYPEII_BIT_S ? 1 : 0, wd.cr_c, ticks_str(get_ticks())));

	floppy_set_sec_id(wd.sr);

	if ((wd.cr_c & TYPEII_BIT_E) == 0) {
		JUMP(t2_hlt);
	}
	SET_TIMER(DATA_DELAY);
	/***************/ ENTRY(t2_waitd); /***************/
	WDH_TRACE(("wd: t2_waitd time=%s\n", ticks_str(get_ticks())));

    WAIT_TIMER();
	/***************/ ENTRY(t2_hlt); /***************/
	WD_TRACE(("wd: t2_hlt time=%s\n", ticks_str(get_ticks())));

    if (!wd_is_hld()) {
		WAIT();
    }
    // TG43
	/***************/ ENTRY(t2_loop); /***************/
	WD_TRACE(("wd: t2_loop time=%s\n", ticks_str(get_ticks())));

	if ((wd.cr_c & TYPEII_WR_MASK) == TYPEII_WR) {
		if (fdc_query(FDC_WP)) {
			WD_TRACE(("wd: t2_loop disk is wp\n"));
			wd.str |= WD17XX_STAT_WP;
			JUMP(done);
		}
	}
	wd.icnt = 0;
	/***************/ ENTRY(t2_amc); /***************/

	if (wd.icnt >= 6) {
		wd.str |= WD17XX_STAT_NOTFOUND;
		JUMP(done);
	}
	stat = floppy_stat();
	if ((stat & FLP_STAT_AM) == 0) {
		WAIT();
	}
	if (!wd_rd_am(am)) {
		JUMP(done);
	}
	WD_TRACE(("wd: t2_amc, sz=%u (trk,side,sec) dsk(%u,%u,%u) wd(%u,%u,%u) time=%s\n", am[3], am[0], am[1], am[2], wd.tr, wd.cr_c & TYPEII_BIT_S ? 1 : 0, wd.sr, ticks_str(get_ticks())));
	if (wd.tr != am[0]) {
		WAIT();
	}
	if (wd.sr != am[2]) {
		WAIT();
	}

	// 1795 - other operation
	if ( (wd.cr_c & TYPEII_BIT_C) != 0 )
	{
		byte side = ( wd.cr_c & TYPEII_BIT_S ) != 0 ? 0 : 1;
		if ( side == am[1] )
		{
			WAIT();
		}
	}

	wd.dcnt = wd_sect_size(am[3]);
    BDI_ResetWrite();
    SET_TIMER_DCNT( wd.dcnt );

	if ((wd.cr_c & TYPEII_WR_MASK) == TYPEII_WR)
	{
	    BDI_ResetRead( wd.dcnt - 1 );

		WD_TRACE(("wd: t2_amc, SET_DRQ() time=%s\n", ticks_str(get_ticks())));
		WD_SET_DRQ();
		JUMP(t2_wr);
	}
	/***************/ ENTRY(t2_rd); /***************/
	WD_TRACE(("wd: t2_rd time=%s\n", ticks_str(get_ticks())));

	stat = floppy_stat();
	if ((stat & FLP_STAT_AM) != 0) {
		wd.str |= WD17XX_STAT_NOTFOUND;
		JUMP(done);
	}

	// put record type in status reg bit 5
	if ((stat & FLP_STAT_DEL) != 0) {
		wd.str |= WD17XX_STAT_RECTYPE;  // 1 = deleted record, 0 - normal record
	} else {
		wd.str &= ~WD17XX_STAT_RECTYPE; // 1 = deleted record, 0 - normal record
	}

	/***************/ ENTRY(t2_rdblk); /***************/
	WDH_TRACE(("wd: t2_rdblk dcnt=%u, time=%s\n", wd.dcnt, ticks_str(get_ticks())));

    while( wd.dcnt > 1 )
    {
        wd_floppy_read();
        BDI_Write( wd.dsr );
        wd.dcnt--;
    }

	// has first byte been assembled in dsr
	if (!WD_DRQ())
	{
	t2_rd_common:
		if (wd.dcnt == 0) {
			stat = floppy_stat();
			if ((stat & FLP_STAT_ERR) != 0 || (stat & FLP_STAT_EOD) == 0) {
				wd.str |= WD17XX_STAT_CRCERR;
				JUMP(done);
			}
			JUMP(t2_mchk);
		}
		wd_floppy_read();
		wd.dr = wd.dsr;
		wd.dcnt -= 1;
		WD_SET_DRQ();
	} else {
		if (TIMER_EXP()) {
			WD_TRACE(("wd: t2_rdblk - set LOST, time=%s (byte time=%s, drq time=%s)\n", ticks_str(get_ticks()), ticks_str1(wd.delta_time), ticks_str2(wd.start_time)));
			//__TRACE( "wd: t2_rdblk - set LOST\n" );
			wd.str |= WD17XX_STAT_LOST;
			BDI_ResetWrite();
			goto t2_rd_common;
		}
	}

	WAIT();
	/***************/ ENTRY(t2_mchk); /***************/
	WD_TRACE(("wd: t2_mchk wdstat=%x, flp_stat=%x, time=%s\n", wd.str, stat, ticks_str(get_ticks())));

	if ((wd.cr_c & TYPEII_BIT_M) != 0) {
		wd.sr += 1;
		JUMP(t2_loop);
	}
	WD_TRACE(("wd: end of read\n"));
	JUMP(done);
	/***************/ ENTRY(t2_wr); /***************/
	WDH_TRACE(("wd: t2_wr dcnt=%u, time=%s\n", wd.dcnt, ticks_str(get_ticks())));

    while( BDI_Read( &wd.dsr ) )
    {
        wd_floppy_write();
        wd.dcnt--;
    }

	if (!WD_DRQ())
	{
		wd.dsr = wd.dr;
    t2_wr_common:
		wd_floppy_write();
		wd.dcnt -= 1;
		if (wd.dcnt == 0) {
			stat = floppy_stat();
			if ((stat & FLP_STAT_ERR) != 0 || (stat & FLP_STAT_EOD) == 0) {
				wd.str |= WD17XX_STAT_WRERR;
				JUMP(done);
			}
			JUMP(t2_mchk);
		}
		WD_SET_DRQ();
	} else {
		if (TIMER_EXP()) {
			WD_TRACE(("wd: t2_wr - set LOST, time=%s (byte time=%s, drq time=%s)\n", ticks_str(get_ticks()), ticks_str1(wd.delta_time), ticks_str2(wd.start_time)));
		    //__TRACE( "wd: t2_wr - set LOST\n" );
		    BDI_ResetRead(0);

			wd.str |= WD17XX_STAT_LOST;
			wd.dsr = 0;
			goto t2_wr_common;
		}
	}
	WAIT();
	//=========================================================
	// type 3
	//=========================================================
	/***************/ ENTRY(t3); /***************/
	WD_TRACE(("wd: t3 time=%s\n", ticks_str(get_ticks())));

	if ((wd.cr_c & TYPEIII_BIT_E) == 0) {
		JUMP(t3_hlt);
	}
	SET_TIMER(DATA_DELAY);
	/***************/ ENTRY(t3_wait); /***************/
	WDH_TRACE(("wd: t3_wait time=%s\n", ticks_str(get_ticks())));

    WAIT_TIMER();
	/***************/ ENTRY(t3_hlt); /***************/
	WD_TRACE(("wd: t3_hlt time=%s\n", ticks_str(get_ticks())));

    if (!wd_is_hld()) {
		WAIT();
    }
    wd.icnt = 0;
    wd.dcnt = 6;
    wd_crc_init();
    BDI_ResetWrite();

	/***************/ ENTRY(t3_rdam); /***************/
	WDH_TRACE(("wd: t3_rdam time=%s\n", ticks_str(get_ticks())));

	if (wd.icnt >= 6) {
		wd.str |= WD17XX_STAT_NOTFOUND;
		JUMP(done);
	}

	if (wd.dcnt == 6) {
		stat = floppy_stat();
		if ((stat & FLP_STAT_AM) == 0) {
			WAIT();
		}
	}

    while( wd.dcnt > 1 )
    {
        wd_floppy_read();
        BDI_Write( wd.dsr );
        wd.dcnt--;
    }

	if (!WD_DRQ()) {
		if (wd.dcnt == 0) {
			WD_TRACE(("wd: rd_am, crc=0x%04x\n", wd.crc));
			WD_TRACE(("wd: rd_am (%u,%u,%u,%u), crc=0x%02x%02x\n", am[5], am[4], am[3], am[2], am[1], am[0]));
			if (wd.crc != 0) {
				wd.str |= WD17XX_STAT_CRCERR;
			}
			JUMP(done);
		}

		wd_floppy_read();
		wd.dr = wd.dsr;
		wd.dcnt -= 1;
#if defined(DEBUG) && DEBUG
		am[wd.dcnt] = wd.dsr;
#endif
		if (wd.dcnt == 5) {
			wd.sr = wd.dsr;
		}
		WD_SET_DRQ();
	}
	WAIT();
	//=========================================================
	// type 4
	//=========================================================
	/***************/ ENTRY(t4); /***************/

    WAIT_TIMER();

	i_met = INTR_MASK_I3;
	if (wd.ready != fdc_query(FDC_READY)) {
		i_met |= wd.ready ? INTR_MASK_I1 : INTR_MASK_I0;
		wd.ready = !wd.ready;
	}
	if (wd.index) {
		i_met |= INTR_MASK_I2;
	}
	if ((i_met & wd.cr & INTR_MASK) == 0) {
		WAIT();
	}
	JUMP(done);
	/***************/ ENTRY(reset); /***************/
	WAIT();
	/***************/ ENTRY(done); /***************/
	fdc_set(FDC_STEP, 0);
	WD_TRACE(("wd: clr busy signal (#2)\n\n"));
	WD_CLR_BUSY();
	//WD_CLR_DRQ();
	WD_SET_INT();
	STATE(t0);
}

///////////////////////////////////////////////////////////////////////

static inline void wd_update_stat(void)
{
	byte str = wd.str;

	if ((wd.cr_c & TYPEI_MASK) == TYPEI) {
		str &= ~(WD17XX_STAT_INDEX|WD17XX_STAT_TRK0|WD17XX_STAT_HLD|WD17XX_STAT_WP);
		if (!fdc_query(FDC_INDEX)) str |= WD17XX_STAT_INDEX;
		if (!fdc_query(FDC_TRK0)) str |= WD17XX_STAT_TRK0;
		if (wd_is_hld()) str |= WD17XX_STAT_HLD;
		if (fdc_query(FDC_WP)) str |= WD17XX_STAT_WP;
	} else {
		str = WD_DRQ() ? (str | WD17XX_STAT_DRQ) : (str & ~WD17XX_STAT_DRQ);
	}
	str = (fdc_query(FDC_READY) && fdc_query(FDC_RESET)) ? (str & ~WD17XX_STAT_NOTRDY) : (str | WD17XX_STAT_NOTRDY);
	wd.str = str;
}

static inline void wd_clr_stat(void)
{
	byte clr;
	clr = WD17XX_STAT_SEEKERR;
	if ((wd.cr_c & TYPEI_MASK) != TYPEI) {
		clr |= WD17XX_STAT_RECTYPE|WD17XX_STAT_WP|WD17XX_STAT_CRCERR|WD17XX_STAT_LOST;
	}
	wd.str &= ~clr;
}

///////////////////////////////////////////////////////////////////////

void wd17xx_write(word addr, byte data)
{
	if (WD_RESET()) {
		return;
	}
	switch (addr & (WD17XX_A1 | WD17XX_A0)) {
		case WDR_CMD:
			if ((data & TYPEIV_MASK) == TYPEIV || !WD_BUSY()) {
				wd.cr = data;
				wd_cmd_start();
			}
			break;
		case WDR_TRK:
			wd.tr = data;
			break;
		case WDR_SEC:
			wd.sr = data;
			break;
		case WDR_DATA:
			wd.dr = data;
			WD_CLR_DRQ();
			WDH_TRACE(("wd: wr_data, clr_drq, dcnt=%u\n", wd.dcnt));
			break;
	}
}

byte wd17xx_read(word addr)
{
    byte data = 0xff;
	if (!WD_RESET()) {
		switch (addr & (WD17XX_A1 | WD17XX_A0)) {
			case WDR_STAT:
				WD_CLR_INT();
				wd_update_stat();
				data = wd.str;
				wd_clr_stat();
				if ((wd.cr_c & TYPEI_MASK) != TYPEI) {
					WD_TRACE(("wd: read stat %s\n", wd_stat_decode(data, wd.cr_c)));
				}
				break;
			case WDR_TRK:
				data = wd.tr;
				break;
			case WDR_SEC:
				data = wd.sr;
				break;
			case WDR_DATA:
				WD_CLR_DRQ();
				data = wd.dr;
				WDH_TRACE(("wd: rd_data, clr_drq, dcnt=%u, data=0x%02x\n", wd.dcnt, wd.dr));
				break;
		}
	}
	return data;
}

///////////////////////////////////////////////////////////////////////

void wd17xx_dispatch(void)
{
	if (fdc_query(FDC_RESET)) {
		if (WD_RESET()) {
			wd_rst();
		}

		if (wd.index != fdc_query(FDC_INDEX)) {
			wd.index = !wd.index;
			if (wd.index) {
				wd.icnt += 1;
			}
		}

		wd_proc();
	} else {
		STATE(reset);
	}

//	WD_TRACE(("wd: icnt=%u\n", wd.icnt));
	if (!WD_BUSY()) {
		if (wd.icnt >= 15 && fdc_query(FDC_HLD)) {
			WD_TRACE(("wd: hld_off icnt=%u hld=%u ready=%u\n", wd.icnt, fdc_query(FDC_HLD), wd.ready != 0));
			fdc_set(FDC_HLD, 0);		// disengage motor
		}
	}
}

///////////////////////////////////////////////////////////////////////

void wd17xx_init(void)
{
	wd.clk = WD_CLOCK;

	STATE(reset);
}

///////////////////////////////////////////////////////////////////////

