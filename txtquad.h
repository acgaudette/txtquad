#ifndef TXTQUAD_H
#define TXTQUAD_H

#include "config.h"
#include "alg.h"

#define PIX_WIDTH (1.f / CHAR_WIDTH)
#define LINE_HEIGHT (PIX_WIDTH + 1.f)

struct extent {
	u16 w;
	u16 h;
};

struct txt_cfg {
	const char *app_name;
	const char *asset_path; // .spv and .pbm files are loaded from here
	enum {
		  MODE_WINDOWED
		, MODE_FULLSCREEN // Note: unsupported aspect ratios will panic
		, MODE_BORDERLESS
	} mode;
	struct extent win_size; // Ignored for MODE_BORDERLESS
};

/* Per-frame data */

struct txt_frame {
	size_t i;
	float t;
	float t_prev;
	float dt;
	struct extent size;
#ifdef TXT_DEBUG
	float acc;
	size_t i_last;
#endif
};

struct txt_share {
	m4 vp;

	union {
		// Common use case, but entirely optional;
		// note that you must fill these yourself!
		struct {
			v2 screen;
			float time;
			float _[5];
		};

		float _extra[8];
	};
};

struct txt_buf {
	size_t count;
	struct txt_quad {
		u8  value;
		m4  model;
		v4  color;
		v2 _extra;
	} quads[MAX_QUAD];
};

/*
 * User callback; must be implemented by consumer (you)
 */
__attribute__((weak))
struct txt_share txtquad_update(struct txt_frame, struct txt_buf*);

/*
 * Public API
 */
void txtquad_init(const struct txt_cfg);
void txtquad_start();

#endif
