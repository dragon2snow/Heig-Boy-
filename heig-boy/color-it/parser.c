/** \file parser.c

	This file implements the core language lexer and parser. You don't need to
	modify this file.
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "parser.h"
#include "lang.h"

#define MAX_CALL_DEPTH 32

// Variables d'état de lecture
static int curLine, curCol, lastLine, lastCol, error;
static Token last, next;
static FILE *f;
static int callCount, callStack[MAX_CALL_DEPTH];
static const unsigned char *lexNames[] = {
	"identifier", "number", "string", "dot",
	"comma", "end of line", "end of file",
	"'+'", "':'", "'('", "')'", "'['", "']'", "'='"
};

// Signale une erreur par rapport à la position courante
static void errorChar(const char *msg) {
	if (!error)
		ColorIt_ShowError(lastLine+1, lastCol+1, msg);
	error++;
}

// Signale une erreur liée à un jeton en particulier
void errorTok(Token *lex, const char *msg) {
	if (!error)
		ColorIt_ShowError(lex->line+1, lex->col+1, msg);
	error++;
}

int isError() { return error; }

// Retourne le prochain caractère, ou 0 en fin de fichier
static char readRawChar() {
	int c = fgetc(f);
	if (c == EOF)
		return '\0';
	lastLine = curLine, lastCol = curCol;
	curCol++;
	if (c == '\n')
		curLine++, curCol = 0;
	return (char)c;
}

// Retourne le prochain caractère en minuscule
static char readChar() {
	char c = readRawChar();
	if (c >= 'A' && c <= 'Z')
		c -= 'A' - 'a';
	return c;
}

// Annule la saisie du caractère (le remet pour un prochain readChar)
static void putBack(char c) {
	curLine = lastLine, curCol = lastCol;
	ungetc(c, f);
}

// Macro pour copier les caractères tant qu'il correspondent à une condition
// (usage interne pour un lexème). Le caractère c courant est toujours ajouté.
#define readAll(cond) \
	while (charNo < MAX_TEXT_SIZE-1 && (cond)) { \
		next.text[charNo++] = c; \
		c = readChar(); \
	} \
	putBack(c); \
	next.text[charNo] = '\0';

// Lit n'importe quel type de lexème
Token *readAny() {
	char c;
	int charNo = 0;
	// Fichier terminé -> il n'y aura plus rien à lire
	if (last.type == EOFILE)
		return &last;
	// Normal
	last = next;
	c = readChar();
	next.line = lastLine;
	next.col = lastCol;
	// Comment
	if (c == '#' || c == ' ' || c == '\t') {
		while (c == ' ' || c == '\t')
			c = readChar();
		next.col = lastCol;
		if (c == '#') {
			do	c = readChar();
			while (c != '\n');
			next.type = EOLINE;
			return &last;
		}
	}
	// Identifier
	if (c >= 'a' && c <= 'z' || c >= 'A' && c <= 'Z' || c == '_') {
		readAll(c >= 'a' && c <= 'z' || c >= 'A' && c <= 'Z' ||
		        c == '_' || c >= '0' && c <= '9');
		next.type = IDENTIFIER;
	}
	else if (c >= '0' && c <= '9') {
		int mode = '0';
		// Préfixe?
		if (c == '0') {
			c = readChar();
			// Cas 0x123 par exemple
			if (c == 'x' || c == 'b') {
				mode = c;
				c = readChar();
			}
			// Par exemple: cas 0, ou 01234
			else
				next.text[charNo++] = c;
		}
		next.type = NUMBER;
		if (mode == '0') {
			readAll(c >= '0' && c <= '9');
			next.data = atoi(next.text);
		}
		else if (mode == 'x') {
			readAll(c >= '0' && c <= '9' || c >= 'a' && c <= 'f');
			sscanf(next.text, "%x", &next.data);
		}
		else if (mode == 'b') {
			int i, k = 0;
			readAll(c == '0' || c == '1');
			// Conversion binaire
			next.data = 0;
			for (i = charNo - 1; i >= 0; i--)
				next.data |= (c - '0') << k++;
		}
	}
	// String
	else if (c == '"') {
		c = readRawChar();
		while (charNo < MAX_TEXT_SIZE-1 && c != '"' && c != EOF && c != '\n') {
			next.text[charNo++] = c;
			c = readRawChar();
		}
		next.text[charNo] = '\0';
		next.type = STRING;
		if (c != '"')
			errorChar("unterminated string");
	}
	else {
		// Conversion des caractères simples en lexèmes
		const struct {
			char c;
			Type lex;
		} individualChars[] = {
			{'.', DOT},
			{',', COMMA},
			{'\n', EOLINE},
			{'\0', EOFILE},
			{'+', PLUS},
			{':', COLON},
			{'(', LEFT_PAR},
			{')', RIGHT_PAR},
			{'[', LEFT_SQBR},
			{']', RIGHT_SQBR},
			{'=', EQUAL},
		};
		for (charNo = 0; charNo < numberof(individualChars); charNo++)
			if (individualChars[charNo].c == c) {
				next.type = individualChars[charNo].lex;
				return &last;
			}
		errorChar("invalid character");
		next.type = EOFILE;
	}
	return &last;
}

// Lit un jeton d'un certain type
Token *read(Type t) {
	if (next.type != t) {
		char buf[MAX_TEXT_SIZE];
		sprintf(buf, "%s expected, not %s", lexNames[t], lexNames[next.type]);
		errorTok(&next, buf);
		return NULL;
	}
	return readAny();
}

// Lit un jeton seulement s'il est du type en question
Token *readIf(Type t) {
	if (nextIs(t))
		return readAny();
	return NULL;
}

// Retourne vrai si le prochain lexème est du type indiqué
int nextIs(Type t) {
	return next.type == t;
}

// Va à un label défini
// Retourne 0 si pas trouvé
static int doGoto(const char *name, int showError) {
	char read[MAX_TEXT_SIZE];
	int end;
	fseek(f, 0, SEEK_SET);
	curLine = curCol = lastLine = lastCol = 0;
	do {
		// Début de ligne: commence à voir jusqu'où on peut faire correspondre au label
		int i = 0, j = 0;
		char c;
		end = fgets(read, sizeof(read), f) == NULL;
		curLine++;
		// Whitespace au début
		while (read[j] == ' ' || read[j] == '\t')
			j++;
		// Début de la comparaison, n'utilise pas readChar pour des raisons de vitesse
		while (1) {
			c = read[j++];
			if (c >= 'A' && c <= 'Z')
				c -= 'A' - 'a';
			// Tout correspond?
			if (name[i] == '\0' && c == ':') {
				next.type = EOLINE;
				return 1;
			}
			if (c != name[i++])
				break;
		}
	} while (!end);
	// Erreur...
	if (showError) {
		sprintf(read, "label %s not found", name);
		errorChar(read);
	}
	return 0;
}

Token *readLabel() {
	Token *tok = readIf(STRING);
	if (!tok)
		return read(IDENTIFIER);
	return tok;
}

// Démarre l'exécution d'un fichier à un label donné
// Label peut être nul, fileName aussi (= réutiliser le même fichier)
int ColorIt_execScript(const char *fileName, const char *label) {
	Token *tok;
	if (!fileName && !f)
		return 0;
	if (fileName) {
		if (f)
			fclose(f);
		f = fopen(fileName, "r");
		if (!f)
			return 0;
	}
	else
		fseek(f, 0, SEEK_SET);
	// Initialisation
	next.type = EOLINE;		// Jeton générique lorsqu'aucun n'a encore été lu
							// Il ne faut juste pas laisser un EOFILE...
	curLine = curCol = lastLine = lastCol = error = 0;
	if (label)
		if (!doGoto(label, 0))
			return 0;
	do {
		// Instruction?
		if (tok = readIf(IDENTIFIER)) {
			if (nextIs(COLON))		// Label ignorés
				;
			// Fonctions intégrées
			else if (!strcmp(tok->text, "goto")) {
				if (tok = readLabel())
					doGoto(tok->text, 1);
			}
			else if (!strcmp(tok->text, "call")) {
				if (tok = readLabel()) {
					if (callCount < MAX_CALL_DEPTH) {
						callStack[callCount++] = ftell(f);
						doGoto(tok->text, 1);
					}
					else
						errorTok(tok, "call stack overflow");
				}
			}
			else if (!strcmp(tok->text, "end")) {
				if (callCount == 0)		// Fin du script
					return 1;
				else {
					fseek(f, callStack[--callCount], SEEK_SET);
					next.type = EOLINE;
				}
			}
			else
				// Fonctions externes
				user_call(tok);
		}
		else if (!nextIs(LEFT_SQBR) && !nextIs(EOLINE))		// [section]:
			errorTok(&next, "only instruction, label or section is allowed here");
		// Va jusqu'à la fin de la ligne
		while (!readIf(EOLINE) && !nextIs(EOFILE))
			readAny();
	} while (!error && !nextIs(EOFILE));
	fclose(f);
	return 1;
}
