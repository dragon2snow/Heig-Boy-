#include "cpu.h"
#include "mem.h"
#include "debug.h"
#include <stdio.h>		// temp

/** Registres du processeur (BC, DE, HL, AF dans cet ordre) */
static u8 registers[8];
u16 SP, PC;
/* IME = interrupt master enable, halt = cpu en pause (attend une IRQ) */
static u8 IME, halted;
// Index des registres dans le tableau (simples & paires) 
enum {R_B = 0, R_C, R_D, R_E, R_H, R_L, R_A, R_F};
enum {R_BC = 0, R_DE, R_HL, R_AF};
/** Flags (R_F)
  Bit  Name  Set Clr  Expl.
  7    zf    Z   NZ   Zero Flag
  6    n     -   -    Add/Sub-Flag (BCD)
  5    h     -   -    Half Carry Flag (BCD)
  4    cy    C   NC   Carry Flagenum */
enum {F_C = BIT(4), F_H = BIT(5), F_N = BIT(6), F_Z = BIT(7)};
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

/** Pour une utilisation plus simple */
#define accu registers[R_A]
#define flags registers[R_F]
// Accumulateur et (HL) encodés dans une opérande de type rrr (cf. Z80.DOC)
#define OP_R_HL   6
#define OP_R_ACCU 7

/** Manipulations sur les flags */
#define flag_set(flag)		(flags |= flag)
#define flag_clear(flag)	(flags &= ~(flag))
#define flag_test(flag)		(flags & (flag))

/** Lit une paire de registres
	\param index une des constantes R_BC, R_DE ou R_HL */
static u16 read_pair(u16 index);
/** Ecrit une paire de registres */
static void write_pair(u16 index, u16 value);
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
/** Réalise une addition 16 bits et met à jour les flags.
	\param pair paire de registres à affecter
	\param value nombre 16 bits à additionner
*/
static void add16(u8 pair, u16 value);
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
	write_pair(R_AF, 0x01B0);
	write_pair(R_BC, 0x0013);
	write_pair(R_DE, 0x00D8);
	write_pair(R_HL, 0x0013);
	SP = 0xFFFE;
	PC = 0x0100;
	IME = 0;		// sûr?
	halted = 0;
}

void cpu_trigger_irq(cpu_interrupt_t int_no) {
	// L'interruption sera exécutée dans cpu_exec_instruction
	REG(IF) |= BIT(int_no);
}

/*static void print_regs() {
	printf("af: %04x bc: %04x de: %04x hl: %04x sp: %04x pc: %04x\n", read_pair(R_AF), read_pair(R_BC), read_pair(R_DE), read_pair(R_HL), SP, pcdeb);
}*/

