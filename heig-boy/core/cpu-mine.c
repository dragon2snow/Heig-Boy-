#include "cpu.h"
#include "mem.h"
#include "debug.h"
#include <stdio.h>		// temp
#include <string.h>		// memcpy

/** Registres du processeur (BC, DE, HL, AF dans cet ordre), par paire */
#ifdef LITTLE_ENDIAN
	// Little endian: octet de poids faible puis de poids fort
	typedef union {
		// Exemple, BC: lo=C, hi=B, word=BC
		struct { u8 lo, hi; };
		u16 word;
	} register_pair_t;
#else
	// Big endian: octet de poids fort puis de poids faible
	// TODO à tester sur un processeur big endian
	typedef union {
		// Exemple, BC: lo=C, hi=B, word=BC
		struct { u8 hi, lo; };
		u16 word;
	} register_pair_t;
#endif

/** Les registres */
register_pair_t registers[4];
/** Pile et instruction courante (program counter) */
u16 SP, PC;

/** IME = interrupt master enable, halt = cpu en pause (attend une IRQ) */
static u8 IME, halted;

/** Méthodes de rotation; que faire avec le bit libéré (représenté par un b):
	00000000 >> 1 = b0000000 ou 0000000 << 1 = 0000000b
	* RM_SA, arithmetic shift:
	    * rotation à gauche: comble avec un 0
		* rotation à droite: comble avec le bit 7
	* RM_SL, logical shift:
	    * rotation à gauche: comble avec un 1
		* rotation à droite: comble avec un 0
	* RM_RC, rotate through carry: comble avec le carry
	* RM_R, rotate: comble avec le bit perdu à l'autre extrémité
*/
typedef enum {RM_SA = 0, RM_SL, RM_RC, RM_R} rotate_method_t;

/** Index des registres dans le tableau (paires) */
enum {R_BC = 0, R_DE, R_HL, R_AF};

/** Définition des flags, cf. pandocs.
	Bit  Name  Set Clr  Expl.
	7    zf    Z   NZ   Zero Flag
	6    n     -   -    Add/Sub-Flag (BCD)
	5    h     -   -    Half Carry Flag (BCD)
	4    cy    C   NC   Carry Flagenum
*/
typedef struct {
	u8 unused: 4;
	u8 carry: 1, halfcarry: 1, n: 1, zero: 1;
} flags_t;

/** Pas très propre, mais permet une bien meilleure compréhension du code
	source. */
#define accu	registers[R_AF].hi
#define flags	(*(flags_t*)(&registers[R_AF].lo))
#define B		registers[R_BC].hi
#define C		registers[R_BC].lo
#define D		registers[R_DE].hi
#define E		registers[R_DE].lo
#define H		registers[R_HL].hi
#define L		registers[R_HL].lo

#define BC		registers[R_BC].word
#define DE		registers[R_DE].word
#define HL		registers[R_HL].word
#define AF		registers[R_AF].word

/** Accumulateur et (HL) encodés dans une opérande de type rrr (cf. Z80.DOC) */
#define OP_R_HL   6
#define OP_R_ACCU 7

