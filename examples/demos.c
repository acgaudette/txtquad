#include <stdlib.h>
#include <math.h>
#include "txtquad.h"
#include "inp.h"

#ifdef DEMO_3
static char cli[1024];
static size_t cli_len;

void inp_ev_text(unsigned int unicode)
{
	char ascii = unicode > 'Z' && unicode <= 'z' ? unicode - 32 : unicode;
	cli[cli_len++] = ascii;
}
#else
void inp_ev_text(unsigned int _) { }
#endif

struct txt_share txtquad_update(struct txt_frame frame, struct txt_buf *txt)
{
	v3 cam_pos = v3_zero();
	v4 cam_rot = qt_id();

#ifdef DEMO_0 // Baseline
	cam_pos = v3_neg(v3_fwd());

	txt->count = 1;
	txt->quads[0] = (struct txt_quad) {
		.value = 'A',
		.model = m4_model(
			(v3) { -.5f + PIX_WIDTH * .5f, -.5f, 0 },
			qt_id(),
			1.f
		),
		.color = v4_one(),
	};
#elif DEMO_1
	char a = 'A' + (1 - .5f * (1 + cos(frame.t * .5f))) * 26 + 0;
	char z = 'Z' - (1 - .5f * (1 + cos(frame.t * .5f))) * 26 + 1;

	txt->count = 4;
	txt->quads[0] = (struct txt_quad) {
		.value = a,
		.model = m4_model(
			(v3) { -.5f - .125f, .5f - .125f, 2 }
			, qt_id()
			, .25f
		),
		.color = v4_one(),
	};
	txt->quads[1] = (struct txt_quad) {
		.value = z,
		.model = m4_model(
			(v3) { .5f - .125f, .5f - .125f, 2 }
			, qt_id()
			, .25f
		),
		.color = v4_one(),
	};
	txt->quads[2] = (struct txt_quad) {
		.value = z,
		.model = m4_model(
			(v3) { -.5f - .125f, -.5f - .125f, 2 }
			, qt_id()
			, .25f
		),
		.color = v4_one(),
	};
	txt->quads[3] = (struct txt_quad) {
		.value = a,
		.model = m4_model(
			(v3) { .5f - .125f, -.5f - .125f, 2 }
			, qt_id()
			, .25f
		),
		.color = v4_one(),
	};
#elif DEMO_2
	cam_rot = qt_axis_angle(v3_up(), sinf(frame.t));
	cam_pos = qt_app(cam_rot, v3_neg(v3_fwd()));

	const float a = cosf(2.f * frame.t) * .5f + .5f;
	const float y = cosf(frame.t);

	txt->count = 1;
	txt->quads[0] = (struct txt_quad) {
		.value = y > 0.f ? 'Q' : 'S',
		.model = m4_model(
			(v3) { -.5f + PIX_WIDTH * .5f, -.5f, .5f }
			, qt_id()
			, 1.f
		),
		.color = y > 0.f ?
			(v4) { 1.f, .8f, .4f, 1.1f * a * a } :
			(v4) { 1.f, .4f, .8f, 1.1f * a * a } ,
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

	txt->count = 0;
/*
	txt->block_count = 1;
	txt->blocks[0] = (struct Block) {
		.str = cli,
		.str_len = cli_len,
		.pos = { -.9f, -.9f, 2 },
		.rot = qt_axis_angle(v3_right(), M_PI * .15f),
		.scale = .25f,
		.piv = { 0.f, 1.f },
		.off = { 0.f, 0.f },
		.just = JUST_LEFT,
		.col = v4_one(),
		.spacing = LINE_HEIGHT,
		.col_lim = 8,
		.cursor = '_',
	};
*/
#endif
	float asp = (float)frame.size.w / frame.size.h;
	m4 view = m4_view(cam_pos, cam_rot);
	m4 proj = m4_persp(60, asp, .001f, 1024);
	m4 vp = m4_mul(proj, view);

	return (struct txt_share) {
		.vp = vp,
		.screen = (v2) { frame.size.w, frame.size.h },
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
	};

	txtquad_init(cfg);
	txtquad_start();
	exit(0);
}