unsigned cpu_exec_instruction() {
	u8 opcode, upper_digit, mid_digit, low_digit;
	char temp_name[256];
	int temp, temp_len;

	// Interruptions en attente?
	if (IME && (REG(IF) & REG(IE))) {
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
			accu = mem_readb(read_pair(R_BC));
			return 2;
		case 0x1a:		// ld a, (de)
			accu = mem_readb(read_pair(R_DE));
			return 2;
		case 0xfa:		// ld a, (nn)
			accu = mem_readb(pc_readw());
			return 4;
		case 0x02:		// ld (bc), a
			mem_writeb(read_pair(R_BC), accu);
			return 2;
		case 0x12:		// ld (de), a
			mem_writeb(read_pair(R_DE), accu);
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
			accu = mem_readb(0xff00 + registers[R_C]);
			return 2;
		case 0xe2:		// ldh (c), a
			mem_writeb(0xff00 + registers[R_C], accu);
			return 2;
		case 0x22: {	// ld (hl+), a
			u16 hl = read_pair(R_HL);
			mem_writeb(hl, accu);
			write_pair(R_HL, hl + 1);
			return 2;
		}
		case 0x2a: {	// ld a, (hl+)
			u16 hl = read_pair(R_HL);
			accu = mem_readb(hl);
			write_pair(R_HL, hl + 1);
			return 2;
		}
		case 0x32: {	// ld (hl-), a
			u16 hl = read_pair(R_HL);
			mem_writeb(hl, accu);
			write_pair(R_HL, hl - 1);
			return 2;
		}
		case 0x3a: {	// ld a, (hl-)
			u16 hl = read_pair(R_HL);
			accu = mem_readb(hl);
			write_pair(R_HL, hl - 1);
			return 2;
		}
		// GMB 16-bit load commands
		case 0xf9:		// ld sp, hl
			SP = read_pair(R_HL);
			return 2;
		// GMB 8-bit arithmetic / logical commands
		case 0x27: {	// daa: http://www.geocities.com/siliconvalley/peaks/3938/z80syntx.htm#DAA
			// Dernière op.: addition ou soustraction?
			s8 direction = flag_test(F_N) ? -1 : 1;
			u8 old_flags = flags;
			flag_clear(F_H | F_C | F_Z);
			// Exemple: 0x8 + 0x3 = 0xb -> 0x11 (+6)
			if ((old_flags & F_H) || (accu & 0xf) >= 0xa)
				accu += direction * 0x06;
			// Pareil pour le digit du haut
			if ((old_flags & F_C) || (accu & 0xf0) >= 0xa0) {
				accu += direction * 0x60;
				flag_set(F_C);
			}
			if (accu == 0)
				flag_set(F_Z);
			return 1;
		}
		case 0x2f:		// cpl - complément à un
			accu = ~accu;
			flag_set(F_N | F_H);
			return 1;
		// GMB 16-bit arithmetic / logical commands
		case 0xe8:		// add sp, (signed)n
			SP = op_add_sp_n();
			return 4;
		case 0xf8:		// ld hl, sp+(signed)n
			write_pair(R_HL, op_add_sp_n());
			return 3;

		// GMB rotate and shift commands
		case 0x07:		// rlca - a <<= 1
			op_rotate_left(OP_R_ACCU, RM_R);
			flag_clear(F_Z);
			return 1;
		case 0x17:		// rla - a=(a<<1)+cy
			op_rotate_left(OP_R_ACCU, RM_RC);
			flag_clear(F_Z);
			return 1;
		case 0x0f:		// rrca - a >>= 1
			op_rotate_right(OP_R_ACCU, RM_R);
			flag_clear(F_Z);
			return 1;
		case 0x1f:		// rla - a=(a>>1)+cy (en bit 7)
			op_rotate_right(OP_R_ACCU, RM_RC);
			flag_clear(F_Z);
			return 1;
		// GMB control commands
		case 0x3f:		// ccf - cy=cy xor 1
			flag_clear(F_N | F_H);
			flags ^= F_C;
			return 1;
		case 0x37:		// scf - cy=1
			flag_clear(F_N | F_H);
			flag_set(F_C);
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
			PC = read_pair(R_HL);
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
						add16(R_HL, op_dd_read(mid_digit >> 1));
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
					flags &= F_C;		// seul flag non affecté
					if (opcode & 1) {	// dec r
						result--;
						if ((result & 0xf) == 0xf)	// half carry
							flag_set(F_H);
						flag_set(F_N);
					}
					else {				// inc r
						result++;
						if ((result & 0xf) == 0)	// half carry (changement de dizaine)
							flag_set(F_H);
					}
					op_r_write(mid_digit, result);
					if (result == 0)
						flag_set(F_Z);
					return 1;
				}
				case 6:
					// 00 rrr 110 [nn] -> ld r, n
					op_r_write(mid_digit, pc_readb());
					return mid_digit == OP_R_HL ? 3 : 2;		// (hl)
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

u16 read_pair(u16 index)
{
	index = (index & 3) << 1;		// modulo 4 paires, fois 2 octets
	return registers[index + 1] | registers[index] << 8;
}

void write_pair(u16 index, u16 value) {
	index = (index & 3) << 1;		// modulo 4 paires, fois 2 octets
	registers[index + 1] = value & 0xff;
	registers[index] = value >> 8 & 0xff;
}

u8 op_r_read(u16 index) {
	if (index == OP_R_HL)
		return mem_readb(read_pair(R_HL));
	else if (index == OP_R_ACCU)
		return accu;
	else
		return registers[index & 7];
}

void op_r_write(u16 index, u8 value) {
	if (index == OP_R_HL)
		mem_writeb(read_pair(R_HL), value);
	else if (index == OP_R_ACCU)
		accu = value;
	else
		registers[index & 7] = value;
}

u16 op_dd_read(u16 index) {
	if (index == 3)
		return SP;
	else
		return read_pair(index);
}

void op_dd_write(u16 index, u16 value) {
	if (index == 3)
		SP = value;
	else
		write_pair(index, value);
}

u16 op_qq_read(u16 index) {
	return read_pair(index);
}

void op_qq_write(u16 index, u16 value) {
	write_pair(index, value);
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
	flag_clear(F_C | F_Z);
	if (val == 0)		// Z si résultat nul
		flag_set(F_Z);
	if (val & ~0xff)	// C si débordement (bits autres que les 8 du registre)
		flag_set(F_C);
	accu = val & 0xff;
}

bool condition_test(u8 operand) {
	// Le bit du haut de l'opérande cc contient le flag à tester (Z, C).
	// Le bit du bas indique si il doit être vrai ou faux (1, 0).
	const u8 flag_table[2] = {F_Z, F_C};
	// Teste le bit demandé (result = 1 s'il est mis)
	u8 result = flag_test(flag_table[(operand & 2) >> 1]) ? 1 : 0;
	// Si l'état du flag testé correspond au test (bit du bas), la
	// condition est vérifiée
	return result == (operand & 1);
}

void add16(u8 pair, u16 value) {
	u16 source = read_pair(pair);
	u32 result = source + value;
	write_pair(pair, result);
	// Affectation des flags
	flag_clear(F_H | F_C | F_N);
	if (result >= 65536)
		flag_set(F_C);
	// Calcule le half carry sur les 8 bits du dessus
	if ((source >> 8 & 0xf) + (value >> 8 & 0xf) >= 0x10)
		flag_set(F_H);
}

void call(u16 address) {
	SP -= 2;			// Pousse PC (adresse de retour) sur la pile
	mem_writew(SP, PC);
	PC = address;		// Et saute à l'adresse
}

u16 op_add_sp_n() {
	int result = SP + (s8)pc_readb();
	flags = 0;
	if (result & 0x10000)		// overflow -> carry
		flag_set(F_C);
	// Si le bit 12 a changé (bit 4 des 8 hauts bits) -> H set
	if ((SP ^ result) & 0x1000)
		flag_set(F_H);
	return (u16)result;
}

void op_rotate_left(u8 index, rotate_method_t method) {
	u8 val = op_r_read(index);
	u8 carry_bit = flag_test(F_C) ? 1 : 0;
	flags = 0;
	if (val & 0x80)		// provoquera la perte du dernier bit?
		flag_set(F_C);
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
	// FIXME Z mis aussi si index == OP_R_ACCU, pas bien?
	if (val == 0)
		flag_set(F_Z);
	op_r_write(index, val);
}

void op_rotate_right(u8 index, rotate_method_t method) {
	u8 val = op_r_read(index);
	u8 carry_bit = flag_test(F_C) ? 0x80 : 0;
	flags = 0;
	if (val & 0x01)		// provoquera la perte du dernier bit?
		flag_set(F_C);
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
	// FIXME Z mis aussi si index == OP_R_ACCU, pas bien?
	if (val == 0)
		flag_set(F_Z);
	op_r_write(index, val);
}

void op_arithmetic(u8 operation, u8 operand) {
	u16 operand16 = operand;
	switch (operation) {
		case 1:			// adc: A=A+n+cy (cy = 1 si F_C, 0 sinon)
			if (flag_test(F_C))
				operand16++;
			// continue sur l'addition normale avec une opérande à additionner
			// éventuellement plus élevée
		case 0:			// add: A=A+n
			flags = 0;		// Cette instruction affecte tous les flags
			// Calcule le half-carry (retenue sur les 4 bits du bas)
			if ((accu & 0xf) + (operand16 & 0xf) >= 0x10)
				flag_set(F_H);
			// Puis l'addition
			accu_write(accu + operand16);
			break;

		case 3:			// sbc: A=A-n-cy
			// Même mécanisme que pour add/adc
			if (flag_test(F_C))
				operand16--;
		case 2:			// sub: A=A-n
			flags = F_N;
			if ((s8)(accu & 0xf) - (s8)(operand16 & 0xf) < 0)
				flag_set(F_H);
			accu_write(accu - operand16);
			break;
		
		// Opérations logiques
		case 4:			// and: A=A&n
			flags = F_H;
			accu_write(accu & operand);
			break;
		case 5:			// xor: A=A^n
			flags = 0;
			accu_write(accu ^ operand);
			break;
		case 6:			// or: A=A|n
			flags = 0;
			accu_write(accu | operand);
			break;

		case 7:			// cp: compare A-n - flags: z1hc
		{	int result = accu - operand;
			flags = F_N;
			if (result == 0)
				flag_set(F_Z);
			// Je suis vraiment pas sûr que le halfcarry est affecté comme ça
			if ((s8)(accu & 0xf) - (s8)(operand & 0xf) < 0)
				flag_set(F_H);
			if (result < 0)
				flag_set(F_C);
			break;
		}
	}
}
