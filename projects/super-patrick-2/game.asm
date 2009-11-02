	INCLUDE "global.asm"

	.area _CODE
_game_init::
	; Initialise les cailloux à zéro
	ld hl, #_posCailloux
	ld a, #0
	ld b, #0
	ld c, #.NbCailloux
	call memset
	
	call _cailloux_init
	call _perso_init
	ret

_game_draw::
	call _cailloux_draw
	call _perso_draw
	ret

_game_handle::
	call _perso_handle
	call _cailloux_handle
	ret
