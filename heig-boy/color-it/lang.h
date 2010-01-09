/** \file lang.h

	This file defines all the functions that you need to implement plus
	exported functions from the parser. You don't need to modify this file.
*/

/** Generic function to execute a script file.
	\param fileName file name of the script to be executed
	\param label label to begin execution from
	\return 0 in case of error (label or file not found), 1 in case of success
*/
extern int ColorIt_execScript(const char *fileName, const char *label);
/** Called when a tile range is added to the CRC.
	\param tileStart first tile (guaranteed <= tileEnd)
	\param tileEnd last tile
	\note Must be implemented by the user
*/
extern void ColorIt_addTileCrc(unsigned tileStart, unsigned tileEnd);
/** Called when a palette is defined.
	\param palNo palette number (0 < palNo < 256)
	\param colors an array of 4 32-bit colors in ABGR format
	\note Must be implemented by the user
*/
extern void ColorIt_setPalette(unsigned palNo, unsigned long *colors);
/** Called when a palette is applied to a tile range.
	\param tileStart first tile (guaranteed <= tileEnd)
	\param tileEnd last tile
	\param palNo palette number to apply to these tiles
	\note Must be implemented by the user
*/
extern void ColorIt_addTileRule(unsigned tileStart, unsigned tileEnd, unsigned palNo);
/** Called when a custom tile is defined.
	\param tileStart first tile (guaranteed <= tileEnd)
	\param tileEnd last tile
	\param palNo palette number to apply to these tiles
	\note Must be implemented by the user
*/
extern void ColorIt_setTilesetData(unsigned ctNo, unsigned char *data);
/** Called when a custom tile is applied to a tile range.
	\param tileStart first tile (guaranteed <= tileEnd)
	\param tileEnd last tile
	\param ctNo custom tile number to apply to these tiles
	\note Must be implemented by the user
*/
extern void ColorIt_setTile(unsigned tileStart, unsigned tileEnd, unsigned ctNo);
/** Called when the auto show CRC command is used.
	\param tileStart first tile (guaranteed <= tileEnd)
	\param tileEnd last tile
	\param palNo palette number to apply to these tiles
	\note Must be implemented by the user
*/
extern void ColorIt_autoShowVramCrc(int value);
extern void ColorIt_messageBox(const char *text);
extern void ColorIt_ShowError(int line, int col, const char *msg);
