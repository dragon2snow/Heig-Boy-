#include "debug.h"
#include <stdio.h>
#include <stdarg.h>		// va_arg
#include <stdlib.h>		// pause
#include "os_specific.h"

static void debug(int col, const char *prefix, const char *fmt, va_list ap) {
    char buf[128];
    vsprintf(buf, fmt, ap);
	set_text_color(col);
	printf("%s: %s\n", prefix, buf);
   set_text_color(COL_NORMAL);
}

void dbg_info(const char *fmt, ...) {
	va_list ap;
	va_start(ap, fmt);
	debug(COL_GREEN, "Info", fmt, ap);
	va_end(ap);
}
void dbg_warning(const char *fmt, ...) {
	va_list ap;
	va_start(ap, fmt);
	debug(COL_YELLOW, "Warning", fmt, ap);
	va_end(ap);
	system("pause");
}
void dbg_error(const char *fmt, ...) {
	va_list ap;
	va_start(ap, fmt);
	debug(COL_RED, "Error", fmt, ap);
	va_end(ap);
	system("pause");
}
