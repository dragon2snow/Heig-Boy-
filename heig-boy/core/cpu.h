#ifndef CPU_H
#define CPU_H

#include "common.h"

/*	De la synchronisation des composants.

	La Game Boy contient plusieurs composants qui fonctionnent � leur rythme,
	mais qui sont tous synchronis�s sur une horloge: l'oscillateur (OSC clock).
	Cette horloge est cadenc�e � 4 MHz et tous les timings peuvent �tre
	exprim�s en fonction de cette horloge, notamment:
	- 4 cycles pour 1 cycle machine du processeur
	- 456 cycles pour le balayage d'une ligne de l'�cran LCD
	- 32 cycles pour la g�n�ration d'une sample audio
	- ...
	Ainsi une fa�on d'�muler le parall�lisme des composants sans utiliser un
	syst�me de threads tr�s complexe et tr�s lent � l'ex�cution, est de tout
	simplement �muler la synchronisation par l'oscillateur!
	
	On a donc une boucle principale qui fait avancer l'horloge d'un cycle, et
	le transmet � tous les composants. Les composants vont alors r�aliser la
	part de travail qui leur incombe durant ce laps de temps.
	Par exemple le CPU ex�cutera une instruction ou finira d'ex�cuter celle
	qu'il a en cours (une instruction durant plusieurs cycles); le contr�leur
	son g�n�rera une "sample" audio ou attendra un peu pour g�n�rer la
	prochaine.
	On a alors une machine d'�tat avec un syst�me �v�nementiel pour chaque
	composant, dont le sch�ma en pseudo-code objet est ainsi:

	Composant:
		temps_dodo: entier := 0
		m�thode tick():			// Un cycle d'horloge s'est pass�
			si temps_dodo > 0:
				temps_dodo--	// Rien � faire
			sinon:
				traite un �v�nement (r�alise une action)
				temps_dodo := temps que dure cette action

		m�thode idle_cycles():	// cycles avant le prochain �v�nement
			retourne temps_dodo

	CPU, LCD, Son: Composants
	Boucle principale:
		CPU.tick()
		LCD.tick()
		Son.tick()
		...

	Notez le raccourci! On n'a pas divis� l'action en le nombre de cycles
	qu'elle prendrait sur le mat�riel. En revanche on l'ex�cute d'un seul coup
	puis on met le composant en pause pour simuler l'ex�cution inachev�e de
	celle-ci.

	Ce mod�le est joli mais il peut toutefois comporter un probl�me: celui de
	la performance du fait du nombre �lev� d'ex�cutions de la boucle principale
	(4 MHz = 4 millions de fois par seconde). Surtout quand on sait que les
	instructions du CPU prennent au moins 4 cycles, et peuvent m�me en durer
	24; on n'aurait pas forc�ment besoin d'�tre aussi pr�cis.

	Une optimisation possible est alors de n'envoyer des ticks que
	p�riodiquement, en indiquant au composant le nombre de cycles qui se sont
	�coul�s depuis la derni�re fois.
	Et comme les �v�nements se produisant le plus souvent sont l'ex�cution
	d'une instruction, pourquoi ne pas le prendre en r�f�rence? La boucle
	principale ressemblerait alors � ceci:

	CPU, LCD, Son: Composants
	Boucle:
		// execInstruction retourne le # de cycles requis par l'instruction
		�coul�s: entier := CPU.execInstruction()
		LCD.tick(�coul�s)
		Son.tick(�coul�s)
		...

	D'o�...
	Les composants ont tous une architecture similaire:
	void ***_tick(int cycles);
	int ***_idle_cycles();
	La premi�re fait "avancer" le temps d'un certain nombre de cycles. Par
	exemple dans le cas du timer cela le fera avancer s'il est activ�.
	La deuxi�me retourne le nombre de cycles pendant lequel le composant
	est inactif (temps avant le prochain �v�nement le concernant).

	Les composants, � part le CPU qui est toujours actif, r�alisent
	p�riodiquement des t�ches puis "s'endorment" pendant un moment (par exemple
	l'�cran dessine une ligne puis attend un signal de synchronisation avant la
	prochaine).

	Ainsi la fonction "tick" doit �tre appel�e aussi souvent que possible pour
	mettre � jour l'�tat des composants (si le timer est mis � jour trop peu
	souvent et affiche des valeurs telles que 1, 4, 8, 11, etc. l'�mulation
	sera probablement peu r�aliste), mais toutefois il n'est pas non plus
	n�cessaire de la rappeler avant que "idle_cycles" soit z�ro, ainsi on
	pourrait imaginer r�duire la boucle principale en utilisant cette notion
	d'�v�nements:

	Boucle:
		// Calcule l'�v�nement le plus proche
		cycles_libres := min(sound_idle_cycles, lcd_idle_cycles, ...)
		// Ensuite ex�cute le CPU jusqu'� cet �v�nement
		cycles_ex�cut�s := 0
		Tant que cycles_ex�cut�s < cycles_libres:
			cycles_ex�cut�s += cpu_exec_instruction()
		// Met � jour les composants (il y en a au moins un qui a quelque
		// chose � faire).
		lcd_tick(cycles_ex�cut�s)
		sound_tick(cycles_ex�cut�s)
		...

	Le fait que le CPU soit "ma�tre" suppose que rien ne se passe pendant
	l'ex�cution de l'instruction qui ait un effet sur celle-ci. En fait m�me si
	cette assomption est fausse, il serait en fait impossible de d�terminer
	exactement comment le mat�riel r�agirait, ainsi on consid�rera que c'est le
	cas.
*/