/** Décode un n° de registre 8 bits et renvoie un pointeur sur celui-ci.
	\param index opérande 3 bits de type rrr (cf. Z80.DOC).
	\note La valeur 6, (hl) n'est pas permise!
*/
static u8 *op_r_decode(u16 index);
/** Décode un n° de registre d'un opcode et lit le registre correspondant.
	\param index opérande 3 bits de type rrr (cf. Z80.DOC).
*/
static u8 op_r_read(u16 index);
/** Décode un registre d'un opcode et l'écrit.
	\param index opérande 3 bits de type rrr (cf. Z80.DOC).
*/
static void op_r_write(u16 index, u8 value);
/** Décode une paire de registres et la lit.
	\param index opérande 2 bits de type dd (cf. Z80.DOC).
*/
static u16 op_dd_read(u16 index);
/** Décode une paire de registres et l'écrit.
	\param index opérande 2 bits de type dd (cf. Z80.DOC).
*/
static void op_dd_write(u16 index, u16 value);
/** Décode une paire de registres et la lit.
	\param index opérande 2 bits de type qq (cf. Z80.DOC).
*/
static u16 op_qq_read(u16 index);
/** Décode une paire de registres et l'écrit.
	\param index opérande 2 bits de type qq (cf. Z80.DOC).
*/
static void op_qq_write(u16 index, u16 value);
/** Lecture du prochain byte pointé par le PC.
	Aussi utilisé pour récupérer les données immédiates.
	---------------------
	FOUTRE DANS LA DOC!!!
	---------------------
	Vous verrez souvent parler dans les commentaires d'adresse ou de donnée
	immédiate. Cela signifie qu'elle est placée dans le ou les octets suivant
	l'opcode en mémoire.
	Exemple: CD (call) prend ensuite 2 octets donnant l'adresse de la fonction
	à appeler. Ainsi un CD 12 34 (3 octets) appelle la fonction à l'adresse
	$3412.
*/
static u8 pc_readb();
/** Lecture du prochain mot (16 bits) pointé par le PC. */
static u16 pc_readw();
/** Ecrit le résultat de l'opération dans l'accumulateur et met automatiquement
	à jour les flags Zero ou Carry.
	\param val résultat (signé, plus grand que 8 bits) de l'opération.
*/
static void accu_write(int val);
/** Teste une condition (de type cc) sur les flags.
	\param operand opérande de type cc (voir Z80.doc) avec le bit 2 à 0
	\return 0 si la condition est fausse, autre valeur sinon
*/
static bool condition_test(u8 operand);
/** Empile PC puis saute à une adresse absolue.
	\param address adresse où sauter.
*/
static void call(u16 address);
/** Réalise une opération mathématique avec l'accu.
	\param operation bits désignant l'opération (mid_digit)
	\param operand opérande 8 bits à appliquer
*/
static void op_arithmetic(u8 operation, u8 operand);
/** Réalise une addition entre SP et une constante immédiate 8 bits signée en
	mettant à jour les flags.
	\return nouvelle valeur de SP
*/
static u16 op_add_sp_n();
/** Décale les bits d'un registre vers la gauche et met à jour les flags.
	\param index n° du registre à affecter selon le type r (cf. Z80.DOC)
	\param method méthode de rotation
*/
static void op_rotate_left(u8 index, rotate_method_t method);
/** Décale les bits d'un registre vers la droite et met à jour les flags.
	\param index n° du registre à affecter selon le type r (cf. Z80.DOC)
	\param method méthode de rotation
*/
static void op_rotate_right(u8 index, rotate_method_t method);

void cpu_init() {
	// Etat initial de la console
	AF = 0x01B0;
	BC = 0x0013;
	DE = 0x00D8;
	HL = 0x0013;
	SP = 0xFFFE;
	PC = 0x0100;
	IME = 0;		// sûr?
	halted = 0;
}

void cpu_trigger_irq(cpu_interrupt_t int_no) {
	// L'interruption sera exécutée dans cpu_exec_instruction
	REG(IF) |= BIT(int_no);
}

unsigned cpu_get_state(u8 *buffer) {
	// Sauvegarde des registres dans un buffer
	memcpy(buffer, registers, sizeof(registers));
	buffer += sizeof(registers);
	*buffer++ = SP & 0xff;
	*buffer++ = SP >> 8;
	*buffer++ = PC & 0xff;
	*buffer++ = PC >> 8;
	*buffer++ = IME;
	*buffer++ = halted;
	return sizeof(registers) + 6;
}

void cpu_set_state(const u8 *buffer) {
	// Restoration des registres depuis le buffer
	memcpy(registers, buffer, sizeof(registers));
	buffer += sizeof(registers);
	SP = buffer[0] | buffer[1] << 8;
	PC = buffer[2] | buffer[3] << 8;
	IME = buffer[4];
	halted = buffer[5];
}

