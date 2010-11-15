#include <stdarg.h>
#include <ctype.h>

#include "system.h"
#include "crc16.h"
#include "specBetadisk.h"

#define BOOT_FILE "boot.dsk"

#define RDCRC_TYPEI        0

#define DRAW_SEEK(x)

#define TRACE(x)
//#define TRACE(x) __TRACE x

#define SYS_READS_BEFORE_LOST 64

#define BETA_PORT_CMD      0x1f
#define BETA_PORT_STA      0x1f
#define BETA_PORT_CYL      0x3f
#define BETA_PORT_SEC      0x5f
#define BETA_PORT_DTA      0x7f
#define BETA_PORT_SYS      0xff
#define BETA_PORT_SYS_MSK  0x80

#define SYS_STAT_MSK       0xc0  // status mask
#define SYS_INTRQ          0x80  // set when wd1793 is completed command
#define SYS_DRQ            0x40  // set when wd1793 is waiting for data
#define SYS_DRV_MSK        0x03  // drive selector

#define SYS_SIDE           0x10  // 1 - zero side, 0 - first side
#define SYS_HLD_EN         0x08
#define SYS_RST            0x04  // transition to 0 then to 1 is caused wd93 reset

#define STAT_NOTRDY        0x80  // drive not ready
#define STAT_WP            0x40  // diskette is write protected
#define STAT_WRERR         0x20  // write error
#define STAT_RECT          0x20
#define STAT_HEAD_ON       0x20  // head is on
#define STAT_NOTFOUND      0x10  // addr marker not found on track
#define STAT_RDCRC         0x08
#define STAT_HEAD_CYL_0    0x04  // head on cyl 0
#define STAT_LOST          0x04  // data underrun
#define STAT_DRQ           0x02  // data request for read/write
#define STAT_INDEX         0x02  // set on index marker
#define STAT_BUSY          0x01  // operation in progress


#define CMD_RESTORE        0x00
#define CMD_SEEK           0x10
#define CMD_STEP_H0        0x20
#define CMD_STEP_H1        0x30
#define CMD_STEP_OUT_H0    0x40
#define CMD_STEP_OUT_H1    0x50
#define CMD_STEP_IN_H0     0x60
#define CMD_STEP_IN_H1     0x70
#define CMD_RD_SEC         0x80
#define CMD_RD_SEC_M       0x90
#define CMD_WR_SEC         0xa0
#define CMD_WR_SEC_M       0xb0
#define CMD_RD_AM          0xc0
#define CMD_RD_TRK         0xe0
#define CMD_WR_TRK         0xf0
#define CMD_INTERRUPT      0xd0

#define TYPEI_CMD_1        0
#define TYPEI_CMD_2        1
#define TYPEII_CMD         2
#define TYPEIII_CMD        3

#define TYPEI_BIT_U        0x10
#define TYPEI_BIT_H        0x08
#define TYPEI_BIT_V        0x04

#define TYPEII_BIT_M       0x10
#define TYPEII_BIT_S       0x08
#define TYPEII_BIT_E       0x04
#define TYPEII_BIT_C       0x02

#define TYPEIII_BIT_E      0x04

#if RDCRC_TYPEI
	#define CLEAR_STAT()       do { beta_state.stat_val &= ~(STAT_DRQ|STAT_LOST|STAT_NOTFOUND|STAT_WP|STAT_WRERR|STAT_BUSY|STAT_NOTRDY); } while (0)
#else
	#define CLEAR_STAT()       do { beta_state.stat_val = 0; } while (0);
#endif

#define SET_STAT_BUSY()    do { beta_state.stat_val |= (STAT_DRQ|STAT_BUSY); } while (0)
#define SET_STAT_READY()   do { beta_state.stat_val &= ~(STAT_DRQ|STAT_BUSY); } while (0)
#define SET_SYS_READY()    do { beta_state.sys_stat = SYS_INTRQ; } while (0)
#define SET_SYS_BUSY()     do { beta_state.sys_stat = SYS_DRQ;  } while (0)
#define SET_SYS_ACK()      do { beta_state.sys_stat |= SYS_INTRQ; } while (0)

#define SECSIZE_128        0
#define SECSIZE_256        1
#define SECSIZE_512        2
#define SECSIZE_1024       3

#define TRD_SEC_PER_CYL    16
#define TRD_SEC_SIZE       256
#define TRD_CYL_CNT        80
#define TRD_SIDE_CNT       2
#define TRD_SEC_SZ_ID      1

#define SYS_SEC_AREA       0xe0
#define FDSC_SIZE          16

struct sys_sec_area
{
	byte unused_0;
	byte first_sec;
	byte first_trk;
	byte dtype;
	byte nfiles;
	byte free_sec_lo;
	byte free_sec_hi;
	byte trdos;
	byte unused_1[2];
	byte unused_2[9];
	byte unused_3[1];
	byte erased;
	byte label[8];
	byte unused_4[3];
} __attribute__(( packed ) );

const sys_sec_area scl_sys_sec_def =
{
	0, 0, 0, 0, 0, 0, 0, 0, { 0, 0 }, { 32, 32, 32, 32, 32, 32, 32, 32, 32 }, { 0 }, 0, { 32, 32, 32, 32, 32, 32, 32, 32 }, { 0, 0, 0 }
};

enum
{
	DISK_NONE = 0,
	DISK_TRD,
	DISK_FDI,
	DISK_SCL,
	DISK_LAST,
};

struct img_fmt
{
	byte fmt;
	char ext[4];
};

#define NUM_DSK_FORMATS 3

const img_fmt img_fmt_def[NUM_DSK_FORMATS] =
{
	{ DISK_TRD, { "trd" } },
	{ DISK_SCL | 0x80, { "scl" } },
	{ DISK_FDI, { "fdi" } }
};



#define SEC_BUF_SIZE       16
// do not set next define to size that less 32,
// because for scl images buffer contains trdos
// disk system info
#define SEC_CACHE_SIZE     32
#define SCL_SYS_AREA_SIZE  32

#define SCL_MAX_FILES      128

#define scl_data           w_priv[0]
#define scl_last_sec       w_priv[1]
#define scl_sys_sec        u_priv_buf_1.b_buf
#define scl_cat_sec        u_priv_buf_2.w_buf
#define scl_numblk         w_priv[2]

