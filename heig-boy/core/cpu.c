#include "cpu.h"
#include "mem.h"
#include "debug.h"

/** Registres du processeur (BC, DE, HL, AF dans cet ordre) */
static u8 registers[8];
static u16 SP, PC;
// Index des registres dans le tableau (simples & paires) 
enum {R_B = 0, R_C, R_D, R_E, R_H, R_L, R_A, R_F};
enum {R_BC = 0, R_DE, R_HL, R_AF};
/** Flags (R_F)
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

void cpu_init() {
	write_pair(R_AF, 0x01B0);
	write_pair(R_BC, 0x0013);
	write_pair(R_DE, 0x00D8);
	write_pair(R_HL, 0x0013);
	SP = 0xFFFE;
	PC = 0x0100;
}

int cpu_exec_instruction() {
	u8 opcode = pc_readb();
	char temp_name[256];
	int temp, temp_len;
	// structure d'un opcode
	// 76 543 210
	// op op1 op2 ex. ld r, s
	u8 mid_digit = (opcode >> 3) & 7, low_digit = opcode & 7;

	cpu_disassemble(PC - 1, temp_name, &temp, &temp_len);
	cpu_print_instruction(PC - 1);

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
		case 0x3e:		// ld A, n
			accu = pc_readb();
			return 3;
		case 0xe0:		// ld (FF00+n),A pas dans le z80
			mem_writeb(0xff00 + pc_readb(),accu); // TODO verify
			return 3;
		case 0x18:		// JR nn
		{		
			s8 saut = (s8)pc_readb();
			PC = PC + saut;
			return 3;
		}
		case 0x2a:
		{		// ldi a,(hl) -> ld (hl),nn en z80
			accu = mem_readb(read_pair(R_HL));
			write_pair(R_HL,read_pair(R_HL)+1);
			return 2;
		}
	}

	// 00 dd0 001 -> ld dd, nn (16 bit load) TODO verify
	if ((opcode & 0317) == 01) {
		// Lit dd
		temp = mid_digit >> 1;
		// Ecrit la valeur
		op_dd_write(temp,pc_readw());
		return 3;
	}

	// 00 r 100 -> inc r TODO verify
	if((opcode & 0307) == 04) {
		u8 temp_calcul, r_val;
		// Lit r
		temp = mid_digit;

		// Sauve l'ancienne valeur pour le half carry
		temp_calcul = op_r_read(temp);

		// incremente r
		op_r_write(temp,op_r_read(temp)+01);

		// Change le flag Z en fonction de la valeur de r
		if (op_r_read(temp) == 0)
			flag_set(F_Z);
		else
			flag_clear(F_Z);

		// Met le flag N à zero
		flag_clear(F_N);

		// Calcul le half carry
		r_val = op_r_read(temp);
		
		r_val = r_val >> 4;
		temp_calcul = temp_calcul >> 4;

		if(temp_calcul == r_val)
			flag_clear(F_H);
		else
			flag_set(F_H);
		
		return 1;
	}


	// 00 nz 000 -> jr nz, e
	if((opcode & 0307) == 00)
	{
		s8 e;
		// Lit nz
		temp = mid_digit;

		// Lit e
		e = (s8)pc_readb();
		
		switch(temp) {
			case 0x07:		// si C == 1
				if(flag_test(F_C))
					PC = PC + e;
				return 3;
			case 0x06:		// si C == 0
				if(!flag_test(F_C))
					PC = PC + e;
				return 3;
			case 0x05:		// si Z == 1
				if(flag_test(F_Z))
					PC = PC + e;
				return 3;
			case 0x04:		// si Z == 0
				if(!flag_test(F_Z))
					PC = PC + e;
				return 3;
			default:
				return 2;
		}		  
	}
	
	// 00 r 101 -> dec r TODO verify, assimilez au inc
	if((opcode & 0307) == 05) {
		u8 temp_calcul, r_val;
		// Lit r
		temp = mid_digit;

		// Sauve l'ancienne valeur pour le half carry
		temp_calcul = op_r_read(temp);

		// incremente r
		op_r_write(temp,op_r_read(temp)-01);

		// Change le flag Z en fonction de la valeur de r
		if (op_r_read(temp) == 0)
			flag_set(F_Z);
		else
			flag_clear(F_Z);

		// Met le flag N à un
		flag_set(F_N);

		// Calcul le half carry
		r_val = op_r_read(temp);
		
		r_val = r_val >> 4;
		temp_calcul = temp_calcul >> 4;

		if(temp_calcul == r_val)
			flag_clear(F_H);
		else
			flag_set(F_H);
		
		return 1;
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

void accu_write(int val) {
	flag_clear(F_C | F_Z);
	if (val == 0)			// Z si résultat nul
		flag_set(F_Z);
	if (val & ~0xff)		// C si débordement (bits autres que les 8 du registre)
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
