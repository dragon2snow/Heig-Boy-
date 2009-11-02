	INCLUDE "global.asm"

	.area	_CODE
; HL = source, DE = dest, BC = count
memcpy::
	inc b
	jr  2$
1$:
    ld  a, (hl+)
	ld  (de), a
	inc de
2$:
    dec c
	jr  nz, 1$
	dec b
	jr  nz, 1$
	ret

; HL = dest, A = val, BC = count
memset::
	inc b
	jr  2$
1$:
    ld  (hl+), a
2$:
    dec c
	jr  nz, 1$
	dec b
	jr  nz, 1$
	ret
