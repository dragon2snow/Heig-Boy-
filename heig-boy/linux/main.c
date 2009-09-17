#include <stdio.h>
#include <GL/glut.h>

#define SCREEN_WIDTH  160*2
#define SCREEN_HEIGHT 144*2

char *pixels;

void display(void) {
	glClearColor(0, 0.5, 0, 0);
	glClear(GL_COLOR_BUFFER_BIT);
	glFinish();
	// Equivalent sous linux?
//	SwapBuffers(wglGetCurrentDC());
}

int main(int argc, char *argv[]) {
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_RGB /*| GLUT_DOUBLE*/);
	glutInitWindowSize(SCREEN_WIDTH, SCREEN_HEIGHT);
	glutCreateWindow("Heig-boy");
	glutDisplayFunc(display);
	glutMainLoop();
	return 0;
}
