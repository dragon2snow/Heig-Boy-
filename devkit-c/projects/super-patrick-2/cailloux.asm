	INCLUDE "global.asm"
	
	.Largeur = 8
	.Hauteur = 8
	.CaillouY = .Sol - .Hauteur

	; Variables globales
	.globl _posCailloux

	.area	_CODE

	; Initialisation des cailloux
_cailloux_init::
	; Crée quelques cailloux aléatoires
	ld a, #0
	ld b, #120
	ld c, #(.Sol - 8)
	call .ecritCaillou
	ret

	; Dessine les cailloux dans l'OAM
_cailloux_draw::
	ld d, #.NbCailloux
	ld e, #0
	
.boucleDessin:			; pour tout caillou...
	ld a, e
	call .getOAM		; pointeur en OAM pour ce perso
	push hl				; sauvé pour plus tard

	ld a, e
	call .litCaillou	; b = caillou[e].x
	
	; Maintenant il faut gérer le scrolling
	Load8 h, #_xCamera
	ld a, b
	sub a, h
	add a, #8
	ld b, a				; b = caillou.x - xCamera + 8

	; Premier sprite
	pop hl				; récupère le pointeur en OAM
	push de
	ld c, #(.CaillouY + 16)
	ld d, #0
	call .storeSprite

	pop de				; prochaine étape de la boucle
	inc e
	dec d
	jr nz, .boucleDessin
	ret

_cailloux_handle::
	ld d, #.NbCailloux
	ld e, #0
.boucleGestion:
	push de
	ld a, e
	call .litCaillou
	ld d, b
	ld e, #.CaillouY	; (d, e) = caillou[e].(x, y)
	call _perso_get_pos	; (b, c) = perso.(x, y)
	
	; Test: point(b, c) dans rectangle[(d, e), (Largeur, Hauteur)]
	ld a, #.PatLargeur
	add b
	sub d
	jr c, .dehors		; b + perso.Largeur < d

	ld a, #.PatHauteur
	add c
	sub e
	jr c, .dehors		; c + perso.Hauteur < e

	ld a, #.Largeur
	add d
	sub b
	jr c, .dehors		; b > d + Caillou.Largeur <=> d < b - Caillou.Largeur

	ld a, #.Hauteur
	add e
	sub c
	jr c, .dehors		; c > e + Caillou.Hauteur <=> e < c - Caillou.Hauteur

	; Fiouw, si on est là le perso touche le caillou il me semble
	Block

.dehors:
	pop de
	inc e
	dec d
	jr nz, .boucleGestion
	ret

	; a [in] = n° du caillou
	; b [out] = position X
.litCaillou:
	ld c, a
	ld b, #0			; bc = a
	ld hl, #_posCailloux
	add hl, bc
	ld b, (hl)
	ret

	; a [in] = n° du caillou
	; b [in] = position X
.ecritCaillou:
	ld e, a
	ld d, #0			; de = a
	ld hl, #_posCailloux
	add hl, de
	ld (hl), b
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

	; Crée un sprite pointé par HL avec les coordonnées X, Y dans (b, c)
	; et le n° de tile dans d
.storeSprite:
	ld a, c
	ld (hl+), a			; sprite.y = c
	ld a, b
	ld (hl+), a			; sprite.x = b
	ld a, d
	ld (hl+), a			; sprite.tile = d
	ld a, #0
	ld (hl+), a			; sprite.attr3 = 0
	ret
