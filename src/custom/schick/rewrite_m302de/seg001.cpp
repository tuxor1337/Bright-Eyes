/*
 *	Rewrite of DSA1 v3.02_de functions of seg001 (cdrom)
 *	Functions rewritten: 9/21
 *	Borlandified and identical: 9/21
 *
 *	Remarks:
 *		The first part of this file is for inclusion in DOSBox.
 *		Aim here is, that they work identical.
 *
 *		The second is a rewrite using the inline assembler of
 *		Borland C++ 3.1. The aim is, to produce exactly the same
 *		OPcodes like in the original. Seems to work. :)
 *
 *	MSCDEX:
 *		In this segment are mostly calls to the MSCDEX-API.
 *		It seems, that MSCDEX 2.1 is needed,
 *		since function 0x10 (driver request) is called.
 *		Further this code is written only on the first installed
 *		CD-ROM drive.
 *
 *
 */

#include "v302de.h"

#include "seg000.h"

#define CDA_DATASEG (0x1238)

#if !defined(__BORLANDC__)

#include "dosbox.h"
#include "regs.h"
#include "callback.h"
#include "cpu.h"

#include "seg001.h"

namespace M302de {

static unsigned short CD_has_drives() {

	reg_ax = 0x1500;
	reg_bx = 0x0000;
	CALLBACK_RunRealInt(0x2f);

	if (reg_bx == 0)
		return 0;

	return 1;
}

static unsigned short CD_count_drives() {

	reg_ax = 0x1500;
	reg_bx = 0x0000;
	CALLBACK_RunRealInt(0x2f);

	return reg_bx;
}

static unsigned short CD_get_first_drive() {

	reg_ax = 0x1500;
	reg_bx = 0x0000;
	CALLBACK_RunRealInt(0x2f);

	return reg_cx;
}

unsigned short CD_set_drive_nr() {

	if (CD_has_drives() == 0)
		return 0;

	if (CD_count_drives() == 0)
		return 0;

	ds_writew(CD_DRIVE_NR, CD_get_first_drive());
	return 1;
}

void CD_driver_request(RealPt req) {
	reg_ax = 0x1510;
	reg_cx = ds_readw(CD_DRIVE_NR);
	CPU_SetSegGeneral(es, RealSeg(req));
	reg_bx = RealOff(req);
	CALLBACK_RunRealInt(0x2f);
}

unsigned int CD_get_tod() {
	reg_ah = 0;
	CALLBACK_RunRealInt(0x1a);
	reg_ax = reg_dx;
	reg_dx = reg_cx;

	return (reg_dx << 16) | reg_ax;
}

void seg001_00c1(unsigned short track_nr) {
	unsigned int track_start, track_end;
	unsigned int track_len, tmp;

	if (ds_readw(0x95) == 0)
		return;

	real_writew(reloc_game + CDA_DATASEG, 0x8f, 0);

	tmp = real_readd(reloc_game + CDA_DATASEG, 0x10a + track_nr * 8) & 0x00ffffff;
	real_writed(reloc_game + CDA_DATASEG, 0x9a, tmp);

	/* calculate track_start */
	tmp = real_readb(reloc_game + CDA_DATASEG, 0x10c + track_nr * 8) * 60;
	tmp += real_readb(reloc_game + CDA_DATASEG, 0x10b + track_nr * 8);
	tmp *= 75;
	tmp += real_readb(reloc_game + CDA_DATASEG, 0x10a + track_nr * 8);
	track_start = tmp;

	/* calculate track_end */
	if (real_readb(reloc_game + CDA_DATASEG, 0x422) == track_nr) {
		tmp = real_readb(reloc_game + CDA_DATASEG, 0x425) * 60;
		tmp += real_readb(reloc_game + CDA_DATASEG, 0x424);
		tmp *= 75;
		tmp += real_readb(reloc_game + CDA_DATASEG, 0x423);
	} else {
		tmp = real_readb(reloc_game + CDA_DATASEG, 0x114 + track_nr * 8) * 60;
		tmp += real_readb(reloc_game + CDA_DATASEG, 0x113 + track_nr * 8);
		tmp *= 75;
		tmp += real_readb(reloc_game + CDA_DATASEG, 0x112 + track_nr * 8);
	}
	track_end = tmp;

	track_len = track_end - track_start;
	real_writed(reloc_game + CDA_DATASEG, 0x9e, track_len - 150);

	CD_driver_request(RealMake(reloc_game + CDA_DATASEG, 0x8c));
	ds_writed(CD_AUDIO_POS, ((track_len - 150) * 0x1234e) / 0x4b000);
	ds_writed(CD_AUDIO_TOD, CD_get_tod());
}

void seg001_02c4(void) {

	signed int val;

	if (ds_readw(0x95) == 0)
		return;

	val = CD_get_tod();
	val -= ds_readd(CD_AUDIO_TOD);

	if (val < (signed int)ds_readd(CD_AUDIO_POS))
		return;

	if (ds_readw(0x9b) != 1)
		return;

	CD_audio_stop_hsg();
	CD_audio_stop_hsg();
	seg001_00c1(ds_readw(0xbc40));
	ds_writew(0x9b, 1);
}

signed short CD_bioskey(signed short cmd) {
	seg001_02c4();
	return bc_bioskey(cmd);
}

/* CD_audio_stop_hsg() - stop audio playback in HSG format */
/* static */
void CD_audio_stop_hsg(void) {

	if (ds_readw(0x95) == 0)
		return;

	real_writew(reloc_game + CDA_DATASEG, 3, 0);
	CD_driver_request(RealMake(reloc_game + CDA_DATASEG, 0));
	ds_writew(0x9b, 0);
}

/* CD_audio_stop() - stop audio playback in HSG and redbook format */
void CD_audio_stop() {
	if (ds_readw(0x95) == 0)
		return;

	CD_audio_stop_hsg();
	real_writew(reloc_game + CDA_DATASEG, 0x1f, 0);
	CD_driver_request(RealMake(reloc_game + CDA_DATASEG, 0x1c));
}

void CD_audio_pause() {
	/* Is CD initialized ? */
	if (ds_readw(0x95) == 0)
		return;

	/* Is CD already paused ? */
	if (ds_readw(0xa1) != 0)
		return;

	/* set CD pause */
	ds_writew(0xa1, 1);
	ds_writed(CD_AUDIO_TOD_BAK, CD_get_tod());
	/* save current position */
	ds_writed(CD_AUDIO_POS_BAK, ds_readd(CD_AUDIO_POS));
	/* set current position to maximum singned int */
	ds_writed(CD_AUDIO_POS, 0x7fffffff);

	real_writew(reloc_game + CDA_DATASEG, 0xab, 0);
	CD_driver_request(RealMake(reloc_game + CDA_DATASEG, 0xa8));
}

void CD_audio_play() {
	/* Is CD initialized ? */
	if (ds_readw(0x95) == 0)
		return;

	/* Is CD paused ? */
	if (ds_readw(0xa1) == 0)
		return;

	CD_check();

	/* reset CD pause */
	ds_writew(0xa1, 0);
	ds_writed(CD_AUDIO_POS, ds_readd(CD_AUDIO_POS_BAK));
	ds_writed(CD_AUDIO_TOD, CD_get_tod() - ds_readd(CD_AUDIO_TOD_BAK) + ds_readd(0xbc4a));

	real_writew(reloc_game + CDA_DATASEG, 0xc7, 0);
	CD_driver_request(RealMake(reloc_game + CDA_DATASEG, 0xc4));
}

void CD_set_track(signed short index)
{
	CPU_Push16(index);
	CALLBACK_RunRealFar(reloc_game + 0x4ac, 0x4f4);
	CPU_Pop16();
}

void CD_check(void)
{
	DUMMY_WARNING();
}

void CD_init(void)
{
	DUMMY_WARNING();
}

}