unsigned cpu_exec_instruction() {
	u8 opcode, upper_digit, mid_digit, low_digit;
	char temp_name[256];
	int temp, temp_len;

	// Interruptions en attente?
	if ((IME || halted) && (REG(IF) & REG(IE))) {
		int i;
		// Teste les bits actifs
		for (i = 0; i < INT_LAST; i++) {
			if (REG(IF) & REG(IE) & BIT(i)) {
				// Désactive les interruptions pour éviter les IRQ multiples,
				// désactive le flag dans IF et saute à l'interruption
				REG(IF) &= ~BIT(i);
				IME = 0;
				call(0x40 + 8 * i);
				halted = 0;
				return 4;
			}
		}
	}
	// Au repos, rien à faire
	if (halted)
		return 1;

	// Décodage de l'opcode
	opcode = pc_readb();
	upper_digit = (opcode >> 6) & 3;
	mid_digit = (opcode >> 3) & 7;
	low_digit = opcode & 7;

	switch (opcode) {
		case 0x00:		// nop
			// Ne fait rien (but de l'instruction ^^)
			return 1;
		// GMB 8-bit load commands
		case 0x0a:		// ld a, (bc)
			accu = mem_readb(BC);
			return 2;
		case 0x1a:		// ld a, (de)
			accu = mem_readb(DE);
			return 2;
		case 0xfa:		// ld a, (nn)
			accu = mem_readb(pc_readw());
			return 4;
		case 0x02:		// ld (bc), a
			mem_writeb(BC, accu);
			return 2;
		case 0x12:		// ld (de), a
			mem_writeb(DE, accu);
			return 2;
		case 0xea:		// ld (nn), a
			mem_writeb(pc_readw(), accu);
			return 4;
		case 0xf0:		// ldh a, (n)
			// ldh = load from io (ld avec $ff00 + adresse)
			accu = mem_readb(0xff00 + pc_readb());
			return 3;
		case 0xe0:		// ldh (n), a
			mem_writeb(0xff00 + pc_readb(), accu);
			return 3;
		case 0x08:		// ld (nn), sp PAS DOCUMENTÉ, peut être faux!!!
			mem_writew(pc_readw(), SP);
			return 5;
		case 0xf2:		// ldh a, (c)
			accu = mem_readb(0xff00 + C);
			return 2;
		case 0xe2:		// ldh (c), a
			mem_writeb(0xff00 + C, accu);
			return 2;
		case 0x22: 		// ld (hl+), a
			mem_writeb(HL++, accu);
			return 2;
		case 0x2a:		// ld a, (hl+)
			accu = mem_readb(HL++);
			return 2;
		case 0x32:		// ld (hl-), a
			mem_writeb(HL--, accu);
			return 2;
		case 0x3a:		// ld a, (hl-)
			accu = mem_readb(HL--);
			return 2;

		// GMB 16-bit load commands
		case 0xf9:		// ld sp, hl
			SP = HL;
			return 2;

		// GMB 8-bit arithmetic / logical commands
		case 0x27: {	// daa: http://www.geocities.com/siliconvalley/peaks/3938/z80syntx.htm#DAA
			// Dernière op.: addition ou soustraction?
			s8 direction = flags.n ? -1 : 1;
			// Exemple: 0x8 + 0x3 = 0xb -> 0x11 (+6)
			if (flags.halfcarry || (accu & 0xf) >= 0xa)
				accu += direction * 0x06;
			// Pareil pour le digit du haut
			if (flags.carry || (accu & 0xf0) >= 0xa0) {
				accu += direction * 0x60;
				flags.carry = 1;
			}
			flags.halfcarry = 0;
			flags.zero = (accu == 0);
			return 1;
		}
		case 0x2f:		// cpl - complément à un
			accu = ~accu;
			flags.n = flags.halfcarry = 1;
			return 1;

		// GMB 16-bit arithmetic / logical commands
		case 0xe8:		// add sp, (signed)n
			SP = op_add_sp_n();
			return 4;
		case 0xf8:		// ld hl, sp+(signed)n
			HL = op_add_sp_n();
			return 3;

		// GMB rotate and shift commands
		case 0x07:		// rlca - a <<= 1
			op_rotate_left(OP_R_ACCU, RM_R);
			flags.zero = 0;
			return 1;
		case 0x17:		// rla - a=(a<<1)+cy
			op_rotate_left(OP_R_ACCU, RM_RC);
			flags.zero = 0;
			return 1;
		case 0x0f:		// rrca - a >>= 1
			op_rotate_right(OP_R_ACCU, RM_R);
			flags.zero = 0;
			return 1;
		case 0x1f:		// rla - a=(a>>1)+cy (en bit 7)
			op_rotate_right(OP_R_ACCU, RM_RC);
			flags.zero = 0;
			return 1;
		// GMB control commands
		case 0x3f:		// ccf - cy=cy xor 1
			flags.n = flags.halfcarry = 0;
			flags.carry = flags.carry ^ 1;
			return 1;
		case 0x37:		// scf - cy=1
			flags.n = flags.halfcarry = 0;
			flags.carry = 1;
			return 1;
		case 0x76:		// halt
			halted = 1;
			return 1;
		case 0x10:		//  stop
			dbg_error("unimplemented STOP");
			return 1;
		case 0xf3:		// di - désactive les interruptions
			IME = 0;
			return 1;
		case 0xfb:		// ei - active les interruptions
			IME = 1;
			return 1;
		case 0xc3:		// jp nn
			// Saute à l'adresse immédiate
			PC = pc_readw();
			return 4;
		case 0xe9:		// jp hl
			PC = HL;
			return 1;
		case 0x18: {	// jr nn
			s8 offset = (s8)pc_readb();
			PC += offset;
			return 3;
		}
		case 0xcd:		// call nn
			// Appel d'une fonction à une adresse immédiate
			call(pc_readw());
			return 6;
		case 0xd9:		// reti - return from interrupt
			IME = 1;
			// Ensuite fait un retour normal
		case 0xc9:		// ret
			// Lit l'adresse de retour depuis la pile
			PC = mem_readw(SP);
			SP += 2;
			return 4;
		case 0xcb:		// opération étendue
			break;
	}

	// L'opération à réaliser dépend principalement des bits du haut
	switch (upper_digit) {
		case 1:		// 01 rrr sss -> ld r, s
			// s -> r
			op_r_write(mid_digit, op_r_read(low_digit));
			return mid_digit == OP_R_HL ? 2 : 1;	// (hl)

		case 2:		// 10 ooo rrr -> ooo a, r (opération arithmétique)
			op_arithmetic(mid_digit, op_r_read(low_digit));
			return 1;

		case 0:		// 00 xxx xxx (l'opération dépend des bits du bas)
			switch (low_digit) {
				case 0:
					// 00 1cc 000 [nn] -> jr cc, nn
					if ((opcode & 040) == 040) {
						s8 offset = pc_readb();
						// Condition ok -> saut & grillage d'un cycle de plus
						if (condition_test(mid_digit & 3)) {
							PC += offset;
							return 3;
						}
						return 2;
					}
					break;
				case 1:
					// 00 dd0 001 [nn nn] -> ld dd, nnnn (dd est une paire de registres)
					if ((opcode & 010) == 0) {
						op_dd_write(mid_digit >> 1, pc_readw());
						return 3;
					}
					// 00 dd1 001 -> add hl, dd
					else {
						// Addition de 2 registres 16 bits (HL + dd)
						u16 value = op_dd_read(mid_digit >> 1);
						u32 result = HL + value;
						// Affectation des flags
						flags.n = 0;
						flags.carry = (result >= 65536);	// Dépassement?
						// Calcule le half carry sur les 8 bits du dessus
						flags.halfcarry =
							((HL >> 8 & 0xf) + (value >> 8 & 0xf) >= 0x10);
						// Résultat: conservation des 16 bits du bas
						HL = (u16)result;
						return 2;
					}
					break;
				case 3:
					// 00 ddo 011 -> inc dd si o=0, dec dd sinon
					if ((opcode & 010) == 0) 		// inc dd
						op_dd_write(mid_digit >> 1, op_dd_read(mid_digit >> 1) + 1);
					else
						op_dd_write(mid_digit >> 1, op_dd_read(mid_digit >> 1) - 1);
					return 2;
				case 4:
				case 5: {
					// 00 rrr 10o -> inc r si o=0, dec r sinon
					u8 result = op_r_read(mid_digit);
					if (opcode & 1) {	// dec r
						result--;
						// Halfcarry si passage d'une dizaine à la précédente.
						// Ex: 20 -> 1f. Donc le dernier digit est f.
						flags.halfcarry = ((result & 0xf) == 0xf);
						flags.n = 1;
					}
					else {				// inc r
						result++;
						// Même mécanisme que pour dec, voir plus haut.
						// 1f -> 20 = changement de dizaine.
						flags.halfcarry = ((result & 0xf) == 0);
						flags.n = 0;
					}
					op_r_write(mid_digit, result);
					flags.zero = (result == 0);
					return 1;
				}
				case 6:
					// 00 rrr 110 [nn] -> ld r, n
					op_r_write(mid_digit, pc_readb());
					return mid_digit == OP_R_HL ? 3 : 2;		// (hl) + lent
			}
			break;

		case 3:		// 11 xxx xxx (l'opération dépend des bits du bas)
			switch (low_digit) {
				case 0:
					// 11 0cc 000 -> ret cc
					if ((opcode & 040) == 0) {
						if (!condition_test(mid_digit & 3))		// faux
							return 2;
						// ret habituel (comme opcode c9)
						PC = mem_readw(SP);
						SP += 2;
						return 5;
					}
					break;
				case 1:
					// 11 qq0 001 -> pop qq (qq est une paire de registres)
					if ((opcode & 010) == 0) {
						op_qq_write(mid_digit >> 1, mem_readw(SP));
						SP += 2;
						return 3;
					}
					break;
				case 2:
					// PAS SUR!! 11 0cc 010 [nn nn] -> jp cc, nn | le bit 2 de c est ignoré! Comment sur GB?
					if ((opcode & 040) == 0) {
						u16 address = pc_readw();
						if (condition_test(mid_digit & 3)) {
							PC = address;
							return 4;
						}
						return 3;
					}
					break;
				case 4: {
					// 11 0cc 100 [nn nn] -> call cc, nn
					u16 address = pc_readw();
					if (condition_test(mid_digit & 3)) {
						call(address);
						return 6;
					}
					return 3;
				}
				case 5:
					// 11 qq0 101 -> push qq (qq est une paire de registres)
					if ((opcode & 010) == 0) {
						SP -= 2;
						mem_writew(SP, op_qq_read(mid_digit >> 1));
						return 4;
					}
					break;
				case 6:
					// 11 ooo 110 [nn] -> ooo nn
					op_arithmetic(mid_digit, pc_readb());
					return 2;
				case 7:
					// 11 ttt 111 -> rst t
					call(mid_digit * 8);
					return 4;
			}
			break;
	}

	cpu_disassemble(PC - 1, temp_name, &temp, &temp_len);
	cpu_print_instruction(PC - 1);
	dbg_error("unimplemented!");
	PC += temp_len - 1;
	return 1;
}

