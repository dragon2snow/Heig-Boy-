/** \file dasm.c
	\brief Implémentation séparée du désassembleur déclaré dans le module CPU.c
*/
#include "mem.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>			// max
#include "os_specific.h"	// set_text_color

void cpu_disassemble(u16 address, char *name, int *length, int *cycles) {
	const char *reg_names[8] = {"b", "c", "d", "e", "h", "l", "(hl)", "a"};
	const char *pair_names[8] = {"bc", "de", "hl", "sp"};
	const char *stack_reg_names[8] = {"bc", "de", "hl", "af"};
	const char *op_table[8] = {"add a,", "adc a,", "sub a,", "sbc a,", "and", "xor", "or", "cp"};
	const char *conditions[4] = {"nz", "z", "nc", "c"};
	const char *op_incdec[2] = {"inc", "dec"};
	u16 opcode = mem_readb(address);
	// Les opcodes sont vus en octal sous la forme xx yyy zzz où zzz peut par
	// exemple être un des registres ci-dessus
	u8 upper_digit = (opcode >> 6) & 3, mid_digit = (opcode >> 3) & 7, low_digit = opcode & 7;

	// Si on ne trouve rien, c'est un bug -> le signaler
	strcpy(name, "<unk-inst>");
	*length = 0, *cycles = 0;

	// Décodage des opcodes sans opérande particulière
	switch (opcode) {
		// GMB 8-bit load commands
		case 0x0a:
			strcpy(name, "ld a, (bc)");
			*cycles = 2, *length = 1;
			return;
		case 0x1a:
			strcpy(name, "ld a, (de)");
			*cycles = 2, *length = 1;
			return;
		case 0xfa:
			sprintf(name, "ld a, ($%04x)", mem_readw(address + 1));
			*cycles = 4, *length = 3;
			return;
		case 0x02:
			strcpy(name, "ld (bc), a");
			*cycles = 2, *length = 1;
			return;
		case 0x12:
			strcpy(name, "ld (de), a");
			*cycles = 2, *length = 1;
			return;
		case 0xea:
			sprintf(name, "ld ($%04x), a", mem_readw(address + 1));
			*cycles = 4, *length = 3;
			return;
		case 0xf0:
			sprintf(name, "ld a, ($ff%02x)", mem_readb(address + 1));
			*cycles = 3, *length = 2;
			return;
		case 0x08:			// ld (nn), sp PAS DOCUMENTÉ, peut être faux!!!
			sprintf(name, "ld ($%04x), sp", mem_readw(address + 1));
			*cycles = 5, *length = 3;
			return;
		case 0xe0:
			// ldh = load from io (ld avec $ff00 + adresse)
			sprintf(name, "ld ($ff%02x), a", mem_readb(address + 1));
			*cycles = 3, *length = 2;
			return;
		case 0xf2:
			strcpy(name, "ld a, ($ff00 + c)");
			*cycles = 2, *length = 1;
			return;
		case 0xe2:
			// S'écrit ld [c], a avec RGBDS
			strcpy(name, "ld ($ff00 + c), a");
			*cycles = 2, *length = 1;
			return;
		case 0x22:
			// Formellement ldi (hl), a
			strcpy(name, "ld (hl+), a");
			*cycles = 2, *length = 1;
			return;
		case 0x2a:
			strcpy(name, "ld a, (hl+)");
			*cycles = 2, *length = 1;
			return;
		case 0x32:
			// Formellement ldd (hl), a
			strcpy(name, "ld (hl-), a");
			*cycles = 2, *length = 1;
			return;
		case 0x3a:
			strcpy(name, "ld a, (hl-)");
			*cycles = 2, *length = 1;
			return;
		// GMB 16-bit load commands
		case 0xf9:
			strcpy(name, "ld sp, hl");
			*cycles = 2, *length = 1;
			return;
		// GMB 8-bit arithmetic / logical commands
		case 0x27:
			strcpy(name, "daa");
			*cycles = 1, *length = 1;
			return;
		case 0x2f:
			strcpy(name, "cpl");
			*cycles = 1, *length = 1;
			return;
		// GMB 16-bit arithmetic / logical commands
		case 0xe8:
			sprintf(name, "add sp, %i", (s8)mem_readb(address + 1));
			*cycles = 4, *length = 2;
			return;
		case 0xf8:
			sprintf(name, "ld hl, sp + %i", (s8)mem_readb(address + 1));
			*cycles = 3, *length = 2;
			return;
		// GMB rotate and shift commands
		case 0x07:
			strcpy(name, "rlca");		// a <<= 1
			*cycles = 1, *length = 1;
			return;
		case 0x17:
			strcpy(name, "rla");		// a <<= 1 with carry
			*cycles = 1, *length = 1;
			return;
		case 0x0f:
			strcpy(name, "rrca");		// a >>= 1
			*cycles = 1, *length = 1;
			return;
		case 0x1f:
			strcpy(name, "rra");
			*cycles = 1, *length = 1;
			return;
		// GMB control commands
		case 0x3f:
			strcpy(name, "ccf");
			*cycles = 1, *length = 1;
			return;
		case 0x37:
			strcpy(name, "scf");
			*cycles = 1, *length = 1;
			return;
		case 0x00:
			strcpy(name, "nop");
			*cycles = 1, *length = 1;
			return;
		case 0x76:
			strcpy(name, "halt");
			*cycles = 1, *length = 1;
			return;
		case 0x10:
			strcpy(name, "stop");
			*cycles = 0, *length = 1;
			return;
		case 0xf3:
			strcpy(name, "di");
			*cycles = 1, *length = 1;
			return;
		case 0xfb:
			strcpy(name, "ei");
			*cycles = 1, *length = 1;
			return;
		case 0xc3:
			sprintf(name, "jp $%04x", mem_readw(address + 1));
			*cycles = 4, *length = 3;
			return;
		case 0xe9:
			strcpy(name, "jp hl");
			*cycles = 1, *length = 1;
			return;
		case 0x18:
			sprintf(name, "jr $%04x", address + 2 + (s8)mem_readb(address + 1));
			*cycles = 3, *length = 2;
			return;
		case 0xcd:
			sprintf(name, "call $%04x", mem_readw(address + 1));
			*cycles = 6, *length = 3;
			return;
		case 0xc9:
			strcpy(name, "ret");
			*cycles = 4, *length = 1;
			return;
		case 0xd9:
			strcpy(name, "reti");
			*cycles = 4, *length = 1;
			return;
	}

	// L'opération à réaliser dépend principalement des bits du haut
	switch (upper_digit) {
		case 1:		// 01 rrr sss -> ld r, s
			sprintf(name, "ld %s, %s", reg_names[mid_digit], reg_names[low_digit]);
			*length = 1;
			*cycles = mid_digit == 6 ? 2 : 1;		// (hl)
			break;

		case 2:		// 10 ooo rrr -> ooo a, r (opération arithmétique)
			sprintf(name, "%s %s", op_table[mid_digit], reg_names[low_digit]);
			*length = 1, *cycles = 1;
			break;

		case 0:		// 00 xxx xxx (l'opération dépend des bits du bas)
			switch (low_digit) {
				case 0:
					// 00 1cc 000 [nn] -> jr cc, nn
					if ((opcode & 040) == 040) {
						sprintf(name, "jr %s, $%04x", conditions[mid_digit & 3], address + 2 + (s8)mem_readb(address + 1));
						// 3 cycles or 2 if false
						*length = 2, *cycles = 3;
					}
					break;
				case 1:
					// 00 dd0 001 [nn nn] -> ld dd, nnnn (dd est une paire de registres)
					if ((opcode & 010) == 0) {
						sprintf(name, "ld %s, $%04x", pair_names[mid_digit >> 1], mem_readw(address + 1));
						*length = 3, *cycles = 3;
					}
					// 00 dd1 001 -> add hl, dd
					else {
						sprintf(name, "add hl, %s", pair_names[mid_digit >> 1]);
						*length = 1, *cycles = 2;
					}
					break;
				case 3:
					// 00 ddo 011 -> inc dd si o=0, dec dd sinon
					sprintf(name, "%s %s", op_incdec[mid_digit & 1], pair_names[mid_digit >> 1]);
					*length = 1, *cycles = 2;
					break;
				case 4:
				case 5:
					// 00 rrr 10o -> inc r si o=0, dec r sinon
					sprintf(name, "%s %s", op_incdec[low_digit & 1], reg_names[mid_digit]);
					*length = 1, *cycles = 1;
					break;
				case 6:
					// 00 rrr 110 [nn] -> ld r, n
					sprintf(name, "ld %s, $%02x", reg_names[mid_digit], mem_readb(address + 1));
					*length = 2;
					*cycles = mid_digit == 6 ? 3 : 2;		// (hl)
					break;
			}
			break;

		case 3:		// 11 xxx xxx (l'opération dépend des bits du bas)
			switch (low_digit) {
				case 0:
					// 11 0cc 000 -> ret cc
					if ((opcode & 040) == 0) {
						sprintf(name, "ret %s", conditions[mid_digit & 3]);
						// 5 cycles or 2 if false
						*length = 1, *cycles = 5;
					}
					break;
				case 1:
					// 11 qq0 001 -> pop qq (qq est une paire de registres)
					if ((opcode & 010) == 0) {
						sprintf(name, "pop %s", stack_reg_names[mid_digit >> 1]);
						*length = 1, *cycles = 3;
					}
					break;
				case 2:
					// PAS SUR!! 11 0cc 010 [nn nn] -> jp cc, nn | le bit 2 de c est ignoré! Comment sur GB?
					if ((opcode & 040) == 0) {
						sprintf(name, "jp %s, $%04x", conditions[mid_digit & 3], mem_readw(address + 1));
						// 4 cycles or 3 if false
						*length = 3, *cycles = 4;
					}
					break;
				case 4:
					// 11 ccc 100 [nn] -> call cc, nn
					sprintf(name, "call %s, $%04x", conditions[mid_digit & 3], mem_readw(address + 1));
					// 6 cycles or 3 if false
					*length = 3, *cycles = 6;
					break;
				case 5:
					// 11 qq0 101 -> push qq (qq est une paire de registres)
					if ((opcode & 010) == 0) {
						sprintf(name, "push %s", stack_reg_names[mid_digit >> 1]);
						*length = 1, *cycles = 4;
					}
					break;
				case 6:
					// 11 ooo 110 [nn] -> ooo nn
					sprintf(name, "%s $%02x", op_table[mid_digit], mem_readb(address + 1));
					*length = 2, *cycles = 2;
					break;
				case 7:
					// 11 ttt 111 -> rst t
					sprintf(name, "rst $%02x", mid_digit * 8);
					*length = 1, *cycles = 4;
					break;
			}
			break;
	}

	// Extended opcode -> c'est reparti pour un tour
	if (opcode == 0xcb) {
		const char *rs_command[8] = {"rlc", "rrc", "rl", "rr", "sla", "sra", "swap", "srl"};
		opcode = mem_readb(address + 1);
		upper_digit = (opcode >> 6) & 3, mid_digit = (opcode >> 3) & 7, low_digit = opcode & 7;
		// 00 ooo rrr -> rotate / shift commands avec registre
		if ((opcode & 0300) == 0) {
			sprintf(name, "%s %s", rs_command[mid_digit], reg_names[low_digit]);
			*length = 2;
			*cycles = low_digit == 6 ? 4 : 2;		// (HL) -> plus lent
		}
		// oo bbb rrr -> bit/set/res b, r (oo != 0)
		else {
			const char *bit_command[3] = {"bit", "res", "set"};
			sprintf(name, "%s %i, %s", bit_command[upper_digit - 1], mid_digit, reg_names[low_digit]);
			*length = 2;
			if (low_digit == 6)		// (HL)
				*cycles = upper_digit == 1 ? 3 : 4;
			else
				*cycles = 2;
		}
	}
}

void cpu_print_instruction(u16 address) {
	char str[256];
	int length, cycles, i;
	cpu_disassemble(address, str, &length, &cycles);
	printf("%04x   ", address);
	for (i = 0; i < 3; i++)
		if (i < max(length, 1))
			printf("%02x", mem_readb(address++));
		else
			printf("  ");
	if (!length) {		// saute les inconnues pour le moment
		set_text_color(COL_RED);
		printf("  %s", str);
		set_text_color(COL_NORMAL);
	}
	else {
		printf("  %s", str);
		// Align to 16 chars
		for (i = strlen(str); i < 16; i++)
			printf(" ");
		printf("%i", cycles * 4);
	}
	printf("\n");
}
