#define numberof(o)		(sizeof(o) / sizeof(*(o)))
// Max char number in a string
#define MAX_TEXT_SIZE 1024

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned long u32;

/** Token types */
typedef enum {
	IDENTIFIER, NUMBER, STRING, DOT,
	COMMA, EOLINE, EOFILE, PLUS,
	COLON, LEFT_PAR, RIGHT_PAR, LEFT_SQBR,
	RIGHT_SQBR, EQUAL
} Type;

/** Token */
typedef struct {
	int line, col;
	Type type;
	char text[MAX_TEXT_SIZE];
	unsigned data;
} Token;

/** Returns true if a parsing error has occured. You should usually stop any
	operation when true.
	\return 0 if no error has occured, nonzero else
*/
extern int isError();
/** Checks whether the next token is of a specified type.
	\param t type to check the next token with
	\return 1 if the token type matches
*/
extern int nextIs(Type t);
/** Reads a token of a specified type. Triggers an error if the token is of
	the wrong type.
	\param t type to check the next token with
	\return pointer to a read token, NULL in case of error
*/
extern Token *read(Type t);
/** Reads a token from the current script.
	\return pointer to a read token
*/
extern Token *readAny();
/** Reads the next token if it is of the specified type. If it is of another
	type, lets the token alone and returns NULL.
	\param t expected token type
	\return pointer to a read token
*/
extern Token *readIf(Type t);
/** Adds an error related to a token.
	\param lex token near which the error happened
	\param msg more explicit error message
*/
extern void errorTok(Token *lex, const char *msg);
/** Called by the parser to handle non built-in functions
	\param tok read instruction token
*/
extern void user_call(Token *tok);
