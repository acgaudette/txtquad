#include <assert.h>

#define BOUNDS_FONT ((v4) { 0.f, 0.f, PIX_WIDTH, 0.f }) // Correct 7px font

struct sprite {
	v2 anch; // Ranges from 0 => +/-1 from center
	float scale; v3 pos; v4 rot;
	v3 col;
	v3 vfx;
	char asc;
	v4 bounds; // Bottom left origin; V4_ZERO contains whole sprite
};

static struct txt_quad sprite_conv(struct sprite in)
{
	// Center, scale
	in.anch = v2_add(
		v2_mul(
			in.anch,
			.5f
		),
		(v2) { 0.5f, 0.5f }
	);

	float off_y = -lerpf(
		in.anch.y + in.bounds.y,
		in.anch.y - in.bounds.w,
		in.anch.y
	);

	float off_x = -lerpf(
		in.anch.x + in.bounds.x,
		in.anch.x - in.bounds.z,
		in.anch.x
	);

	off_x *= in.scale;
	off_y *= in.scale;

	// Anchor
	v3 pos = v3_add(
		in.pos,
		qt_app(in.rot, (v3) { off_x, off_y, 0.f })
	);

	// Distribute color and fx
	v4 col = { in.col.x, in.col.y, in.col.z, in.vfx.x };
	v2 vfx = { in.vfx.y, in.vfx.z };

	struct txt_quad result = {
		.value = in.asc,
		.model = m4_model(pos, in.rot, in.scale),
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

static void sprite_draw_imm(struct sprite in, struct txt_buf *txt)
{
	size_t count = txt->count++;
	assert(count < MAX_QUAD);
	*(txt->quads + count) = sprite_conv(in);
}
