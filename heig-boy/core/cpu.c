#include "cpu.h"
#include "mem.h"
#include "debug.h"

/** Registres du processeur (BC, DE, HL, AF dans cet ordre) */
static u8 registers[8];
static u16 SP, PC;
// Index des registres dans le tableau (simples & paires) 
enum {R_B = 0, R_C, R_D, R_E, R_H, R_L, R_A, R_F};
enum {R_BC = 0, R_DE, R_HL, R_AF};
/** Flags (R_F)y
  Bit  Name  Set Clr  Expl.
  7    zf    Z   NZ   Zero Flag
  6    n     -   -    Add/Sub-Flag (BCD)
  5    h     -   -    Half Carry Flag (BCD)
  4    cy    C   NC   Carry Flagenum */
enum {F_C = 1 << 4, F_H = 1 << 5, F_N = 1 << 6, F_Z = 1 << 7};

/** Pour une utilisation plus simple */
#define accu registers[R_A]
#define flags registers[R_F]

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
/** Définit l'état d'un flag.
	\param flag flag à affecter
	\param val met le flag à 0 si 0, 1 sinon
*/
//static void flag_affect(u8 flag, u8 val);
/** Ecrit le résultat de l'opération dans l'accumulateur et met à jour les
	flags Zero ou Carry
	\param val résultat (signé, plus grand que 8 bits) de l'opération.
*/
static void accu_write(int val);

void cpu_init() {
	write_pair(R_AF, 0x01B0);
	write_pair(R_BC, 0x0013);
	write_pair(R_DE, 0x00D8);
	write_pair(R_HL, 0x0013);
	SP = 0xFFFE;
	PC = 0x0100;
}

int cpu_exec_instruction() {
	u16 opcode = pc_readb();
	char temp_name[256];
	int temp, temp_len;
	u8 mid_digit = (opcode >> 3) & 7, low_digit = opcode & 7;

	switch (opcode) {
		case 0x00:		// nop
			// Ne fait rien (but de l'instruction ^^)
			return 1;
		case 0xc3:		// jp nn
			// Saute à l'adresse immédiate
			PC = pc_readw();
			return 4;
		case 0xcd:		// call nn
			// Appel d'une fonction à une adresse immédiate
			SP -= 2;			// Pousse PC (adresse de retour) sur la pile
			mem_writew(SP, PC + 2);
			PC = pc_readw();	// Et saute à l'adresse immédiate
			return 6;
		case 0xc9:		// ret
			// Lit l'adresse de retour depuis la pile
			PC = mem_readw(SP);
			SP += 2;
			return 4;
	}

	// 01 rrr sss -> ld r, s
	if ((opcode & 0300) == 0100) {
		// s -> r
		op_r_write(mid_digit, op_r_read(low_digit));
		return low_digit == 6 ? 2 : 1;		// (hl)
	}
	// 10 ooo rrr -> opération arithmétique a, r
	else if ((opcode & 0300) == 0200) {
		u8 operand = op_r_read(low_digit);
		// TODO finir...
		switch (mid_digit) {
			case 0:		// add
				flags = 0;		// Cette instruction affecte tous les flags
				// Calcule le half-carry (carry sur les 4 bits du bas)
				if ((accu & 0xf) + (operand & 0xf) >= 0x10)
					flag_set(F_H);
				// Puis l'addition
				accu_write(accu + operand);
				return 1;
		}
	}

	cpu_disassemble(--PC, temp_name, &temp, &temp_len);
	dbg_error("unkown instruction @%04x: %s", PC, temp_name);
	PC += temp_len;
	return 1;
}

u16 read_pair(u16 index)
{
	index = (index & 3) << 1;		// modulo 4 paires, fois 2 octets
	return registers[index] | registers[index + 1] << 8;
}

void write_pair(u16 index, u16 value) {
	index = (index & 3) << 1;		// modulo 4 paires, fois 2 octets
	registers[index] = value & 0xff;
	registers[index + 1] = value >> 8 & 0xff;
}

u8 op_r_read(u16 index) {
	if (index == 6)
		return mem_readb(read_pair(R_HL));
	else if (index == 7)
		return accu;
	else
		return registers[index & 7];
}

void op_r_write(u16 index, u8 value) {
	if (index == 6)
		mem_writeb(read_pair(R_HL), value);
	else if (index == 7)
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

u8 pc_readb() {
	return mem_readb(PC++);
}

u16 pc_readw() {
	u16 r = mem_readw(PC);
	PC += 2;
	return r;
}

/*void flag_affect(u8 flag, u8 val) {
	flag_clear(flag);
	if (val)
		flag_set(val);
}*/

void accu_write(int val) {
	flag_clear(F_C);
	flag_clear(F_Z);
	if (val == 0)			// Z si résultat nul
		flag_set(F_Z);
	if (val & ~0xff)		// C si débordement (bits autres que les 8 du registre)
		flag_set(F_C);
	accu = val & 0xff;
}
