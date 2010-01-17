/** To be called when loading a new ROM (from ColorIt_init) */
extern void ColorIt_systemInit();
/** Call this when the LCD state switches from on to off. */
extern void ColorIt_exitingLcdc(const unsigned char *vram);
/** Call this at the end of a frame. */
extern void ColorIt_endFrame();
/** Call this at the end of your application. */
extern void ColorIt_stop();