u8 *op_r_decode(u16 index) {
	switch (index) {
		case 0:  return &B;
		case 1:  return &C;
		case 2:  return &D;
		case 3:  return &E;
		case 4:  return &H;
		case 5:  return &L;
		case 7:  return &accu;
		default: return 0L;		// (hl) et autres interdits!
	}
}

u8 op_r_read(u16 index) {
	if (index == OP_R_HL)		// (hl)
		return mem_readb(HL);
	else						// registre habituel
		return *op_r_decode(index);
}

void op_r_write(u16 index, u8 value) {
	if (index == OP_R_HL)		// (hl)
		mem_writeb(HL, value);
	else
		*op_r_decode(index) = value;
}

u16 op_dd_read(u16 index) {
	if (index == 3)
		return SP;
	else
		return registers[index].word;
}

void op_dd_write(u16 index, u16 value) {
	if (index == 3)
		SP = value;
	else
		registers[index].word = value;
}

u16 op_qq_read(u16 index) {
	return registers[index].word;
}

void op_qq_write(u16 index, u16 value) {
	registers[index].word = value;
}

u8 pc_readb() {
	return mem_readb(PC++);
}

u16 pc_readw() {
	u16 r = mem_readw(PC);
	PC += 2;
	return r;
}

void accu_write(int val) {
	// C si débordement (bits autres que les 8 du registre)
	flags.carry = ((val & ~0xff) != 0);
	accu = val & 0xff;
	// Z si résultat nul
	flags.zero = (accu == 0);
}

