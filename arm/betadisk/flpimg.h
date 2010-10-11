/*
 *  This file is a part of spec2009 project.
 */

#ifndef __FLPIMG_H
#define __FLPIMG_H

#define FLOPPY_RPS			5
#define FLP_AM_CONSTANT		6

#define FLP_BUF_SIZE		16

#define FSZID_128			0
#define	FSZID_256			1
#define FSZID_512			2
#define FSZID_1024			3


#define STD_SEC_SIZE		256
#define STD_SEC_CNT			16
#define STD_TRK_CNT			80
#define STD_SIDE_CNT		2
#define STD_SEC_SZ_ID		FSZID_256
#define STD_TRK_SZ			(16 * 256)

#define FLP_FLAGS_RO		0x01

#define FLP_PRIV(p, t)      ((t *)p->priv)

#define FLP_FLAT_TRK_PTR(sd,t,sc,sdn,tn,scn) ((((dword)(sdn)) * ((dword)(tn)) + ((dword)(sd)))) * ((dword)scn) * ((dword)STD_SEC_SIZE)

enum {
	DISK_NONE = 0,
	DISK_TRD,
	DISK_FDI,
	DISK_SCL,
	DISK_DSK,
	DISK_TD0,
	DISK_UDI,
	DISK_LAST,
};

struct flp_image;

struct flp_ops {
	byte flags;

	int (*open)(struct flp_image *img, dword size);
	void (*close)(struct flp_image *img);

	int (*read)(byte *buf);	// return 1 on ok
	int (*write)(byte *buf);   // return 1 on ok

	int (*set_trk)(void);
	int (*set_sec)(void);
	int (*set_sec_id)(byte sec);
	void (*change)(void);
};

struct flp_image {
	char fname[8+1+3+1];
	FIL  fp;

	struct flp_ops PROGMEM_PTR * ops;

	byte flp_num;

	byte sec_cnt;
	byte trk_cnt;
	byte side_cnt;

	word fs_wp: 1;			// write-protect by filesystem
	word wp: 1;				// write-protect by user
	word img_wp: 1;
	word set_trk: 1;
	word no_data: 1;
	word hld: 1;
	word interleave: 1;
	word sec_done: 1;
	word stat;

	byte sec_n;
	byte trk_n;
	byte side_n;
	byte am_buf[6];

	dword trk_ptr;
	word trk_sz;

	word data_left;
	word byte_time;
	word am_time;
	word sector_time;
	word time;
	word rps;

	dword priv[ 64 / 4 ];
};

#define am_trk am_buf[0]
#define am_side am_buf[1]
#define am_sec am_buf[2]
#define am_size am_buf[3]
#define am_crc_hi am_buf[4]
#define am_crc_lo am_buf[5]

extern struct flp_image *sel_drv;

///////////////////////////////////////////////////////////////////////

static inline word op_size(void)
{
	word size = FLP_BUF_SIZE;
	if (sel_drv->data_left <= FLP_BUF_SIZE) {
		sel_drv->stat |= FLP_STAT_EOD;
		size = sel_drv->data_left;
	}
	sel_drv->data_left -= size;

	return size;
}

///////////////////////////////////////////////////////////////////////

extern struct flp_ops PROGMEM trd_ops;
extern struct flp_ops PROGMEM scl_ops;
extern struct flp_ops PROGMEM fdi_ops;
extern struct flp_ops PROGMEM odi_ops;

///////////////////////////////////////////////////////////////////////

static inline int read_file(FIL *f, byte *dst, int sz)
{
	UINT nr;
	if (f_read(f, (char *)dst, sz, &nr) != FR_OK || nr != sz) {
		return 0;
	}
	return (word)nr;
}

static inline int write_file(FIL *f, byte *dst, int sz)
{
#if !_FS_READONLY
	UINT nw;
	if (f_write(f, (const char *)dst, sz, &nw) != FR_OK || nw != sz) {
		return 0;
	}
	return (word)nw;
#else
	return 0;
#endif
}

static inline byte read_le_byte(FIL *f, byte *dst)
{
	return read_file(f, dst, 1);
}

static inline byte read_le_word(FIL *f, word *dst)
{
#if defined(AVR_CTRL)
	byte w[2];
	if ( !read_file(f, w, 2) )
	{
		return 0;
	}

	*dst = (((word)w[1]) << 8) | ((word)w[0]);
	return 2;
#else
	return read_file(f, (byte *)dst, sizeof(*dst));
#endif
}

static inline byte read_le_dword(FIL *f, dword *dst)
{
#if defined(AVR_CTRL)
	byte dw[4];
	if ( !read_file(f, dw, 4) )
	{
		return 0;
	}

	*dst = (((dword)dw[3]) << 24) | (((dword)dw[2]) << 16) | (((dword)dw[1]) << 8) | ((dword)dw[0]);
	return 4;
#else
	return read_file(f, (byte *)dst, sizeof(*dst));
#endif
}

///////////////////////////////////////////////////////////////////////

#endif
