#include "emu.h"
#include "sound.h"
#include <windows.h>
#include <mmsystem.h>
#include <stdio.h>

#define NB_BUFFERS 12		// 8 assez?
#define SAMPLES_PER_BUF 256

static DWORD WINAPI win32_sound_thread(LPVOID lpParam);
static HWAVEOUT hWaveOut;
static s16 buffer[NB_BUFFERS][SAMPLES_PER_BUF][2];	// 2 16-bit stereo buffers
static WAVEHDR header[NB_BUFFERS];

int main(int argc, char *argv[]) {
	DWORD thread_id;
	HANDLE thread_handle;
	thread_handle = CreateThread(NULL, 0, win32_sound_thread, NULL, 0, &thread_id);
	emu_load_cart("roms/sml.gb");
	while (true)
		emu_do_frame();
}

static void win32_sound_init() {
	WAVEFORMATEX wfx;
	HWND hwnd;
	WNDCLASSEX wcex;
	wfx.nSamplesPerSec = 44100;
	wfx.wBitsPerSample = 16;
	wfx.nChannels = 2;
	wfx.cbSize = 0;		// size of _extra_ info
	wfx.wFormatTag = WAVE_FORMAT_PCM;
	wfx.nBlockAlign = (wfx.wBitsPerSample >> 3) * wfx.nChannels;
	wfx.nAvgBytesPerSec = wfx.nBlockAlign * wfx.nSamplesPerSec;

	// OS de merde, on est OBLIGÉ de créer une fenêtre pour gérer les
	// événements -_-
	ZeroMemory(&wcex, sizeof(wcex));
	wcex.cbSize = sizeof(WNDCLASSEX); 
	wcex.lpfnWndProc = (WNDPROC)DefWindowProc;
	wcex.hInstance = GetModuleHandle(0);
	wcex.lpszClassName = "billouOnTaime";
	RegisterClassEx(&wcex);
	hwnd = CreateWindow("billouOnTaime", "", 0, CW_USEDEFAULT, 0, 0, 0, NULL, NULL, GetModuleHandle(0), NULL);

	if (waveOutOpen(&hWaveOut, WAVE_MAPPER, &wfx, (DWORD_PTR)hwnd, 0, CALLBACK_WINDOW) != MMSYSERR_NOERROR) {
		fprintf(stderr, "unable to open WAVE_MAPPER device\n");
		return;
	}
}

DWORD WINAPI win32_sound_thread(LPVOID lpParam) {
	unsigned i;
//	FILE *f = fopen("out.raw", "wb");
	MSG msg;
	memset(buffer, 0, sizeof(buffer));
	win32_sound_init();
	while (GetMessage(&msg, NULL, 0, 0)) {
		switch (msg.message) {
			case MM_WOM_OPEN:
				// Prépare les buffers
				for (i = 0; i < NB_BUFFERS; i++) {
					memset(&header[i], 0, sizeof(WAVEHDR));
					header[i].lpData = (LPSTR)buffer[i];
					header[i].dwBufferLength = sizeof(buffer[i]);
					waveOutPrepareHeader(hWaveOut, &header[i], sizeof(WAVEHDR));
					waveOutWrite(hWaveOut, &header[i], sizeof(WAVEHDR));
				}
				break;
			case MM_WOM_DONE:
			{
				WAVEHDR *hdr = (WAVEHDR*)msg.lParam;
				sound_render((s16*)hdr->lpData, SAMPLES_PER_BUF);
//				fwrite(hdr->lpData, 4, SAMPLES_PER_BUF, f);
				waveOutWrite(hWaveOut, hdr, sizeof (WAVEHDR));
				break;
			}
		}
	}
//	fclose(f);
	return 0;
}
