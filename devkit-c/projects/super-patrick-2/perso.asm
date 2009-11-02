	INCLUDE "global.asm"

	; Autres variables dans main
	.globl _joyPressed
	.globl _xCamera

	.area	_BSS
.xVirgule:				; Position X, valeur 16 bits virgule fixe
	.ds 1
.x:						; Position X, entière
	.ds	1
.yVirgule:
	.ds 1
.y:
	.ds	1
.vx:
	.ds 2
.vy:
	.ds 2

	.area	_CODE
	; Initialise le perso
_perso_init::
	; Initialise tout à zéro
	ld hl, #.x
	ld a, #0
	ld b, #0
	ld c, #8
	call memset		; memset(local vars, 0, 8)
	ret

	; Ajoute la vitesse à une coordonnée
	; pos: variable de position (#.x par exemple)
	; value: variable de vitesse (#.vx par exemple)
AddToPos	MACRO	pos, value
	Load16 b, c, value		; bc = vx
	Load16ToHl pos
	add hl, bc
	Store16FromHl pos
	ENDM

	; Gestion du perso (physique, etc.), appelé une fois par frame
_perso_handle::
	; Ajoute la gravité à la vitesse y
	Load16ToHl #.vy
	ld b, #0
	ld c, #.Gravite
	add hl, bc			; vy += Gravité
	Store16FromHl #.vy

	; Ajoute la vitesse à x et y
	AddToPos #.xVirgule, #.vx
	AddToPos #.yVirgule, #.vy
	
	; Test de collision avec le sol
	Load8 a, #.y
	cp #(.Sol - .PatHauteur)	; c = y < (Sol - Hauteur)
	call nc, .collisionDetectee

	; Gestion du joypad
	Load8 d, #_joyPressed
	ld hl, #0x100			; vitesse par défaut
	
	ld a, #.J_LEFT
	and d					; joyState & J_LEFT ?
	call nz, .appuiGauche
	
	ld a, #.J_RIGHT
	and d					; joyState & J_RIGHT ?
	call nz, .appuiDroite

	Store16FromHl #.vx
	
	; Saut?
	ld a, #.J_A
	and d
	call nz, .appuiA
	ret

.collisionDetectee:			; remet le perso sur le sol
	ld a, #(.Sol - .PatHauteur)
	Store8 #.y, a			; y = Sol - Hauteur
	ld a, #0
	Store8 #.vy, a			; vy = 0
	ret
.appuiGauche:				; ralentit le perso
	ld hl, #0x80
	ret
.appuiDroite:				; accélère le perso
	ld hl, #0x180
	ret
.appuiA:
	Load8 a, #.y
	cp #(.Sol - .PatHauteur)	; c = y < (Sol - Hauteur)
	ret c					; Pas de saut en l'air!
	ld hl, #(-3 * 0x100)	; Vitesse négative (pour le saut)
	Store16FromHl #.vy
	ret

	; Gestion de la caméra
_perso_handle_cam::
	Load8 b, #_xCamera
	Load8 a, #.x
	sub a, b
	cp a, #60				; x - xCamera >= 60?
	call nc, .scrollEnX

	; SCX = xCamera
	Load8 a, #_xCamera
	ldh (#.SCX), a
	ret

.scrollEnX:					; xCamera = x - 60
	Load8 a, #.x
	sub a, #60
	Store8 #_xCamera, a
	ret

	; Dessine le perso dans le sprite n°0
_perso_draw::
	; Détermine les positions du sprite à l'écran
	; On rajoute 8 en x et 16 en y car la Game Boy soustrait cela
	; pour permettre des nombres négatifs
	Load8 a, #.x
	Load8 b, #_xCamera
	sub a, b
	add a, #8
	ld b, a					; b = x - xCamera + 8

	Load8 a, #.y
	add #16					; a = y + 16
	
	; Ecrit les coordonnées x et y
	ld hl, #.OAM
	ld (hl+), a
	ld (hl), b
	ret

	; (b, c) = perso.(x, y)
_perso_get_pos::
	Load8 b, #.x
	Load8 c, #.y
	ret
