	; Nécessite ma version personnalisée de MACCER v0.22.
	; maccer.c, ligne 289 devient:
	; {SOPCODE_ARGS_WS,			TSPECIAL,		SOPCODE_ARGS},

	; Charge une valeur 16 bits dans une paire de registres
	; Exemple: Load16 b, c, #.variable
Load16	MACRO	dest1, dest2, address
	ld hl, address
	ld a, (hl+)
	ld dest2, a
	ld dest1, (hl)
	ENDM

	; Stocke une valeur 16 bits dans la mémoire
	; Exemple: Store16 #.variable, b, c
Store16	MACRO	address, dest1, dest2
	ld hl, address
	ld a, dest2
	ld (hl+), a
	ld (hl), dest1
	ENDM

	; ld reg1|reg2, (hl)
Load16FromHl	MACRO	reg1, reg2
	ld reg2, (hl)
	inc hl
	ld reg1, (hl)
	ENDM

	; ld (hl), reg1|reg2
Store16ToHl	MACRO	reg1, reg2
	ld (hl), reg2
	inc hl
	ld (hl), reg1
	ENDM

Load16ToHl	MACRO	address
	ld hl, address
	ld a, (hl+)
	ld h, (hl)
	ld l, a
	ENDM

Store16FromHl	MACRO	address
	ld a, l
	ld b, h				; ba = hl
	ld hl, address		; hl = addy
	ld (hl+), a			; (hl) = ba
	ld (hl), b
	ENDM

Load8	MACRO	reg, address
	ld hl, address
	ld reg, (hl)
	ENDM

Store8	MACRO	address, reg
	ld hl, address
	ld (hl), reg
	ENDM

BeginFunc	MACRO
	push bc
	push de
	ENDM

EndFunc MACRO
	pop bc
	pop de
	ret
	ENDM

Block	MACRO
block:
	jr block
	ENDM

Mov16	MACRO	dest, src
	push src
	pop dest
	ENDM

Mul32	MACRO	reg
	sla reg
	sla reg
	sla reg
	sla reg
	sla reg
	ENDM

Div8	MACRO	reg
	srl reg
	srl reg
	srl reg
	ENDM

	; Ajoute une consante à un registre
Add	MACRO	reg, val
	ld a, reg
	add val
	ld reg, a
	ENDM

	; A = 0
ClearA	MACRO
	xor a
	ENDM

	; dest = reg1 + reg2
	; A détruit
Add3	MACRO	dest, reg1, reg2
	ld a, reg1
	add reg2
	ld dest, a
	ENDM

	; dest = reg1 - reg2
	; A détruit
Sub3	MACRO	dest, reg1, reg2
	ld a, reg1
	sub reg2
	ld dest, a
	ENDM


	; Attributs des sprites	
	.OAM = 0xfe00
	.SCX = 0x43
	; Boutons du joypad
	.J_START = 0x80
	.J_SELECT = 0x40
	.J_B = 0x20
	.J_A = 0x10
	.J_DOWN = 0x08
	.J_UP = 0x04
	.J_LEFT = 0x02
	.J_RIGHT = 0x01

	; Fonctions de perso.asm
	.globl _perso_init
	.globl _perso_draw
	.globl _perso_handle
	.globl _perso_handle_cam
	; cailloux.asm
	.globl _cailloux_init
	.globl _cailloux_draw
	.globl _cailloux_handle
	; game.asm
	.globl _game_init
	.globl _game_draw
	
	; Constantes du jeu
	.Sol = 8 * 15
	.NbCailloux = 1
	.Gravite = 32
	.PatLargeur = 8
	.PatHauteur = 8
