#include "vga.h"

byte *DVGA = (byte *) 0xA0000; /* this points to video memory. */
byte *VGA_BUFFER;

byte DEF_PALETTE[] = {
		0, 0, 0,
		0, 0, 255,
		0, 255, 255,
		255, 0, 0,
		255, 0, 255,
		255, 255, 0,
		0, 0, 0,
		0, 0, 0,
		0, 0, 0,
		0, 0, 0,
		0, 0, 0,
		0, 0, 0,
		0, 0, 0,
		0, 0, 0,
		0, 0, 0,
		255, 255, 255,
};
void vga_init() {
	if (__djgpp_nearptr_enable() == 0) {
		printf("Couldn't get access to first 640K of memory.\n");
		exit(-1);
	}
	VGA_BUFFER = (byte *) malloc(VGA_WIDTH * VGA_HEIGHT);
	DVGA += __djgpp_conventional_base;
	// __djgpp_nearptr_disable();
	return;
}

void vga_set_mode(byte mode) {
	union REGS regs;
	regs.h.ah = SET_MODE;
	regs.h.al = mode;
	int86(VIDEO_INT, &regs, &regs);
}

void vga_plot(int x, int y, byte color) {
	VGA_BUFFER[(y << 8) + (y << 6) + x] = color;
}

void vga_clear(byte color) {
	memset(VGA_BUFFER, color, VGA_WIDTH * VGA_HEIGHT);
}

void vga_swap(int flags) {
	if (flags & VGA_WAIT_RETRACE)
		vga_wait_retrace();
	memcpy(DVGA, VGA_BUFFER, VGA_WIDTH * VGA_HEIGHT);
	if (flags & VGA_CLEAR) {
		vga_clear(0);
	}
}

void vga_wait_retrace() {
	while ((inp(INPUT_STATUS_1) & VRETRACE))
		;
	while (!(inp(INPUT_STATUS_1) & VRETRACE))
		;
}

void vga_set_palette_index(int index, int r, int g, int b) {
	outp(0x03c8, index);
	outp(0x03c9, r >> 2);
	outp(0x03c9, g >> 2);
	outp(0x03c9, b >> 2);
}

void vga_set_palette(byte *palette, byte palette_size) {
	outp(0x03c8, 0);
	if(palette_size > NUM_COLORS - RESERVED_PALETTE)
		raise_error("Invalid Palette Size");

	for (int i = 0; i < RESERVED_PALETTE * 3; i++) {
		outp(0x03c9, DEF_PALETTE[i]);
	}
	for (int i = 0; i < palette_size * 3; i++) {
		outp(0x03c9, palette[i] >> 2);
	}
}

