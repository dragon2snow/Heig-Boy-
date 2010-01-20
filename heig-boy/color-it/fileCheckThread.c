/** \file fileCheckThread.cpp

	VisualBoyAdvance and Windows-specific file to allow for automatic reloading
	when detecting a modification of the current loaded file.
*/
#include <stdio.h>
#include <string.h>
#include <windows.h>
#include <process.h>
#include "user.h"

static char fileName[1024], fullName[1024];
static HANDLE hCheckThread = NULL;
int ColorIt_reload = 0;

extern void extractPath(char *dest, const char *fileName);

// Check when the configuration file is modified
DWORD WINAPI FileCheckThread(LPVOID lpParam)
{
	char dirName[1024];
	FILE_NOTIFY_INFORMATION Buffer[1024];
	DWORD BytesReturned;
	HANDLE hDir;
	wchar_t lowerFname[1024];
	int i = 0;

	extractPath(dirName, fullName);
	hDir = CreateFile((char*)dirName,		// File name
		FILE_LIST_DIRECTORY,                // access (read/write) mode
		FILE_SHARE_READ|FILE_SHARE_DELETE,  // share mode
		NULL,                               // security descriptor
		OPEN_EXISTING,                      // how to create
		FILE_FLAG_BACKUP_SEMANTICS,         // file attributes
		NULL                                // file with attributes to copy
		);

	// Convert to wide char
	do {
		lowerFname[i] = fileName[i];
	} while (fileName[i++]);
	wcslwr(lowerFname);

	while (ReadDirectoryChangesW(
		hDir,                                 // handle to directory
		&Buffer,                              // read results buffer
		sizeof(Buffer),                       // length of buffer
		FALSE,                                // monitoring option
		FILE_NOTIFY_CHANGE_SECURITY|
		FILE_NOTIFY_CHANGE_CREATION|
		FILE_NOTIFY_CHANGE_LAST_ACCESS|
		FILE_NOTIFY_CHANGE_LAST_WRITE|
		FILE_NOTIFY_CHANGE_SIZE|
		FILE_NOTIFY_CHANGE_ATTRIBUTES|
		FILE_NOTIFY_CHANGE_DIR_NAME|
		FILE_NOTIFY_CHANGE_FILE_NAME,         // filter conditions
		&BytesReturned,                       // bytes returned
		NULL,                                 // overlapped buffer
		NULL                                  // completion routine
		))
	{
		i = 0;
		do {
			wchar_t str[1024];
			memcpy(str, Buffer[i].FileName, Buffer[i].FileNameLength);
			str[Buffer[i].FileNameLength / sizeof(str[0])] = 0;
			wcslwr(str);
			if (!wcscmp(str, lowerFname)) {
				// It's the right file
				if (Buffer[0].Action == FILE_ACTION_MODIFIED)
					ColorIt_reload = 1;
			}
		} while (!Buffer[++i].NextEntryOffset);
	}
	return 0;
}

void startCheckThread(const char *cartName, const char *fullPath) {
	DWORD threadId;
	strcpy(fileName, cartName);
	strcpy(fullName, fullPath);
	// Terminate the thread when loading another game
	if (hCheckThread) {
		GetExitCodeThread(hCheckThread, &threadId);
		TerminateThread(hCheckThread, threadId);
	}
	hCheckThread = CreateThread(NULL, 0, FileCheckThread, NULL, 0, &threadId);
}
