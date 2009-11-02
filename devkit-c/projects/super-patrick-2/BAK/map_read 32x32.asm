	INCLUDE "global.asm"

	.area	_CODE
_map_read::
	push hl
	
	; Charge les 2 arguments sur la pile (+2 car on a poussé HL)
	lda hl, 4(sp)
	ld e, (hl)			; e = x
	inc hl
	ld d, (hl)			; d = y
	
	; Calcule l'adresse à lire
	ld hl, #_bglvl01_map
	ld c, d		; lo(bc) = y << 5
	Mul32 c
	ld b, d		; hi(bc) = y >> 3
	Div8 b
	add hl, bc
	; Maintenant en X
	ld c, e		; bc = x
	ld b, #0
	add hl, bc
	
	ld e, (hl)	; valeur de retour
	pop hl
	ret

