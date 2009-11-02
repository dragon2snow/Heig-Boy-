#ifndef COMMON_H
#define COMMON_H

/** Types de base
 * Ceux-ci sont généralement utilisés dans les programmes "homebrew" (amateur)
 * écrits en C pour Game Boy Advance, donc on a décidé de reprendre la notation
 * en clin d'oeil à la communauté.
 */
typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef signed char s8;
typedef signed short s16;
typedef signed int s32;
typedef unsigned long long u64;

typedef enum {false=0, true} bool;

/** Permet d'obtenir un masque pour un bit donné. Par exemple:
	x & BIT(7) teste si le bit 7 est 'set'. */
#define BIT(x)		(1 << (x))

/** Sous windows ces macros sont définies en standard, et c'est bien pratique!
	Retourne respectivement le minimum et le maximum de deux nombres */
#ifndef WIN32
	#define min(x, y)		((x) < (y) ? (x) : (y))
	#define max(x, y)		((x) > (y) ? (x) : (y))
#endif

/** Allocation sur la pile, car le C standard ne supporte pas les tableaux
	locaux d'une taille non constante... */
#if _MSC_VER > 1000
	#define alloca _alloca
#endif

#endif
