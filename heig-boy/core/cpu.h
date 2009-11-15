#ifndef CPU_H
#define CPU_H

#include "common.h"

/*	De la synchronisation des composants.

	La Game Boy contient plusieurs composants qui fonctionnent à leur rythme,
	mais qui sont tous synchronisés sur une horloge: l'oscillateur (OSC clock).
	Cette horloge est cadencée à 4 MHz et tous les timings peuvent être
	exprimés en fonction de cette horloge, notamment:
	- 4 cycles pour 1 cycle machine du processeur
	- 456 cycles pour le balayage d'une ligne de l'écran LCD
	- 32 cycles pour la génération d'une sample audio
	- ...
	Ainsi une façon d'émuler le parallélisme des composants sans utiliser un
	système de threads très complexe et très lent à l'exécution, est de tout
	simplement émuler la synchronisation par l'oscillateur!
	
	On a donc une boucle principale qui fait avancer l'horloge d'un cycle, et
	le transmet à tous les composants. Les composants vont alors réaliser la
	part de travail qui leur incombe durant ce laps de temps.
	Par exemple le CPU exécutera une instruction ou finira d'exécuter celle
	qu'il a en cours (une instruction durant plusieurs cycles); le contrôleur
	son générera une "sample" audio ou attendra un peu pour générer la
	prochaine.
	On a alors une machine d'état avec un système événementiel pour chaque
	composant, dont le schéma en pseudo-code objet est ainsi:

	Composant:
		temps_dodo: entier := 0
		méthode tick():			// Un cycle d'horloge s'est passé
			si temps_dodo > 0:
				temps_dodo--	// Rien à faire
			sinon:
				traite un événement (réalise une action)
				temps_dodo := temps que dure cette action

		méthode idle_cycles():	// cycles avant le prochain événement
			retourne temps_dodo

	CPU, LCD, Son: Composants
	Boucle principale:
		CPU.tick()
		LCD.tick()
		Son.tick()
		...

	Notez le raccourci! On n'a pas divisé l'action en le nombre de cycles
	qu'elle prendrait sur le matériel. En revanche on l'exécute d'un seul coup
	puis on met le composant en pause pour simuler l'exécution inachevée de
	celle-ci.

	Ce modèle est joli mais il peut toutefois comporter un problème: celui de
	la performance du fait du nombre élevé d'exécutions de la boucle principale
	(4 MHz = 4 millions de fois par seconde). Surtout quand on sait que les
	instructions du CPU prennent au moins 4 cycles, et peuvent même en durer
	24; on n'aurait pas forcément besoin d'être aussi précis.

	Une optimisation possible est alors de n'envoyer des ticks que
	périodiquement, en indiquant au composant le nombre de cycles qui se sont
	écoulés depuis la dernière fois.
	Et comme les événements se produisant le plus souvent sont l'exécution
	d'une instruction, pourquoi ne pas le prendre en référence? La boucle
	principale ressemblerait alors à ceci:

	CPU, LCD, Son: Composants
	Boucle:
		// execInstruction retourne le # de cycles requis par l'instruction
		écoulés: entier := CPU.execInstruction()
		LCD.tick(écoulés)
		Son.tick(écoulés)
		...

	D'où...
	Les composants ont tous une architecture similaire:
	void ***_tick(int cycles);
	int ***_idle_cycles();
	La première fait "avancer" le temps d'un certain nombre de cycles. Par
	exemple dans le cas du timer cela le fera avancer s'il est activé.
	La deuxième retourne le nombre de cycles pendant lequel le composant
	est inactif (temps avant le prochain événement le concernant).

	Les composants, à part le CPU qui est toujours actif, réalisent
	périodiquement des tâches puis "s'endorment" pendant un moment (par exemple
	l'écran dessine une ligne puis attend un signal de synchronisation avant la
	prochaine).

	Ainsi la fonction "tick" doit être appelée aussi souvent que possible pour
	mettre à jour l'état des composants (si le timer est mis à jour trop peu
	souvent et affiche des valeurs telles que 1, 4, 8, 11, etc. l'émulation
	sera probablement peu réaliste), mais toutefois il n'est pas non plus
	nécessaire de la rappeler avant que "idle_cycles" soit zéro, ainsi on
	pourrait imaginer réduire la boucle principale en utilisant cette notion
	d'événements:

	Boucle:
		// Calcule l'événement le plus proche
		cycles_libres := min(sound_idle_cycles, lcd_idle_cycles, ...)
		// Ensuite exécute le CPU jusqu'à cet événement
		cycles_exécutés := 0
		Tant que cycles_exécutés < cycles_libres:
			cycles_exécutés += cpu_exec_instruction()
		// Met à jour les composants (il y en a au moins un qui a quelque
		// chose à faire).
		lcd_tick(cycles_exécutés)
		sound_tick(cycles_exécutés)
		...

	Le fait que le CPU soit "maître" suppose que rien ne se passe pendant
	l'exécution de l'instruction qui ait un effet sur celle-ci. En fait même si
	cette assomption est fausse, il serait en fait impossible de déterminer
	exactement comment le matériel réagirait, ainsi on considérera que c'est le
	cas.
*/

/** Met le CPU dans l'état initial (au démarrage de la Game Boy) */
void cpu_init();

/** Exécute une instruction CPU et donne le nombre de cycles "mangés". Il
	faudrait faire avancer le reste du hardware du même nombre d'unités et
	vérifier si des événements ont lieu avant de continuer à exécuter la
	prochaine instruction.
	\return le nombre de cycles utilisés par l'instruction
*/
unsigned cpu_exec_instruction();

/** Types d'interruptions possibles. */
typedef enum {
	INT_VBLANK = 0,		// période de VBLANK du LCD
	INT_STAT = 1,			// statut du LCD
	INT_TIMER = 2,			// ça se comprend tout seul
	INT_SERIAL = 3,		// pas émulé
	INT_JOYPAD = 4,		// combinaison de touches
	INT_LAST = 5
} cpu_interrupt_t;

/** Déclenche une interruption logicielle, qui va éventuellement s'exécuter sur
	le CPU si l'utilisateur ne l'a pas masquée.
	\note le bit correspondant de IF est mis à jour
*/
void cpu_trigger_irq(cpu_interrupt_t int_no);

/** Récupère les valeurs des registres du CPU dans un tampon (y.c statut).
	\param buffer tampon d'au moins 14 octets (8 registres + SP, HL, IME, HALT)
	\note Utilisé pour la sauvegarde d'état.
*/
unsigned cpu_get_state(u8 *buffer);

/** Définit les valeurs des registres du CPU depuis un tampon récupéré avec
	#cpu_get_regs (y.c. statut, IME, etc.).
	\param buffer tampon de 14 octets (voir #cpu_get_state)
*/
void cpu_set_state(const u8 *buffer);

/** Decode (désassemble) une instruction.
	\param address adresse du bus où lire l'instruction
	\param name [out] chaîne contenant l'instruction décodée
	\param length [out] nombre d'octets utilisés par l'instruction. PC devrait
		ensuite être avancé de ce nombre d'unités.
	\param cycles [out] nombre de cycles machine (1 m-cycle = 4 t-cycles sur
		Game Boy) requis pour l'exécution de l'instruction
*/
void cpu_disassemble(u16 address, char *name, int *length, int *cycles);

/** Affiche une instruction (formatée) */
void cpu_print_instruction(u16 address);

/** Activer ceci sur un processeur little endian (comme l'Intel x86/x64 cible) */
#define LITTLE_ENDIAN

#endif