// fdi data-block offset
#define fdi_data           w_priv[0]
// fdi track-descriptors offset
#define fdi_trk            w_priv[1]
// fdi current track descriptor offset
#define fdi_trk_head       w_priv[2]
#define fdi_sec_id         u_priv_buf_1.b_buf
#define fdi_sec_sz         u_priv_buf_2.b_buf

struct beta_drive
{
	//
	FIL  fd;
	//
unsigned char head_on :
	1;
unsigned char wp:
	1;
unsigned char end_of_sec:
	1;

	byte img_fmt;
	byte sec_cnt;
	byte cyl_cnt;
	byte side_cnt;
	word sec_size;
	byte sec_sz_id;    // 0 - 128, 1 - 256, 2 - 512, 3 - 1024
	byte cur_cyl;
	byte cur_sec;
	//
	byte cur_sec_id;
	byte sec_res;      // sector crc status
	//
	dword img_trkoff;  // offset of the track from file begin
	word sec_off;     // offset from begin of track
	word next_sec_off;
	//
	// extended image format private area
	word w_priv[3];
//  byte b_priv[1];
	union
	{
		byte b_buf[SEC_CACHE_SIZE];
		word w_buf[1];
	} u_priv_buf_1;
	union
	{
		byte b_buf[SEC_CACHE_SIZE / 2];
		word w_buf[1];
	} u_priv_buf_2;
	//
	byte sec_buff[SEC_BUF_SIZE];
	byte sec_buff_pos;
	byte sec_buff_fill;
};

struct beta_drive drives[BETADSK_NUM_DRIVES];
struct beta_drive *cur_drv;

// betadisk & wd1793 registers
struct beta_state
{
	byte sys_val;
	byte sys_stat;
	byte cmd_reg;
	int step_dir;
	int cyl_reg;
	byte sec_reg;
	byte stat_val;
	byte data_reg;
	byte cur_side;
	word sys_stat_reads;
	byte beta_state;
};

struct beta_state beta_state;

///////////////////////////////////////////////////////////////////////

void beta_dsk_init();
void beta_init_drive( byte drv_id );
void clear_drive( byte drv_id );

///////////////////////////////////////////////////////////////////////


word trdos_sector( byte sec, byte trk, byte side )
{
	return ( trk * cur_drv->side_cnt * cur_drv->sec_cnt ) + side * cur_drv->sec_cnt + sec;
}

///////////////////////////////////////////////////////////////////////

byte read_file( FIL *f, byte *dst, byte sz )
{
	UINT nr;
	if ( f_read( f, dst, sz, &nr ) != FR_OK || nr != sz )
	{
		return 0;
	}
	return ( byte )nr;
}

byte write_file( FIL *f, byte *dst, byte sz )
{
	UINT nw;
	if ( f_write( f, dst, sz, &nw ) != FR_OK || nw != sz )
	{
		return 0;
	}
	return ( byte )nw;
}

byte read_le_byte( FIL *f, byte *dst )
{
	return read_file( f, dst, 1 );
}


byte read_le_word( FIL *f, word *dst )
{
	byte w[2];
	if ( !read_file( f, w, 2 ) )
	{
		return 0;
	}
	*dst = ((( word )w[1] ) << 8 ) | (( word )w[0] );
	return 2;
}

byte read_le_dword( FIL *f, dword *dst )
{
	byte dw[4];
	if ( !read_file( f, dw, 4 ) )
	{
		return 0;
	}
	*dst = ((( dword )dw[3] ) << 24 ) | ((( dword )dw[2] ) << 16 ) | ((( dword )dw[1] ) << 8 ) | (( dword )dw[0] );
	return 4;
}

