;******************************
;* Petit programme de test ^^ *
;******************************
	INCLUDE	"hardware.inc"

	SECTION	"Startup", HOME[0]
	; Vecteurs RST et interruptions (je les ai ignor�s)
	DS  $100

	; La console boote ici. On saute l'en-t�te et va � Main, o� le code commence
	nop
	jp  Main

	; Logo Nintendo, obligatoire. Permet de d�tecter si la cartouche marche bien.
	DB  $CE,$ED,$66,$66,$CC,$0D,$00,$0B,$03,$73,$00,$83,$00,$0C,$00,$0D
	DB  $00,$08,$11,$1F,$88,$89,$00,$0E,$DC,$CC,$6E,$E6,$DD,$DD,$D9,$99
	DB  $BB,$BB,$67,$63,$6E,$0E,$EC,$CC,$DD,$DC,$99,$9F,$BB,$B9,$33,$3E

	; Reste de l'en-t�te, pas important ici
	DS  28

; D�marrage du programme � proprement parler
Main:
	; D�sactive les interruptions
	di
	; Initialise le registre sp sur un espace pour la pile.
	; Les instructions telles que CALL (appelle une fonction) se servent
	; de la pile et utilisent pour ce faire le registre sp (Stack Pointer)
	; qui doit pointer vers un espace valide pour celle-ci.
	ld  sp, StackTop
	; Les op�rations sur le LCD ne sont possibles que pendant la VBLANK
	call Attend_VBlank

	; [LCDC] = 0 (configure le LCD, voir pandocs)
	ld  a, 0
	; Cette instruction stocke la valeur du registre A � l'adresse rLCDC ($40, d�finie dans hardware.inc).
	; Le h (signifiant hi) de ldh ajoute $FF00 � l'adresse. L'adresse finale sera donc $FF40, ce qui
	; correspond bien � LCDC (cf. pandocs).
	ldh [rLCDC], a

	; L� on va copier la fonte en VRAM (adresse $8000, $ pour hexa)
	; Gr�ce � la routine memcpy. On pr�pare juste les arguments dans les registres.
	; La fonction va s'en servir (voir son en-t�te, plus bas).
	ld  hl, Font
	ld  de, $8000
	ld  bc, 5*16
	call memcpy

	; Configure le scrolling (d�filement) de l'�cran. Voir SCX, SCY dans pandocs.
	ld  a, -16
	ldh [rSCY], a   ; [SCY] = -16 (on est oblig� de passer par un registre)
	ldh [rSCX], a   ; (ie. on ne peut pas �crire directement ldh [rSCX], -16)

	; Configure la palette du plan. Voir pandocs.
	; (% indique une valeur binaire).
	ld  a, %00100111
	ldh [rBGP], a

	; Configure le LCD (LCDC).
	; Chaque valeur (LCDCF_BG8000 par exemple) repr�sente un bit (1, 2, 4, 8, etc.)
	; et les ORer ensemble permet d'en activer plusieurs. On les nomme pour mieux
	; comprendre la source. Ils sont d�finis dans hardware.inc.
	ld  a, LCDCF_ON | LCDCF_BG8000 | LCDCF_BG9800 | LCDCF_BGON
	ldh [rLCDC],a
	
	; Dessine le HELLO sur l'�cran.
	; La matrice de caract�res commence � l'adresse 9800 (sp�cifi� plus haut)
	ld  hl, $9800
	ld  a, 1
	ld  [hl+], a		; car. n�1 = H
	inc a				; incr�mente a (il vaudra 2)
	ld  [hl+], a		; car. n�2 = E
	inc a
	ld  [hl+], a		; car. n�3 = L
	ld  [hl+], a		; 2x
	inc a
	ld  [hl+], a		; car. n�4 = O

	; Boucle infinie - on laisse affich� ce qu'on a fait
dummy:
	call Attend_VBlank
	jp  dummy

; Fonction attendant la VBLANK (que le balayage de l'�cran soit au fond)
Attend_VBlank:
	; Lit le n� de ligne du balayage en cours
	ldh a, [rLY]
	; Et boucle tant qu'il n'atteint pas le bas de l'�cran (144)
	cp  144
	; (carry si a - 144 < 0)
	jr  c, Attend_VBlank
	ret

;***************************************************************************
;* Fonction: memcpy
;* Copie des donn�es d'un endroit de la m�moire � un autre.
;*
;* Arguments (dans les registres suivants):
;*   hl - adresse source
;*   de - adresse destination
;*   bc - nombre d'octets � copier
;*
;* Retour: aucun
;***************************************************************************
memcpy:
	inc b
	inc c
	jr  .skip
.loop
    ld  a,[hl+]
	ld  [de], a
	inc de
.skip
    dec c
	jr  nz, .loop
	dec b
	jr  nz, .loop
	ret

	; Donn�es de la police de caract�res.
	; 0 repr�sente du noir, 1 du gris fonc�, 2 gris clair et 3 blanc, comme d�fini dans la
	; palette plus haut.
Font:
	; Car n�0 vide (les pixels sont sur 2 bits alors en laissant 16 octets de vide on laisse 8x(8x2) bits soit un caract�re de 8x8 vide)
	DS  16
	; H (car. n�1)
	DW  `32000230
	DW  `32000230
	DW  `32000230
	DW  `33333330
	DW  `32000230
	DW  `32000230
	DW  `32000230
	DW  `00000000
	; E (car n�2)
	DW  `33333330
	DW  `32000000
	DW  `32000000
	DW  `33333330
	DW  `32000000
	DW  `32000000
	DW  `33333330
	DW  `00000000
	; L (car n�3)
	DW  `32000000
	DW  `32000000
	DW  `32000000
	DW  `32000000
	DW  `32000000
	DW  `32000000
	DW  `33333330
	DW  `00000000
	; O (car n�4)
	DW  `02333200
	DW  `32000230
	DW  `32000230
	DW  `32000230
	DW  `32000230
	DW  `32000230
	DW  `02333200
	DW  `00000000

	; Pour remplir le reste de la ROM de 0 (se compressera mieux)
	SECTION "Nops", HOME[$1000]
	REPT  $3000
	nop
	ENDR

	; Rajoute encore 16k pour arrondir la ROM � 32k, sa taille th�orique
	SECTION "Nops2", CODE
	REPT  $4000
	nop
	ENDR

	; Variables (ce qu'on trouve en RAM) - section BSS (contrairement � CODE)
	; Ici on a juste r�serv� un espace ($200 = 512 octets) pour la pile
	; StackTop pointe � la fin de cet espace, c'est � dire le sommet de
	; la pile, et son �tat initial.
	SECTION "StartupVars", BSS
Stack:	DS  $200
StackTop:
