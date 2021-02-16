#include <assert.h>

struct sprite {
	v2 anch; // Ranges from 0 => +/-1 from center
	float scale;
	v3 pos;
	v4 rot;
	v3 col;
	v3 vfx;
	char asc;
	int font;
};

static struct txt_quad sprite_conv(struct sprite spr)
{
	// Center, scale
	spr.anch = v2_add(
		v2_mul(
			spr.anch,
			.5f
		),
		(v2) { 0.5f, 0.5f }
	);

	float off_y = -spr.anch.y;
	float off_x =  spr.font ? // Correct 7px font
		-lerpf(spr.anch.x, spr.anch.x - PIX_WIDTH, spr.anch.x) :
		-spr.anch.x;

	off_x *= spr.scale;
	off_y *= spr.scale;

	// Anchor
	v3 pos = v3_add(
		spr.pos,
		qt_app(spr.rot, (v3) { off_x, off_y, 0.f })
	);

	// Distribute color and fx
	v4 col = { spr.col.x, spr.col.y, spr.col.z, spr.vfx.x };
	v2 vfx = { spr.vfx.y, spr.vfx.z };

	struct txt_quad result = {
		.value = spr.asc,
		.model = m4_model(pos, spr.rot, spr.scale),
		col,
		vfx,
	};

	return result;
}

static void quad_draw_imm(struct txt_quad quad, struct txt_buf *txt)
{
	size_t count = txt->count++;
	assert(count < MAX_QUAD);
	*(txt->quads + count) = quad;
}

static void sprite_draw_imm(struct sprite spr, struct txt_buf *txt)
{
	size_t count = txt->count++;
	assert(count < MAX_QUAD);
	*(txt->quads + count) = sprite_conv(spr);
}