int open_dsk_image( byte drv_id, const char *filename )
{
	FIL f;
	FILINFO fi;

    char lfn[1];
    fi.lfname = lfn;
    fi.lfsize = 0;

	const char *p_ext;
	const char *p_name;

	byte wp = 0, fmt, boot_present, nfiles, boot_file;
	word cyl_cnt, side_cnt, i, last_sec;

	p_ext = filename + strlen( filename );
	while( *p_ext != '.' && p_ext > filename ) p_ext--;

	p_name = filename + strlen( filename );
	while( *p_name != '/' && p_name > filename ) p_name--;
	if( *p_name == '/' ) p_name++;

	if ( strlen( p_ext ) != 4 )
	{
		return 1;
	}

	for ( fmt = DISK_NONE, i = 0; i < NUM_DSK_FORMATS; i++ )
	{
		if ( strcasecmp( p_ext + 1, img_fmt_def[i].ext ) == 0 )
		{
			fmt = img_fmt_def[i].fmt;
			break;
		}
	}
	if (( fmt & 0x7f ) == DISK_NONE )
	{
		return 1;
	}

	if ( f_stat( filename, &fi ) != FR_OK )
	{
		return 2;
	}


	if ( f_open( &f, filename, FA_OPEN_EXISTING | ( fmt & 0x80 ? 0 : FA_WRITE ) | FA_READ ) != FR_OK )
	{
		TRACE(( "file open failed\n" ) );
		return 2;
	}

	fmt &= 0x7f;

	boot_present = 0;
	boot_file = 0;

	if ( fmt == DISK_SCL )
	{
		byte sig[8];
		word first_cat_sec[8];
		sys_sec_area *sys_area;

		if ( !read_file( &f, sig, 8 ) )
		{
			f_close( &f );
			return 2;
		}

		if ( memcmp( sig, "SINCLAIR", 8 ) )
		{
			TRACE(( "invalid signature, %.8s\n", sig ) );
			f_close( &f );
			return 2;
		}

		if ( !read_le_byte( &f, &nfiles ) )
		{
			f_close( &f );
			return 2;
		}
		if ( nfiles > 128 )
		{
			f_close( &f );
			return 2;
		}
		last_sec = TRD_SEC_PER_CYL;
		for ( i = 0; i < nfiles; i++ )
		{
			byte file_sz;

			f_lseek( &f, 9 + i * 14 + 13 );
			if ( !read_le_byte( &f, &file_sz ) )
			{
				f_close( &f );
				return 2;
			}
			if ( i % ( TRD_SEC_SIZE / FDSC_SIZE ) == 0 )
			{
				first_cat_sec[i / ( TRD_SEC_SIZE / FDSC_SIZE )] = last_sec;
			}
			last_sec += file_sz;
		}
		if ( last_sec > TRD_SEC_PER_CYL * TRD_CYL_CNT * TRD_SIDE_CNT )
		{
			f_close( &f );
			return 2;
		}

		word nameSize = p_ext - p_name;
		if ( nameSize > 8 ) nameSize = 8;

		sys_area = (sys_sec_area*) drives[drv_id].scl_sys_sec;
		memcpy( sys_area, &scl_sys_sec_def, 32/*sizeof(sys_area)*/ );

		for ( i = 0; i < nameSize; i++ )
		{
			sys_area->label[i] = toupper( p_name[i] );
		}

		for ( i = 0; i < 8; i++ )
		{
			drives[drv_id].scl_cat_sec[i] = first_cat_sec[i];
		}

		sys_area->first_sec = last_sec % TRD_SEC_PER_CYL;
		sys_area->first_trk = last_sec / TRD_SEC_PER_CYL;
		sys_area->dtype = 0x16;
		sys_area->nfiles = nfiles;
		sys_area->free_sec_lo = ( byte )((( TRD_SEC_PER_CYL * 2 * TRD_CYL_CNT * TRD_SIDE_CNT ) - last_sec ) & 0xff );
		sys_area->free_sec_hi = ( byte )((( TRD_SEC_PER_CYL * 2 * TRD_CYL_CNT * TRD_SIDE_CNT ) - last_sec ) >> 8 );
		sys_area->trdos = 0x10;

		side_cnt = TRD_SIDE_CNT;
		cyl_cnt = TRD_CYL_CNT;
		drives[drv_id].sec_cnt = TRD_SEC_PER_CYL;
		drives[drv_id].sec_sz_id = TRD_SEC_SZ_ID;
		drives[drv_id].sec_size = TRD_SEC_SIZE;
		drives[drv_id].scl_last_sec = last_sec;
		drives[drv_id].scl_data = nfiles * 14 + 9;
		drives[drv_id].scl_numblk = nfiles;
		wp = 1;
	}
	else if ( fmt == DISK_TRD )
	{
//    byte dtype, trd, d;
		byte trd_id[5];
		byte trd, dtype;

		if ( f_lseek( &f, 256 * 8 + 227 ) != FR_OK )
		{
			f_close( &f );
			return 2;
		}
		if ( !read_file( &f, trd_id, sizeof( trd_id ) ) )
		{
			f_close( &f );
			return 2;
		}

		dtype = trd_id[0];
		trd = trd_id[4];
		TRACE(( "trd: dtype = %x, trd = %x\n", dtype, trd ) );
		if (( !( trd == 0 || dtype == 0 ) ) && ( trd != 0x10 || dtype < 0x16 || dtype > 0x19 ) )
		{
			// disk contains trash
			f_close( &f );
			return 1;
		}
		side_cnt = dtype & 0x08 ? 1 : 2;
		cyl_cnt = dtype & 0x01 ? 40 : 80;
		// scan catalog for boot present

		beta_init_drive( drv_id );

		drives[drv_id].sec_cnt = TRD_SEC_PER_CYL;
		drives[drv_id].sec_sz_id = TRD_SEC_SZ_ID;
		drives[drv_id].sec_size = TRD_SEC_SIZE;
	}
	else if ( fmt == DISK_FDI )
	{
		byte sig[3];
		word data_offset;
		word trk_offset;
		word max_sec, cur_off;

		if ( !read_file( &f, sig, 3 ) )
		{
			TRACE(( "signature read fail\n" ) );
			f_close( &f );
			return 2;
		}
		if ( !(( sig[0] == 'F' ) && ( sig[1] == 'D' ) && ( sig[2] == 'I' ) ) )
		{
			TRACE(( "invalid signature, %.3s\n", sig ) );
			f_close( &f );
			return 2;
		}
		if ( !read_le_byte( &f, &wp ) )
		{
			f_close( &f );
			return 2;
		}
		if ( !read_le_word( &f, &cyl_cnt ) )
		{
			f_close( &f );
			return 2;
		}
		if ( !read_le_word( &f, &side_cnt ) || side_cnt > 2 )
		{
			f_close( &f );
			return 2;
		}

		f_lseek( &f, 0xa );
		if ( !read_le_word( &f, &data_offset ) )
		{
			f_close( &f );
			return 2;
		}
		if ( !read_le_word( &f, &trk_offset ) )
		{
			f_close( &f );
			return 2;
		}

		cur_off = trk_offset + 0xe;
		for ( i = 0, max_sec = 0; i < ( word )cyl_cnt * ( word )side_cnt; i++ )
		{
			byte nsecs;
			f_lseek( &f, cur_off + 6 );
			if ( !read_le_byte( &f, &nsecs ) )
			{
				f_close( &f );
				return 2;
			}
			if ( nsecs > max_sec )
			{
				max_sec = nsecs;
			}
			cur_off += 7 * ( 1 + nsecs );
		}
		if ( max_sec > SEC_CACHE_SIZE )
		{
			TRACE(( "fdi: found too long trk max_sec = %u\n", max_sec ) );
			f_close( &f );
			return 2;
		}

		beta_init_drive( drv_id );

		drives[drv_id].fdi_data = data_offset;
		drives[drv_id].fdi_trk_head =
			drives[drv_id].fdi_trk = trk_offset + 0xe;
		drives[drv_id].cur_sec_id = 0;
		wp = wp ? 1 : 0;
	}
	if ( drives[drv_id].img_fmt != DISK_NONE )
	{
		f_close( &drives[drv_id].fd );
	}
	memcpy( &drives[drv_id].fd, &f, sizeof( FIL ) );

	if (( fi.fattrib & AM_RDO ) != 0 )
	{
		wp = 1;
	}

	drives[drv_id].wp = wp;
	drives[drv_id].img_fmt = fmt;
	drives[drv_id].side_cnt = ( byte )side_cnt;
	drives[drv_id].cyl_cnt = ( byte )cyl_cnt;

	return 0;
}

void close_dsk_image( byte drv_id )
{
	if ( drives[drv_id].img_fmt != DISK_NONE )
	{
		f_close( &drives[drv_id].fd );
		clear_drive( drv_id );
	}
}

///////////////////////////////////////////////////////////////////////

