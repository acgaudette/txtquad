#ifndef TXTQUAD_H
#define TXTQUAD_H

#include "alg.h"

#define WIDTH 512
#define HEIGHT 512

#define PIX_WIDTH (1.f / 8.f)
#define LINE_HEIGHT (PIX_WIDTH + 1.f)

#define MAX_CHAR 4096
#define MAX_BLCK 1024

struct Frame {
	size_t i;
	size_t last_i;
	float t;
	float last_t;
	float dt;
#ifdef DEBUG
	float acc;
#endif
};

struct Share {
	m4 vp;
};

struct Char {
	v3 pos;
	v4 rot;
	float scale;
	char v;
	v4 col;
};

enum Justify {
	  JUST_LEFT
	, JUST_CENTER
	, JUST_RIGHT
};

struct Block {
	char *str; // TODO: fx/color tags?
	size_t str_len;
	v3 pos;
	v4 rot;
	float scale;
	v2 piv;
	v2 off;
	enum Justify just;
	v4 col; // TODO: allow for mottle variation
	float spacing;
	size_t col_lim; // TODO: parse newline chars
	char cursor;
};

struct Text {
	struct Block blocks[MAX_BLCK];
	size_t block_count;
	struct Char chars[MAX_CHAR];
	size_t char_count;
};

struct Share update(struct Frame data, struct Text *text);

void txtquad_init();
void txtquad_start();

#endif
