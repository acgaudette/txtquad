#include <stdlib.h>
#include <math.h>
#include "txtquad/txtquad.h"
#include "txtquad/inp.h"

#ifdef DEMO_1
# elif DEMO_2
# elif DEMO_3
	#define DEBUG_UI
	#include "txtquad/extras/block.h"

	static char cli[1024];
	static size_t cli_len;

	void inp_ev_text(unsigned int unicode)
	{
		char ascii = 'z' >= unicode && unicode >= 'a' ?
			unicode ^ ' ' : unicode;
		cli[cli_len++] = ascii;
	}
#elif DEMO_4
	#include "txtquad/extras/sprite.h"
	void inp_ev_text(unsigned int _) { }
#elif DEMO_5
	#include "acg/sys.h"
#else
	#error invalid demo selection
	void inp_ev_text(unsigned int _) { }
#endif

struct txt_share txtquad_update(struct txt_frame frame, struct txt_buf *txt)
{
	v3 cam_pos = V3_ZERO;
	v4 cam_rot = QT_ID;

#ifdef DEMO_0 // Baseline
	cam_pos = v3_neg(V3_FWD);

	txt->count = 1;
	txt->quads[0] = (struct txt_quad) {
		.value = 'A',
		.model = m4_model(
			(v3) { -.5f + PIX_WIDTH * .5f, -.5f, 0 }
			, QT_ID
			, 1.f
		),
		.color = V4_ONE,
	};
#elif DEMO_1
	char a = 'A' + (1 - .5f * (1 + cos(frame.t * .5f))) * 26 + 0;
	char z = 'Z' - (1 - .5f * (1 + cos(frame.t * .5f))) * 26 + 1;

	txt->count = 4;
	txt->quads[0] = (struct txt_quad) {
		.value = a,
		.model = m4_model(
			(v3) { -.5f - .125f, .5f - .125f, 2.f }
			, QT_ID
			, .25f
		),
		.color = V4_ONE,
	};
	txt->quads[1] = (struct txt_quad) {
		.value = z,
		.model = m4_model(
			(v3) { .5f - .125f, .5f - .125f, 2.f }
			, QT_ID
			, .25f
		),
		.color = V4_ONE,
	};
	txt->quads[2] = (struct txt_quad) {
		.value = z,
		.model = m4_model(
			(v3) { -.5f - .125f, -.5f - .125f, 2.f }
			, QT_ID
			, .25f
		),
		.color = V4_ONE,
	};
	txt->quads[3] = (struct txt_quad) {
		.value = a,
		.model = m4_model(
			(v3) { .5f - .125f, -.5f - .125f, 2.f }
			, QT_ID
			, .25f
		),
		.color = V4_ONE,
	};
#elif DEMO_2
	cam_rot = qt_axis_angle(V3_FWD, frame.t);
	cam_pos = qt_app(cam_rot, v3_genz(-1.5f));

	const float a = cosf(2.f * frame.t) * .5f + .5f;
	const float y = cosf(frame.t);
	const float w = 1.f - (1.2f * (a - 1e-2 * .5f));

	txt->count = 1;
	txt->quads[0] = (struct txt_quad) {
		.value = y > 0.f ? 'Q' : 'R',
		.model = m4_model(
			(v3) { -.5f + PIX_WIDTH * .5f, -.5f, 0.f }
			, QT_ID
			, 1.f
		),
		.color = y > 0.f ?
			(v4) { 1.f, .8f, .4f, w } :
			(v4) { 1.f, .4f, .8f, w } ,
	};
#elif DEMO_3
	if (KEY_DOWN(ENTER)) {
		cli[cli_len++] = '\n';
	}

	if (KEY_DOWN(BACKSPACE)) {
		if (cli_len) --cli_len;
	}

	if (KEY_DOWN(ESCAPE)) {
		cli_len = 0;
	}

	*(cli + cli_len) = 0;
	txt->count = 0;

	struct block_ctx ctx = block_prepare(
		(struct block) {
			.str = cli,
			.scale = .25f,
			.pos = { 0.f, -.9f, 2.f },
			.rot = qt_axis_angle(V3_RT, M_PI * .15f),
			.anch = { 0.f, -1.f },
			.justify = JUST_LEFT,
			.spacing = 1.f,
			.line_height = 1.f,
		}
	);