byte update_fdi_sec_cache( void )
{
	byte sec_id;
	f_lseek( &cur_drv->fd, cur_drv->fdi_trk_head );
	TRACE(( "fdi_trk_head = %u\n", cur_drv->fdi_trk_head ) );

	read_le_dword( &cur_drv->fd, &cur_drv->img_trkoff );

	for ( sec_id = 0; sec_id < cur_drv->sec_cnt; sec_id++ )
	{
		byte s, sz, shift;
		f_lseek( &cur_drv->fd, cur_drv->fdi_trk_head + ( sec_id + 1 ) * 7 + 2 );
		if ( !read_le_byte( &cur_drv->fd, &s ) )
		{
			return 0;
		}
		if ( !read_le_byte( &cur_drv->fd, &sz ) )
		{
			return 0;
		}
		cur_drv->fdi_sec_id[sec_id] = s;
		shift = ( sec_id & 3 ) << 1;
		cur_drv->fdi_sec_sz[sec_id >> 2] &= ~( 3 << shift );
		cur_drv->fdi_sec_sz[sec_id >> 2] |= ( sz & 3 ) << shift;
		TRACE(( "update_fdi_sec_cache: %u, %u, %x\n", sec_id, s, sz ) );
	}
	return 1;
}

byte lookup_track( byte alloc )
{
	if ( cur_drv->img_fmt == DISK_NONE )
	{
		return 0;
	}
	TRACE(( "lookup_track: cur_cyl = %u, cyl_cnt = %u, cur_side=%u\n", cur_drv->cur_cyl, cur_drv->cyl_cnt, beta_state.cur_side ) );
	if ( cur_drv->cur_cyl >= cur_drv->cyl_cnt )
	{
		if ( cur_drv->img_fmt == DISK_TRD )
		{
		}
		else
		{
			cur_drv->sec_cnt = 0;
		}
		return 0;
	}
	if ( cur_drv->img_fmt == DISK_SCL )
	{
		dword req_sec;
		req_sec = ((( dword )cur_drv->cur_cyl ) * (( dword )cur_drv->side_cnt ) + (( dword )beta_state.cur_side ) ) * (( dword )cur_drv->sec_cnt );
		if ( req_sec < cur_drv->scl_last_sec )
		{
			cur_drv->img_trkoff = ( req_sec - (( dword )beta_state.cur_side ) * (( dword )cur_drv->sec_cnt ) - (( dword )cur_drv->sec_cnt ) ) * (( dword )cur_drv->sec_size ) + cur_drv->scl_data;
			return 1;
		}
		return 0;
	}
	else if ( cur_drv->img_fmt == DISK_TRD )
	{
		cur_drv->img_trkoff = (( dword )cur_drv->cur_cyl ) * (( dword )cur_drv->sec_cnt ) * (( dword )cur_drv->sec_size ) * (( dword )cur_drv->side_cnt );
		TRACE(( "lookup_track: trd: trk=%x\n", cur_drv->img_trkoff ) );
	}
	else if ( cur_drv->img_fmt == DISK_FDI )
	{
		word cur_off;
		byte cur_trk, sec_cnt, side;
		byte found = 0;

		cur_off = cur_drv->fdi_trk;
		for ( cur_trk = 0; cur_trk < cur_drv->cyl_cnt; cur_trk++ )
		{
			for ( side = 0; side < cur_drv->side_cnt; side++ )
			{
				f_lseek( &cur_drv->fd, cur_off + 6 );
				TRACE(( "trk_head: trk=%u, side = %u, off = %u\n", cur_trk, side, cur_off ) );
				if ( !read_le_byte( &cur_drv->fd, &sec_cnt ) )
				{
					return 0;
				}
				if ( cur_trk == cur_drv->cur_cyl && side == beta_state.cur_side )
				{
					found = 1;
					break;
				}
				cur_off += 7 + 7 * ( word )sec_cnt;
			}
			if ( found )
			{
				break;
			}
		}
		if ( !found )
		{
			return 0;
		}
		cur_drv->fdi_trk_head = cur_off;
		cur_drv->sec_cnt = sec_cnt;
		if ( !update_fdi_sec_cache() )
		{
			return 0;
		}

		cur_drv->img_trkoff += cur_drv->fdi_data;
		TRACE(( "lookup_track: fdi: trk=%u, head_off=%u, data_off = %x, sec_cnt=%u, side = %u\n", cur_trk, cur_drv->fdi_trk_head, cur_drv->img_trkoff, sec_cnt, side ) );
	}
	return 1;
}

byte lookup_sector( byte sec, byte side_check, byte side )
{
	if ( cur_drv->img_fmt == DISK_NONE )
	{
		return 0;
	}
	TRACE(( "lookup_sector: %u, c = %u, s = %u\n", sec, side_check, side ) );
	if ( cur_drv->img_fmt == DISK_TRD || cur_drv->img_fmt == DISK_SCL )
	{
		if ( sec > cur_drv->sec_cnt || sec == 0 )
		{
			return 0;
		}
		if ( side_check && side != beta_state.cur_side )
		{
			return 0;
		}
		cur_drv->sec_off = ((( word )beta_state.cur_side ) * (( word )cur_drv->sec_cnt ) + sec - 1 ) * (( word )cur_drv->sec_size );
		cur_drv->cur_sec = sec;
		cur_drv->sec_res = ( 1 << cur_drv->sec_sz_id );
	}
	else if ( cur_drv->img_fmt == DISK_FDI )
	{
		byte sec_id;
		word sec_off;
		byte cyl_head[2];

		if ( sec & 0x80 )
		{
			// specified sector number is not real sector number, but sector_id
			sec_id = sec & 0x7f;
			sec = cur_drv->fdi_sec_id[sec_id];
		}
		else
		{
			for ( sec_id = 0; sec_id < cur_drv->sec_cnt; sec_id++ )
			{
				if ( cur_drv->fdi_sec_id[sec_id] == sec )
				{
					break;
				}
			}
		}
		if ( sec_id == cur_drv->sec_cnt )
		{
			// sector not found
			return 0;
		}
		f_lseek( &cur_drv->fd, cur_drv->fdi_trk_head + ( sec_id + 1 ) * 7 );
		if ( !read_file( &cur_drv->fd, cyl_head, 2 ) )
		{
			return 0;
		}
		if ( cyl_head[0] != beta_state.cyl_reg || cyl_head[1] != beta_state.cur_side )
		{
			return 0;
		}
		f_lseek( &cur_drv->fd, cur_drv->fdi_trk_head + ( sec_id + 1 ) * 7 + 4 );
		if ( !read_le_byte( &cur_drv->fd, &cur_drv->sec_res ) )
		{
			return 0;
		}
		if ( !read_le_word( &cur_drv->fd, &sec_off ) )
		{
			return 0;
		}
		cur_drv->cur_sec_id = sec_id;
		cur_drv->sec_sz_id = ( cur_drv->fdi_sec_sz[sec_id >> 2] >> (( sec_id & 3 ) << 1 ) ) & 3;
		cur_drv->sec_size = 0x80 << cur_drv->sec_sz_id;
		cur_drv->cur_sec = sec;
		cur_drv->sec_off = sec_off;
	}
	cur_drv->sec_buff_pos = 0;
	cur_drv->sec_buff_fill = 0;
	cur_drv->next_sec_off = cur_drv->sec_off + cur_drv->sec_size;
	TRACE(( "lookup_sector: data off = %x (r=%x,next_off=%x)\n", cur_drv->sec_off, cur_drv->sec_res, cur_drv->next_sec_off ) );
	f_lseek( &cur_drv->fd, cur_drv->img_trkoff + ( dword )cur_drv->sec_off );
	return 1;
}

