#include <stdio.h>
#include <math.h>
#include <unistd.h>
#include <pc.h>
#include <time.h>
#include <string.h>

#include "types.h"
#include "vga.h"
#include "util.h"
#include "decoder.h"
#include "keyboard.h"
#include "font.h"
#include "wav.h"

#define MSPERSEC 1000

#define SPEAKER_PORT 	0x61
#define PIT_CONTROL 	0x43
#define PIT_CHANNEL_2	0x42
#define PIT_FREQ		0x1234DD

#define AUDIO_BUF_SZ 		65536
#define VIDEO_BUF_SZ		1

extern byte *VGA_BUFFER;
extern byte *DVGA;

byte mute = 0;
byte skip = 0;
byte vsync = 0;
long long shift = 0;

byte video_data[VGA_WIDTH * VGA_HEIGHT * VIDEO_BUF_SZ];
byte palette_data[768 * VIDEO_BUF_SZ];
byte palette_sizes[VIDEO_BUF_SZ];
byte audio_data[AUDIO_BUF_SZ];

int main() {
	hook_keyb_handler();
	vga_init();
	vga_set_mode(VGA_256_COLOR_MODE);

	FILE* video_file = fopen("video.bin", "rb");
	struct video_header vh = decode_video_header(video_file);
	decode_video_reset();
	font_text(0, 0, ">>", 1);
	int last_video_frame = 0;

	/* AUDIO EVENT SETUP */
	FILE *wav_file = fopen("audio.wav", "rb");
	struct wav_header wh = wav_readheader(wav_file);

	// calculate timing data
	int clocks_per_sample = PIT_FREQ / wh.SampleRate;
	byte quantmap[256];
	for (int i = 0; i < 256; i++)
		quantmap[i] = i * clocks_per_sample / 255;

	// connect PIT timer to speaker
	outportb(PIT_CONTROL, 0x90);
	outportb(SPEAKER_PORT, inportb(SPEAKER_PORT) | 3);
	/* END AUDIO EVENT SETUP */

	// read, decode, evaluate
	uclock_t last_sec = uclock();
	long long uclockspersample = UCLOCKS_PER_SEC / wh.SampleRate;
	int video_offset = 0;

	int frame_ratio = wh.SampleRate / vh.frame_rate;

	for (int f = 0; f < wh.Subchunk2Size; f++) {
		last_sec = uclock();

		// video processing
		if (f % frame_ratio == 0) {
			// decode new frames
			if (video_offset % VIDEO_BUF_SZ == 0) {
				for (int i = 0;
						i < VIDEO_BUF_SZ && video_offset + i < vh.frame_count;
						i++) {
					decode_video_frame(video_file, palette_data + i * 768,
							palette_sizes + i,
							video_data + i * VGA_WIDTH * VGA_HEIGHT);
				}
			}
			skip = key_pressed(0x1d);

			if (key_pressed(0x01))	// exit condition
				break;

			if (key_pressed(0x10))
				shift++;
			if (key_pressed(0x1E))
				shift--;
			video_offset++;
		}

		// only change palette during vertical retraces
		int vstate = inp(INPUT_STATUS_1) & VRETRACE;
		if (vstate) {
			if (vsync == 1) {
				int modoffset = video_offset % VIDEO_BUF_SZ;
				if (last_video_frame != video_offset) {
					vga_set_palette(palette_data + modoffset * 768,
							*(palette_sizes + modoffset));
					memcpy(DVGA,
							video_data + modoffset * VGA_WIDTH * VGA_HEIGHT,
							VGA_WIDTH * VGA_HEIGHT);
					printf("\rF%d S%lld", f, shift);
					last_video_frame = video_offset;
				}
			}
			vsync = 0;
		} else {
			vsync = 1;
		}

		// audio processing
		if (f % AUDIO_BUF_SZ == 0) {
			wav_readdata(wav_file, audio_data, 0, AUDIO_BUF_SZ);
		}

		outportb(PIT_CHANNEL_2, quantmap[audio_data[f % AUDIO_BUF_SZ]]);

		if (!skip) {
			uclock_t trigger = last_sec + uclockspersample + shift;
			while (uclock() < trigger) {
				asm("");
				// prevent the compiler from optimizing this away
			}
		}

	}
	fclose(video_file);
	fclose(wav_file);

	// reset speaker
	outportb(SPEAKER_PORT, inportb(SPEAKER_PORT) & 0xFC);
	outportb(PIT_CONTROL, 0xB6);
	vga_set_mode(TEXT_MODE);
	return 0;
}