#else	/* __BORLANDC__ */


#include "seg013.h"

/**
 * CD_has_drives() - check if cdrom-drives are installed
 *
 * Returns: 0 - no drive, 1 - at least one drive
 */
/* Borlandified and identical */
static int CD_has_drives(void)
{
	/* al ==  0: return number of drive letters */
	asm {
		mov ax, 0x1500
		xor bx, bx
		int 0x2f
		xor ax, ax
		or  bx, bx
		jz has_cd
		inc ax
	}
has_cd:

	return _AX;
}

/* Borlandified and identical */
static int CD_count_drives(void)
{

	asm {
		mov ax, 0x1500
		xor bx, bx
		int 0x2f
	}

	return _BX;
}

/* Borlandified and identical */
static int CD_get_first_drive(void)
{
	asm {
		mov ax, 0x1500
		xor bx, bx
		int 0x2f
	}

	return _CX;
}

/* Borlandified and identical */
static int CD_set_drive_nr(void)
{
	if (CD_has_drives() == 0)
		return 0;

	if (CD_count_drives() == 0)
		return 0;

	ds_writew(CD_DRIVE_NR, CD_get_first_drive());

	return 1;
}

/* Borlandified and identical */
static void CD_driver_request(void * request)
{
	asm {
		mov ax, 0x1510
		mov cx, [CD_DRIVE_NR]
		les bx, request
		int 0x2f
	}
}

/* Borlandified and identical */
/* TODO: check adresses of seg013 */
static void CD_unused1(void)
{
	if (ds_readw(0x95) == 0)
		return;

	req[3].status = 0;
	req[3].ptr = cd_buf1;
	cd_buf1[0] = 0x0c;
	CD_driver_request(&req[3]);
}

/**
 * CD_get_tod() - get time of day
 *
 * Returns: clock ticks since midnight, the system time.
 *
 * TODO: produces a compiler warning and is a bit hacky
 */
/* Borlandified and identical */
unsigned long CD_get_tod(void)
{
	asm {
		mov ah, 0x0
		int 0x1a
		mov ax, dx
		mov dx, cx
		jmp near leave_tod
	}
leave_tod:
}


/* Borlandified and identical */
void CD_audio_stop_hsg(void)
{
	if (ds_readw(0x95) == 0)
		return;

	req[0].status = 0;
	CD_driver_request(&req[0]);

	ds_writew(0x9b, 0);
}

/* Borlandified and identical */
void CD_audio_stop(void)
{
	if (ds_readw(0x95) == 0)
		return;

	CD_audio_stop_hsg();
	req[1].status = 0;
	CD_driver_request(&req[1]);
}

void CD_set_track(signed short index)
{
	/* DUMMY */
}

void CD_check(void)
{
	/* DUMMY */
}
#endif	/* __BORLANDC__ */
