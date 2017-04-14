#ifndef DECODER_H_
#define DECODER_H_

#include <stdio.h>
#include <string.h>
#include <unistd.h> // todo remove
#include <stdlib.h> // remove
#include "types.h"

#define HEADER_DATA_SIZE 	3
#define FRAME_DATA_SIZE 	2
#define CHUNK_DATA_SIZE		3

struct header_data {
	word frame_count;
	byte frame_rate;
};

struct frame_data {
	word chunk_count;
};

struct chunk_data {
	byte data;
	word repeat;
};

struct header_data decode_header(FILE *file);
struct frame_data decode_frame(FILE *file, byte* dest);

#endif