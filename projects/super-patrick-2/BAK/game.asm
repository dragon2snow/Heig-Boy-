	INCLUDE "global.asm"
	.NbCailloux = 16

	; Fonctions exportées
	.globl _game_init
	.globl _game_draw
	; Variables globales
	.globl _posCailloux

	.area _CODE
	; a [in] = n° du caillou
	; b, c [out] = positions X et Y
.litCaillou:
	add a, a			; a = index caillou 16 bits
	ld c, a
	ld b, #0			; bc = a
	ld hl, #_posCailloux
	add hl, bc
	Load16FromHl b, c
	ret

	; a [in] = n° du caillou
	; b, c [in] = positions X et Y
.ecritCaillou:
	add a, a			; a = index caillou 16 bits
	ld d, a
	ld e, #0			; bc = a
	ld hl, #_posCailloux
	add hl, de
	Store16ToHl b, c
	ret

	; a [in] = n° du cailloux
	; hl [out] = adresse en OAM de l'objet
.getOAM:
	add a, #1			; sprite 0 réservé pour le perso
	add a
	add a				; 4 octets par OBJ

	ld hl, #.OAM
	ld b, #0
	ld c, a
	add hl, bc			; hl = OAM + 5 * (a + 1)
	ret

	; Dessine les cailloux dans l'OAM
.drawCailloux:
	ld d, #.nCailloux
	ld e, #0
	
.boucleDessin:			; pour tout caillou...
	ld a, e
	call .getOAM		; pointeur en OAM pour ce perso
	push hl				; sauvé pour plus tard

	ld a, e
	call .litCaillou	; (b, c) = (x, y) caillou n°e
	
	; Maintenant il faut gérer le scrolling
	Load8 h, #_xCamera
	ld a, b
	sub a, h
	add a, #8
	ld b, a				; b = x - xCamera + 8
	Addc c, #16			; c = y + 16
	
	pop hl				; récupère le pointeur en OAM
	ld (hl), c			; sprite.y = c
	inc hl
	ld (hl), b			; sprite.x = b
	
	dec d
	jr nz, .boucleDessin
	ret

_game_init::
	; Initialise les cailloux à zéro
	ld hl, #_posCailloux
	ld a, #0
	ld b, #0
	ld c, #(.nCailloux * 2)
	call memset
	
	; Crée quelques cailloux aléatoires
	ld a, #0
	ld b, #80
	ld c, #(.Sol - 8)
	call .ecritCaillou
	
	call _perso_init
	ret

_game_draw::
	call .drawCailloux
	call _perso_draw
	ret

_game_handle::
	call _perso_handle
	ret
