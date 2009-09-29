#ifndef COMMON_H
#define COMMON_H

/** Types de base
 * Ceux-ci sont g�n�ralement utilis�s dans les programmes "homebrew" (amateur)
 * �crits en C pour Game Boy Advance, donc on a d�cid� de reprendre la notation
 * en clin d'oeil � la communaut�.
 */
typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef signed char s8;
typedef signed short s16;
typedef signed int s32;

typedef enum {false=0, true} bool;

//#define NULL ((void*)0)

#ifndef WIN32
	#define min(x, y)		((x) < (y) ? (x) : (y))
	#define max(x, y)		((x) > (y) ? (x) : (y))
#endif

#endif