byte lookup_next_sector()
{
	if ( cur_drv->img_fmt == DISK_NONE )
	{
		return 0;
	}
	if ( cur_drv->img_fmt == DISK_TRD || cur_drv->img_fmt == DISK_SCL )
	{
		if ( cur_drv->cur_sec == cur_drv->sec_cnt )
		{
			cur_drv->cur_sec = 1;
			lookup_sector( 1, 0, 0 );
		}
		else
		{
			lookup_sector( cur_drv->cur_sec + 1, 0, 0 );
		}
		return cur_drv->cur_sec;
	}
	else if ( cur_drv->img_fmt == DISK_FDI )
	{
		if ( cur_drv->cur_sec_id != cur_drv->sec_cnt )
		{
			if ( cur_drv->cur_sec_id == cur_drv->sec_cnt - 1 )
			{
				lookup_sector( 0x80, 0, 0 );
				return 1;
			}
			else
			{
				lookup_sector( 0x80 | ( cur_drv->cur_sec_id + 1 ), 0, 0 );
				return 2;
			}
		}
	}
	return 0;
}

void fix_sector_crc( byte crc_val )
{
	if ( cur_drv->img_fmt == DISK_FDI )
	{
		if ( cur_drv->cur_sec_id == cur_drv->sec_cnt )
		{
			// sector not found
			return;
		}
		TRACE(( "fdi: fixing sector %u crc to %x at %x\n", cur_drv->cur_sec, crc_val, cur_drv->fdi_trk_head + ( cur_drv->cur_sec_id + 1 ) * 7 + 4 ) );
		f_lseek( &cur_drv->fd, cur_drv->fdi_trk_head + ( cur_drv->cur_sec_id + 1 ) * 7 + 4 );
		write_file( &cur_drv->fd, &crc_val, 1 );
//    f_write(&cur_drv->fd, &crc_val, 1, &nw);
	}
}

byte read_sector( byte size, byte raw )
{
	{
		byte rd_size, nfile;
		word offset;
#if BETADSK_TRACE
		int i;
#endif

		if ( cur_drv->cur_cyl == 0 && beta_state.cur_side == 0 )
		{
			size = 16;
		}

		if (( cur_drv->sec_off + (( word )size ) ) >= cur_drv->next_sec_off )
		{
			rd_size = ( byte )( cur_drv->next_sec_off - cur_drv->sec_off );
			cur_drv->end_of_sec = 1;
		}
		else
		{
			rd_size = size;
		}

		if ( rd_size == 0 )
		{
			return 0;
		}

		offset = cur_drv->sec_size - ( cur_drv->next_sec_off - cur_drv->sec_off );

		if ( cur_drv->img_fmt == DISK_SCL )
		{
			if ( cur_drv->cur_cyl == 0 && beta_state.cur_side == 0 )
			{
				if ( cur_drv->cur_sec == 9 )
				{
					// system sector
					if ( offset < SYS_SEC_AREA )
					{
						goto zero_blk;
					}
					memcpy( cur_drv->sec_buff, cur_drv->scl_sys_sec + ( offset - SYS_SEC_AREA ), rd_size );
				}
				else if ( cur_drv->cur_sec <= 8 )
				{
					// catalog sector
					byte cat_sec_id, prev_sec_cnt;

					cat_sec_id = cur_drv->cur_sec - 1;

					nfile = ( cat_sec_id * 16 ) + ( offset / 16 );

					prev_sec_cnt = cur_drv->sec_buff[13];
					if ( nfile >= cur_drv->scl_numblk )
					{
						goto zero_blk;
					}
					f_lseek( &cur_drv->fd, 9 + nfile * 14 );
					if ( !read_file( &cur_drv->fd, cur_drv->sec_buff, 14 ) )
					{
						return 0;
					}
					TRACE(( "prev_sec=%x, prev_trk=%x, sec_cnt=%x\n", cur_drv->sec_buff[14], cur_drv->sec_buff[15], prev_sec_cnt ) );
					offset = offset ? (( word )cur_drv->sec_buff[14] ) + (( word )cur_drv->sec_buff[15] ) * (( word )TRD_SEC_PER_CYL ) + (( word )prev_sec_cnt ) : cur_drv->scl_cat_sec[cat_sec_id / ( TRD_SEC_SIZE / FDSC_SIZE )];
					cur_drv->sec_buff[14] = offset % TRD_SEC_PER_CYL;
					cur_drv->sec_buff[15] = offset / TRD_SEC_PER_CYL;
				}
				else
				{
zero_blk:
					memset( cur_drv->sec_buff, 0, rd_size );
				}
				goto end_of_read;
			}

			if ( cur_drv->sec_off >= ((( dword )cur_drv->scl_last_sec - TRD_SEC_PER_CYL ) * (( dword )cur_drv->sec_size ) + (( dword )cur_drv->scl_data ) - cur_drv->img_trkoff ) )
			{
				goto zero_blk;
			}
		}

		if ( !read_file( &cur_drv->fd, cur_drv->sec_buff, rd_size ) )
		{
			cur_drv->end_of_sec = 1;
			cur_drv->sec_res = 0;
			return 0;
		}

end_of_read:
		TRACE(( "read_sector: offset=%u, data=", cur_drv->sec_size - ( cur_drv->next_sec_off - cur_drv->sec_off ) ) );
#if BETADSK_TRACE
		for ( i = 0; i < rd_size; i++ )
		{
			TRACE(( "%02x ", cur_drv->sec_buff[i] ) );
		}
		TRACE(( "\n" ) );
#endif
		cur_drv->sec_off += rd_size;
		cur_drv->sec_buff_fill = rd_size;
		cur_drv->sec_buff_pos = 0;
		return rd_size;
	}
	return 0;
}