bool condition_test(u8 operand) {
	// Le bit du haut de l'opérande cc contient le flag à tester (0 = Z, 1 = C).
	// Le bit du bas indique si il doit être vrai ou faux (1, 0).
	if (operand & 2)	// [N]C
		return flags.carry == (operand & 1);
	else				// [N]Z
		return flags.zero == (operand & 1);
}

void call(u16 address) {
	SP -= 2;			// Pousse PC (adresse de retour) sur la pile
	mem_writew(SP, PC);
	PC = address;		// Et saute à l'adresse
}

u16 op_add_sp_n() {
	int result = SP + (s8)pc_readb();
	flags.zero = flags.n = 0;
	flags.carry = ((result & 0x10000) != 0);	// overflow -> carry
	// Si le bit 12 a changé (bit 4 des 8 hauts bits) -> H set
	flags.halfcarry = (((SP ^ result) & 0x1000) != 0);
	return (u16)result;
}

void op_rotate_left(u8 index, rotate_method_t method) {
	u8 val = op_r_read(index);
	u8 carry_bit = flags.carry ? 1 : 0;
	// Les flags sont tous affectés par cette instruction
	flags.n = flags.halfcarry = 0;
	// Provoquera la perte du dernier bit? -> Carry
	flags.carry = ((val & 0x80) != 0);
	switch (method) {
		case RM_R:
			val = val << 1 | val >> 7;
			break;
		case RM_RC:
			val = val << 1 | carry_bit;
			break;
		case RM_SA:
			val = val << 1 | 0;
			break;
		case RM_SL:
			val = val << 1 | 1;
			break;
	}
	// Flag zéro (résultat nul, comme d'habitude)
	flags.zero = (val == 0);
	op_r_write(index, val);
}

