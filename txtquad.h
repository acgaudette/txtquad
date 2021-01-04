#ifndef TXTQUAD_H
#define TXTQUAD_H

#include "config.h"
#include "alg.h"

#define PIX_WIDTH (1.f / CHAR_WIDTH)
#define LINE_HEIGHT (PIX_WIDTH + 1.f)

struct Extent {
	u16 w;
	u16 h;
};

struct Settings {
	const char *app_name;
	const char *asset_path; // .spv and .pbm files are loaded from here
	enum {
		  MODE_WINDOWED
		, MODE_FULLSCREEN // Note: unsupported aspect ratios will panic
		, MODE_BORDERLESS
	} mode;
	struct Extent win_size; // Ignored for MODE_BORDERLESS
};

struct Frame {
	struct Extent win_size;
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
	float scale; // ... of one char, regardless of CHAR_WIDTH
	u8 v;
	v4 col;
	v2 fx;
};

enum Justify {
	  JUST_LEFT
	, JUST_CENTER
	, JUST_RIGHT
};

struct Block {
	const char *str;
	size_t str_len;
	v3 pos;
	v4 rot;
	float scale;
	v2 piv;
	v2 off;
	enum Justify just;
	v4 col;
	float spacing;
	size_t col_lim; // TODO: parse newline chars
	u8 cursor;
};

struct Text {
	struct Block blocks[MAX_BLCK];
	size_t block_count;
	struct Char chars[MAX_CHAR];
	size_t char_count;
};

__attribute__((weak))
struct Share txtquad_update(struct Frame data, struct Text *text);

void txtquad_init(const struct Settings);
void txtquad_start();

#endif