byte write_sector( byte size, byte raw )
{
	if ( cur_drv->img_fmt == DISK_SCL )
	{
		return 0;
	}

	{
		byte i, wr_size;

		if ( cur_drv->cur_cyl == 0 && beta_state.cur_side == 0 )
		{
			size = 16;
		}

		if (( cur_drv->sec_off + (( word )size ) ) >= cur_drv->next_sec_off )
		{
			wr_size = ( byte )( cur_drv->next_sec_off - cur_drv->sec_off );
			cur_drv->end_of_sec = 1;
		}
		else
		{
			wr_size = size;
		}
		if ( wr_size == 0 )
		{
			return 0;
		}


		if ( !write_file( &cur_drv->fd, cur_drv->sec_buff, wr_size ) )
		{
			return 0;
		}
		size -= wr_size;
		for ( i = 0; i < size; i++ )
		{
			cur_drv->sec_buff[i] = cur_drv->sec_buff[i + wr_size];
		}
		cur_drv->sec_buff_pos = size;
		cur_drv->sec_buff_fill = size;
		cur_drv->sec_off += wr_size;
		return wr_size;
	}
	return 0;
}


///////////////////////////////////////////////////////////////////////

void wd_cmd_write( byte data )
{
	if (( data & 0xf0 ) == CMD_INTERRUPT )
	{
		beta_state.cmd_reg = data;
		beta_state.beta_state = BETA_IDLE;
		SET_SYS_READY();
		return;
	}
	if (( beta_state.sys_stat & SYS_DRQ ) != 0 )
	{
		return;
	}
	beta_state.cmd_reg = data;
	if ( cur_drv->img_fmt == DISK_NONE )
	{
		beta_state.stat_val = STAT_NOTRDY;
		SET_SYS_ACK();
		return;
	}
	CLEAR_STAT();
	data &= 0xf0;
	switch (( data & 0xc0 ) >> 6 )
	{
		// type I commands
	case TYPEI_CMD_1:
	case TYPEI_CMD_2:
		TRACE(( "wd_cmd: typeI cmd: %x (prev_dr=%x,cur_sec=%x)\n", data, beta_state.data_reg, cur_drv->cur_sec ) );
		if ( data == CMD_SEEK )
		{
			cur_drv->cur_cyl = beta_state.cyl_reg = beta_state.data_reg;
		}
		else
		{
			if (( beta_state.cmd_reg & 0x40 ) != 0 )
			{
				beta_state.step_dir = ( beta_state.cmd_reg & 0x20 ) == 0 ? 1 : -1;
			}

			if (( beta_state.cmd_reg & 0x60 ) != 0 )
			{
				if ( beta_state.cmd_reg & TYPEI_BIT_U )
				{
					beta_state.cyl_reg += beta_state.step_dir;
				}

				if ( beta_state.step_dir == -1 && cur_drv->cur_cyl == 0 )
				{
					beta_state.cyl_reg = 0;
					cur_drv->cur_cyl = 0;
				}
				else
				{
					cur_drv->cur_cyl += beta_state.step_dir;
				}
			}
			else
			{
				beta_state.cyl_reg = 0;
				cur_drv->cur_cyl = 0;
			}
		}

		// !!!!!!!!!!!!!!!!!!!!!!!!!!FIXME: save RDCRC status !!!!!!!!!!!!!!!!!!!!
		if ( beta_state.cyl_reg == 0 )
		{
#if RDCRC_TYPEI
			beta_state.stat_val |= STAT_HEAD_CYL_0;
#else
			beta_state.stat_val = STAT_HEAD_CYL_0;
#endif
		}

		if ( beta_state.cmd_reg & ( TYPEI_BIT_H | TYPEI_BIT_V ) )
		{
			cur_drv->head_on = 1;
			beta_state.stat_val |= STAT_HEAD_ON;
		}
		else
		{
			cur_drv->head_on = 0;
		}

		DRAW_SEEK( cur_drv->cur_cyl );

		if ( !lookup_track( 0 ) )
		{
			// only check address mark when motor on & verify flag is on
			if ( beta_state.cmd_reg & TYPEI_BIT_V )
			{
				beta_state.stat_val |= STAT_NOTFOUND;
			}
		}

		if ( cur_drv->wp )
		{
			beta_state.stat_val |= STAT_WP;
		}
		break;

		// type II comamnds
	case TYPEII_CMD:
	{
		byte is_write = beta_state.cmd_reg & 0x20;

		TRACE(( "wd_cmd: typeII cmd: %x\n", data ) );

		if ( is_write && cur_drv->wp )
		{
			beta_state.stat_val |= STAT_WP;
			break;
		}
		if ( lookup_sector( beta_state.sec_reg & 0x7f, ( beta_state.cmd_reg & TYPEII_BIT_C ) != 0, ( beta_state.cmd_reg & TYPEII_BIT_S ) != 0 ) )
		{
			TRACE(( "reading sector sec=%d, img_trk_off=%x, trk_sec_off=%x, trk=%u (drv_trk=%u), side = %u\n", beta_state.sec_reg, cur_drv->img_trkoff, cur_drv->sec_off, beta_state.cyl_reg, cur_drv->cur_cyl, beta_state.cur_side ) );
			cur_drv->head_on = 1;
			beta_state.beta_state = is_write ? BETA_WRITE : BETA_READ;
			cur_drv->end_of_sec = false;
			beta_state.sys_stat_reads = 0;
			SET_SYS_BUSY();
			SET_STAT_BUSY();
			return;
		}
		else
		{
			TRACE(( "invalid sector sec=%d, img_trk_off=%x, trk_sec_off=%x, trk=%u (drv_trk=%u), side = %u\n", beta_state.sec_reg, cur_drv->img_trkoff, cur_drv->sec_off, beta_state.cyl_reg, cur_drv->cur_cyl, beta_state.cur_side ) );
			beta_state.stat_val |= STAT_NOTFOUND;
		}
		break;
	}

	// type III commands
	case TYPEIII_CMD:
		TRACE(( "wd_cmd: typeIII cmd: %x\n", data ) );
		if ( beta_state.cmd_reg & 0x20 )
		{

			beta_state.stat_val |= STAT_LOST;
		}
		else
		{
			// read am
			if ( lookup_sector( cur_drv->cur_sec & 0x7f, 0, 0 ) )
			{
				beta_state.beta_state = BETA_READ_ADR;
				beta_state.sys_stat_reads = 0;
				cur_drv->head_on = 1;
				SET_SYS_BUSY();
				SET_STAT_BUSY();
				return;
			}
			beta_state.stat_val |= STAT_NOTFOUND;
		}
		break;
	}
	SET_SYS_ACK();
}

