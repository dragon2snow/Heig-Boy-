#include <stdio.h>
#include <sys/time.h>
#include "os_specific.h"
#include "core/emu.h"

u64 time() {
	struct timeval t;
	gettimeofday(&t, NULL);
	return (u64)t.tv_sec * 1000000 + t.tv_usec;
}

int main(int argc, char *argv[]) {
	u64 lastTime = time();
	if (argc < 2) {
		printf("Usage: heig-boy romname\n");
		return 0;
	}
	if (!emu_load_cart(argv[1])) {
		printf("%s: failed to load ROM\n", argv[1]);
		return 0;
	}
	sound_driver_init();
	while (true) {
		emu_do_frame(true);
		while (time() - lastTime < 16667);
		lastTime += 16667;
	}
	return 0;
}