void op_rotate_right(u8 index, rotate_method_t method) {
	u8 val = op_r_read(index);
	u8 carry_bit = flags.carry ? 0x80 : 0;
	// Les flags sont tous affectés par cette instruction
	flags.n = flags.halfcarry = 0;
	// Provoquera la perte du dernier bit? -> Carry
	flags.carry = ((val & 0x01) != 0);
	switch (method) {
		case RM_R:
			val = val >> 1 | val << 7;
			break;
		case RM_RC:
			val = val >> 1 | carry_bit;
			break;
		case RM_SA:
			val = val >> 1 | val & 0x80;
			break;
		case RM_SL:
			val >>= 1;
			break;
	}
	// Flag zéro
	flags.zero = (val == 0);
	op_r_write(index, val);
}

void op_arithmetic(u8 operation, u8 operand) {
	u16 operand16 = operand;
	switch (operation) {
		case 1:			// adc: A=A+n+cy (cy = 1 si F_C, 0 sinon)
			if (flags.carry)
				operand16++;
			// continue sur l'addition normale avec une opérande à additionner
			// éventuellement plus élevée
		case 0:			// add: A=A+n
			// Type addition
			flags.n = 0;
			// Calcule le half-carry (retenue sur les 4 bits du bas)
			flags.halfcarry =
				((accu & 0xf) + (operand16 & 0xf) >= 0x10);
			// Puis l'addition
			accu_write(accu + operand16);
			break;

		case 3:			// sbc: A=A-n-cy
			// Même mécanisme que pour add/adc
			if (flags.carry)
				operand16++;
		case 2:			// sub: A=A-n
			// Type soustraction
			flags.n = 1;
			flags.halfcarry =
				((s8)(accu & 0xf) - (s8)(operand16 & 0xf) < 0);
			accu_write(accu - operand16);
			break;
		
		// Opérations logiques
		case 4:			// and: A=A&n
			flags.n = 0;
			flags.halfcarry = 1;
			accu_write(accu & operand);
			break;
		case 5:			// xor: A=A^n
			flags.n = flags.halfcarry = 0;
			accu_write(accu ^ operand);
			break;
		case 6:			// or: A=A|n
			flags.n = flags.halfcarry = 0;
			accu_write(accu | operand);
			break;

		case 7:			// cp: compare A-n - flags: z1hc
		{	int result = accu - operand;
			flags.n = 1;
			// Je suis vraiment pas sûr que le halfcarry est affecté comme ça
			flags.halfcarry =
				((s8)(accu & 0xf) - (s8)(operand & 0xf) < 0);
			flags.zero = (result == 0);
			flags.carry = (result < 0);
			break;
		}
	}
}