///////////////////////////////////////////////////////////////////////

byte put_buff( byte data )
{
	cur_drv->sec_buff[cur_drv->sec_buff_pos] = data;
	return ( ++cur_drv->sec_buff_pos != SEC_BUF_SIZE );
}

byte get_buff( byte *data )
{
	*data = cur_drv->sec_buff[cur_drv->sec_buff_pos];
	return ( ++cur_drv->sec_buff_pos != cur_drv->sec_buff_fill );
}

///////////////////////////////////////////////////////////////////////

void wd_cyl_write( byte data )
{
	beta_state.cyl_reg = data;
}

void wd_sec_write( byte data )
{
	beta_state.sec_reg = data;
}

void wd_dat_write( byte data )
{
	beta_state.data_reg = data;

	switch ( beta_state.beta_state )
	{
	case BETA_WRITE:
		{
			byte multi = beta_state.cmd_reg & TYPEII_BIT_M;
			if ( !put_buff( beta_state.data_reg ) )
			{
				byte nr = write_sector( SEC_BUF_SIZE, 0 );
				TRACE(( "%u bytes writen to file\n", nr ) );
				if ( nr == 0 )
				{
					SET_SYS_READY();
					SET_STAT_READY();
					beta_state.stat_val |= STAT_WRERR;
					beta_state.beta_state = BETA_IDLE;
					cur_drv->head_on = 0;
					break;
				}
				if ( cur_drv->end_of_sec )
				{
					byte stat;
					fix_sector_crc( 1 << cur_drv->sec_sz_id );
					stat = lookup_next_sector();
					if ( !stat && multi )
					{
						SET_SYS_READY();
						SET_STAT_READY();
						beta_state.stat_val |= STAT_NOTFOUND;
						beta_state.beta_state = BETA_IDLE;
						cur_drv->head_on = 0;
						break;
					}
					else if ( !multi || stat == 1 )
					{
						SET_SYS_READY();
						SET_STAT_READY();
						beta_state.beta_state = BETA_IDLE;
						cur_drv->head_on = 0;
						break;
					}
				}
			}
			break;
		}
	}
}

void beta_sys_write( byte data )
{
	byte drv_sel;


	drv_sel = data & SYS_DRV_MSK;

	//TRACE(( "beta sys write: %x\n", data ) );

	if ((( beta_state.sys_val ^ data ) & SYS_RST ) != 0 )
	{
		if ( data & SYS_RST )
		{
			beta_dsk_init();
		}
	}

	beta_state.sys_val = data & ~SYS_STAT_MSK;
	cur_drv = drives + drv_sel;
	beta_state.cur_side = ( beta_state.sys_val & SYS_SIDE ) == 0 ? 1 : 0;
}

///////////////////////////////////////////////////////////////////////

byte disk_revolve()
{
	byte stat_cur;

	stat_cur = 0;
	if ( lookup_next_sector() == 1 )
	{
		stat_cur |= STAT_INDEX;
	}
	if ( cur_drv->cur_cyl == 0 )
	{
		stat_cur |= STAT_HEAD_CYL_0;
	}
	return stat_cur;
}

///////////////////////////////////////////////////////////////////////

byte wd_stat_read()
{
	beta_state.sys_stat &= ~SYS_INTRQ;
	if ( beta_state.beta_state == BETA_IDLE && cur_drv->img_fmt != DISK_NONE && cur_drv->sec_cnt != 0 && ( beta_state.sys_val & SYS_HLD_EN ) != 0 && ( beta_state.cmd_reg & 0x80 ) == 0 )
	{
		return disk_revolve() | beta_state.stat_val;
	}
	else
	{
		return beta_state.stat_val;
	}
}

byte wd_cyl_read()
{
	return beta_state.cyl_reg;
}

byte wd_sec_read()
{
	return beta_state.sec_reg;
}

byte wd_dat_read()
{
	byte data_val;

	data_val = 0xff;

	switch ( beta_state.beta_state )
	{
	case BETA_READ:
	{
		byte multi = beta_state.cmd_reg & TYPEII_BIT_M;

		if ( cur_drv->sec_buff_pos == cur_drv->sec_buff_fill )
		{
			if ( !read_sector( SEC_BUF_SIZE, 0 ) )
			{
				SET_SYS_READY();
				SET_STAT_READY();
				beta_state.stat_val |= STAT_RDCRC;
				beta_state.stat_val |= STAT_NOTFOUND;
				beta_state.beta_state = BETA_IDLE;
				cur_drv->head_on = 0;
				break;
			}
		}

		if ( !get_buff( &data_val ) )
		{
			if ( cur_drv->end_of_sec )
			{
				byte cur_bres, stat;

				cur_bres = ( cur_drv->sec_res & ( 1 << cur_drv->sec_sz_id ) ) == 0;

				stat = lookup_next_sector();
				TRACE(( "wd_dat_read: stat = %u (cur_sec=%u)\n", stat, cur_drv->cur_sec ) );
				if ( !stat && multi )
				{
					SET_SYS_READY();
					SET_STAT_READY();
					beta_state.beta_state = BETA_IDLE;
					cur_drv->head_on = 0;
					if ( cur_bres )
					{
						beta_state.stat_val |= STAT_RDCRC;
						beta_state.stat_val |= STAT_NOTFOUND;
					}
					break;
				}
				else if ( !multi || stat == 1 )
				{
					SET_SYS_READY();
					SET_STAT_READY();
					beta_state.beta_state = BETA_IDLE;
					cur_drv->head_on = 0;
					if ( cur_bres )
					{
						beta_state.stat_val |= STAT_RDCRC;
					}
					break;
				}
				cur_drv->end_of_sec = 0;
			}
		}
		break;
	}

	case BETA_READ_ADR:
		if ( cur_drv->sec_buff_pos == 0 )
		{
			word crc = crc_add( crc_init(), cur_drv->sec_buff[0] = cur_drv->cur_cyl );
			if ( cur_drv->img_fmt == DISK_TRD )
			{
				cur_drv->sec_buff[1] = 0; // в TR-DOS всегда 0 ????
			}
			else
			{
				cur_drv->sec_buff[1] = beta_state.cur_side;
			}
			crc = crc_add( crc, cur_drv->sec_buff[1] );
			crc = crc_add( crc, cur_drv->sec_buff[2] = cur_drv->cur_sec );
			crc = crc_add( crc, cur_drv->sec_buff[3] = cur_drv->sec_sz_id );
			cur_drv->sec_buff[4] = ( crc >> 8 );  // crc hi byte
			cur_drv->sec_buff[5] = crc & 0xff;  // crc lo byte
			cur_drv->sec_buff_fill = 6;
		}
		if ( !get_buff( &data_val ) )
		{
			SET_SYS_READY();
			SET_STAT_READY();
			disk_revolve();
			beta_state.beta_state = BETA_IDLE;
			cur_drv->head_on = 0;
		}
		break;
	default:
		break;
	}


	return data_val;
}