	struct sprite sprite;
	while (block_draw(&sprite, &ctx, txt)) {
		assert(sprite.asc);
		sprite.col = V3_ONE;
		txt->quads[txt->count++] = sprite_conv(sprite);
	}

	// Cursor
	sprite.asc = fmodf(frame.t, 1.f) > .5f ? '_' : ' ';
	txt->quads[txt->count++] = sprite_conv(sprite);
#elif DEMO_4
	txt->count = 0;
	const fff pos = { 0.f, 0.f, 1.f };

	sprite_draw_imm(
		(struct sprite) {
			.anch = { 1.f, -1.f },
			.scale = .5f,
			.pos = pos,
			.rot = qt_axis_angle(V3_FWD, sinf(frame.t) * .5f),
			.col = { .8f, .3f, .3f },
			.vfx = V3_RT,
			.asc = '1',
			.bounds = BOUNDS_FONT,
		}, txt
	);

	sprite_draw_imm(
		(struct sprite) {
			.anch = { -1.f, 1.f },
			.scale = .25f,
			.pos = pos,
			.rot = qt_axis_angle(V3_FWD, -sinf(frame.t) * .5f),
			.col = { .8f, .8f, .8f },
			.vfx = V3_RT,
			.asc = '4',
			.bounds = BOUNDS_FONT,
		}, txt
	);

	for (unsigned i = 0; i < 4; ++i) {
		sprite_draw_imm(
			(struct sprite) {
				.anch = V2_ZERO,
				.scale = 1.f,
				.pos = V3_FWD,
				.rot = qt_axis_angle(V3_FWD, M_PI * .5f * i),
				.col = { .1f, .1f, .2f },
				.vfx = V3_RT,
				.asc = '!',
				.bounds = mulffff(
					(v4) { 2.f, 3.f, 4.f, 0.f },
					PIX_WIDTH
				),
			}, txt
		);
	}
#elif DEMO_5
	const size_t waterline = MAX_QUAD;
	txt->count = waterline;

	const fff origin = V3_FWD;
	const float scale = .1f;
	const float depth = waterline / 60 + 1;

	for (size_t i = 0; i < waterline; ++i) {
		float x = i % 10, y = (i % 60) / 10, z = i / 60;

		fff pos = {
			-.95f + x * 2.f * scale,
			-.55f + y * 2.f * scale,
			1.f + .5f * scale * z,
		};

		ffff col = {
			x / 10.f,
			y / 6.f,
			1.f - (i / 54) / (depth * !((i / 54) % 4)),
			.5f + fmodf(frame.t * .1f, 1.f) * .5f,
		};

		v4 rot = qt_axis_angle(V3_FWD, (i % 4) * M_PI * .5f);

		txt->quads[i] = (struct txt_quad) {
			.value = 1,
			.model = m4_model(pos, rot, scale),
			.color = col,
		};
	}

	if (frame.i > 32 && frame.t > 3.f && frame.dt > 1.f / 60.f + 1e-3) {
		fprintf(stderr, "dt=%f (%.1f)\n", frame.dt, 1.f / frame.dt);
		panic_msg("performance barrier reached");
	}
#endif
	float asp = (float)frame.size.w / frame.size.h;
	m4 view = m4_view(cam_pos, cam_rot);
	m4 proj = m4_persp(60, asp, .001f, 1024);
	m4 vp = m4_mul(proj, view);

	return (struct txt_share) {
		.vp = vp,
		.screen = { frame.size.w, frame.size.h },
		.time = frame.t,
	};
}

int main()
{
#ifndef DEMO_3
	inp_key_init(NULL, 0);
#else
	int inp_handles[] = {
		  KEY(ENTER)
		, KEY(BACKSPACE)
		, KEY(ESCAPE)
	};

	inp_key_init(inp_handles, sizeof(inp_handles) / sizeof(int));
#endif
	struct txt_cfg cfg = {
		.app_name = "txtquad-demo",
		.asset_path = "./assets/",
		.win_size = { 800, 800 }, // Ignored
		.mode = MODE_BORDERLESS,
		.cursor = CURSOR_SCREEN,
	};

	txtquad_init(cfg);
	txtquad_start();
	exit(0);
}