/** Met le CPU dans l'�tat initial (au d�marrage de la Game Boy) */
void cpu_init();

/** Ex�cute une instruction CPU et donne le nombre de cycles "mang�s". Il
	faudrait faire avancer le reste du hardware du m�me nombre d'unit�s et
	v�rifier si des �v�nements ont lieu avant de continuer � ex�cuter la
	prochaine instruction.
	\return le nombre de cycles utilis�s par l'instruction
*/
unsigned cpu_exec_instruction();

/** Types d'interruptions possibles. */
typedef enum {
	INT_VBLANK = 0,		// p�riode de VBLANK du LCD
	INT_STAT = 1,			// statut du LCD
	INT_TIMER = 2,			// �a se comprend tout seul
	INT_SERIAL = 3,		// pas �mul�
	INT_JOYPAD = 4,		// combinaison de touches
	INT_LAST = 5
} cpu_interrupt_t;

/** D�clenche une interruption logicielle, qui va �ventuellement s'ex�cuter sur
	le CPU si l'utilisateur ne l'a pas masqu�e.
	\note le bit correspondant de IF est mis � jour
*/
void cpu_trigger_irq(cpu_interrupt_t int_no);

/** R�cup�re les valeurs des registres du CPU dans un tampon (y.c statut).
	\param buffer tampon d'au moins 14 octets (8 registres + SP, HL, IME, HALT)
	\note Utilis� pour la sauvegarde d'�tat.
*/
unsigned cpu_get_state(u8 *buffer);

/** D�finit les valeurs des registres du CPU depuis un tampon r�cup�r� avec
	#cpu_get_regs (y.c. statut, IME, etc.).
	\param buffer tampon de 14 octets (voir #cpu_get_state)
*/
void cpu_set_state(const u8 *buffer);

/** Decode (d�sassemble) une instruction.
	\param address adresse du bus o� lire l'instruction
	\param name [out] cha�ne contenant l'instruction d�cod�e
	\param length [out] nombre d'octets utilis�s par l'instruction. PC devrait
		ensuite �tre avanc� de ce nombre d'unit�s.
	\param cycles [out] nombre de cycles machine (1 m-cycle = 4 t-cycles sur
		Game Boy) requis pour l'ex�cution de l'instruction
*/
void cpu_disassemble(u16 address, char *name, int *length, int *cycles);

/** Affiche une instruction (format�e) */
void cpu_print_instruction(u16 address);

/** Activer ceci sur un processeur little endian (comme l'Intel x86/x64 cible) */
#define LITTLE_ENDIAN

#endif
