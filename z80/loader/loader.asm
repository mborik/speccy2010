 DEVICE	ZXSPECTRUM128
 SLOT	0
 
	org	#8040

restore_context:	
	ld	sp, #8001
	pop hl
	pop de
	pop bc
	exx

	pop af
	ex	af,af'

	pop hl
	pop de
	pop bc
	pop iy
	pop ix

	ld	a, ( #8000 )
	ld	i, a
	ld	a, ( #8000 + #14 )
	ld	r, a

	pop af
	pop af
	ld	sp, ( #8000 + #17 )

exit:
	jp	exit

	org	#8080	

save_context:
	ld	( #8000 + #17 ), sp
	ld	sp, #8000 + #17
	push	af
	push	af

	ld	a, i
	ld	( #8000 ), a
	ld	a, r
	ld	( #8000 + #14 ), a

	push	ix
	push	iy
	push	bc
	push	de
	push	hl

	ex	af,af'
	push	af

	exx
	push	bc
	push	de
	push	hl

exit2:
	jp	exit2

 savebin	"loader.bin", 0x8040, 0x80