#include <windows.h>
#include <stdio.h>
#include "emu.h"

int main(int argc, char *argv[]) {
	emu_load_cart("testprog.gb");
	emu_do_frame();
}

/*#define SCREEN_WIDTH  160*2
#define SCREEN_HEIGHT 144*2

char *pixels;

void display(void) {
	glClearColor(0, 0.5, 0, 0);
	glClear(GL_COLOR_BUFFER_BIT);
	glFinish();
	SwapBuffers(wglGetCurrentDC());
}

int main(int argc, char *argv[]) {
   glutInit(&argc, argv);
   glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE);
   glutInitWindowSize(SCREEN_WIDTH, SCREEN_HEIGHT);
   glutCreateWindow("Heig-boy - testprog.gb");
   glutDisplayFunc(display);
   glutMainLoop();
}*/