byte beta_sys_read()
{
	return /*beta_state.sys_val|*/ beta_state.sys_stat | 0x3f;
}

///////////////////////////////////////////////////////////////////////

void beta_write_port( byte port, byte data )
{
//  TRACE(("beta_write_port: (%02x, %02x)\n", port, data));
	if ( port == BETA_PORT_CMD )
	{
		wd_cmd_write( data );
	}
	if ( port == BETA_PORT_CYL )
	{
		wd_cyl_write( data );
	}
	if ( port == BETA_PORT_SEC )
	{
		wd_sec_write( data );
	}
	if ( port == BETA_PORT_DTA )
	{
		wd_dat_write( data );
		beta_state.sys_stat_reads = 0;
	}
	if ( port & BETA_PORT_SYS_MSK )
	{
		beta_sys_write( data );
	}
}

byte beta_read_port( byte port )
{
	byte data = 0xff;
	if ( port == BETA_PORT_STA )
	{
		data = wd_stat_read();
	}
	if ( port == BETA_PORT_CYL )
	{
		data = wd_cyl_read();
	}
	if ( port == BETA_PORT_SEC )
	{
		data = wd_sec_read();
	}
	if ( port == BETA_PORT_DTA )
	{
		data = wd_dat_read();
		beta_state.sys_stat_reads = 0;
	}
	if ( port & BETA_PORT_SYS_MSK )
	{
		if ( beta_state.sys_stat_reads >= SYS_READS_BEFORE_LOST )
		{
			if ((( beta_state.sys_stat & SYS_INTRQ ) == 0 ) && ( beta_state.beta_state != BETA_IDLE ) )
			{
				TRACE(( "emulating data underrun\n" ) );
				SET_SYS_READY();
				SET_STAT_READY();
				beta_state.stat_val |= STAT_LOST;
			}
			beta_state.sys_stat_reads = 0;
		}
		else
		{
			beta_state.sys_stat_reads += 1;
		}
		data = beta_sys_read();
	}

//  TRACE(("beta_read_port: (%02x) -> %02x\n", port, data));
	return data;
}

///////////////////////////////////////////////////////////////////////

byte beta_get_state()
{
	return beta_state.beta_state;
}

byte beta_cur_drv()
{
	return beta_state.sys_val & SYS_DRV_MSK;
}

byte beta_is_disk_wp( byte drv )
{
	if ( drv < 3 )
	{
		return drives[drv].wp;
	}
	return 0;
}

void beta_set_disk_wp( byte drv, byte wp )
{
	if ( drv < 3 )
	{
		drives[drv].wp = wp ? 1 : 0;
	}
}

byte beta_is_disk_loaded( byte drv )
{
	if ( drv < 3 )
	{
		return drives[drv].cyl_cnt > 0;
	}
	return 0;
}

///////////////////////////////////////////////////////////////////////

void clear_drive( byte drv_id )
{
	beta_init_drive( drv_id );

	drives[drv_id].sec_cnt = 0;
	drives[drv_id].cyl_cnt = 0;
	drives[drv_id].img_fmt = DISK_NONE;
}

void beta_init_drive( byte drv_id )
{
	TRACE(( "beta_init_drive\n" ) );
//  drives[drv_id].wp = 0;
	drives[drv_id].cur_sec = 1;
	drives[drv_id].cur_cyl = 0;
	drives[drv_id].head_on = 0;
	drives[drv_id].img_trkoff = 0;
	drives[drv_id].sec_off = 0;
	drives[drv_id].next_sec_off = 0;
	drives[drv_id].sec_buff_pos = 0;
	drives[drv_id].sec_buff_fill = 0;
	drives[drv_id].sec_res = 0;
}

void beta_dsk_init()
{
	beta_state.beta_state = BETA_IDLE;
	beta_state.cyl_reg = 0;
	beta_state.sec_reg = 1;
	beta_state.stat_val = 0;
	beta_state.step_dir = 1;
	beta_state.sys_stat_reads = 0;
}

void beta_dump_state()
{
	static byte dump_num = 0;
	static char dump_fn[] = "bstat000.log";
	byte num, i;
	UINT nw;
	FIL fd;

	i = 7;
	num = dump_num;
	do
	{
		dump_fn[i] = ( num % 10 ) + '0';
		i -= 1;
		num /= 10;
	}
	while ( num > 0 );

	dump_num += 1;

	f_open( &fd, dump_fn, FA_WRITE | FA_CREATE_ALWAYS );
	f_write( &fd, drives, sizeof( struct beta_drive ) * BETADSK_NUM_DRIVES, &nw );
	num = 0xed;
	f_write( &fd, &num, 1, &nw );
	f_write( &fd, &num, 1, &nw );
	f_write( &fd, &num, 1, &nw );
	f_write( &fd, &num, 1, &nw );
	f_write( &fd, &beta_state, sizeof( struct beta_state ), &nw );
	f_close( &fd );
}

void beta_init()
{
	int i;

	beta_dsk_init();
	for ( i = 0; i < BETADSK_NUM_DRIVES; i++ )
	{
		clear_drive( i );
	}

	beta_state.sys_val = SYS_RST;
	beta_state.sys_stat = 0;
	cur_drv = drives + 0;
	beta_state.cur_side = 0;
}

int beta_leds()
{
    byte beta_state = beta_get_state();

    int result = 0;

    if ( beta_state == BETA_READ || beta_state == BETA_READ_TRK || beta_state == BETA_READ_ADR || beta_state == BETA_SEEK )
        result |= 1;

    if ( beta_state == BETA_WRITE || beta_state == BETA_WRITE_TRK )
        result |= 4;

    return result;
}
