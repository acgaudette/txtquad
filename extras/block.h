#include <assert.h>

struct block {
	const char *str;

	float scale;
	v3 pos;
	v4 rot;
	v2 anch;

	#define JUST_LEFT   (-1.f)
	#define JUST_CENTER ( 0.f)
	#define JUST_RIGHT  ( 1.f)
	float justify;

	float spacing;
	float line_height;
};

struct block_ctx {
	const struct block block;
	const float nlspace;
	const v2 offset;
	const v2 extent;
	const char *ptr, *endl;
	float lines;
	float h;
};

static struct block_ctx block_prepare(struct block block)
{
	const float nlspace = block.line_height * LINE_HEIGHT;

	/* Compute bounds and offset */

	v2 extent = v2_zero();
	float ex = 0.f;

	for (const char *v = block.str; *v; ++v) {
		switch (*v) {
		case '\n':
			extent.y -= nlspace * block.spacing;
			ex = 0.f;
			continue;
		default:
			ex += block.spacing;
			extent.x = maxf(ex, extent.x);
		}
	}

	v2 anch = v2_add(
		(v2) { block.anch.x * .5f, block.anch.y * -.5f },
		(v2) { 0.5f, 0.5f }
	);

	--extent.y;
	v2 offset = v2_schur(extent, v2_mul(anch, -1.f));

	return (struct block_ctx) {
		.block = block,
		.nlspace = nlspace,
		.offset = offset,
		.extent = extent,
		.ptr = block.str,
		.endl = block.str,
	};
}

#ifdef DEBUG_UI
static void draw_marker(
	const v3 pos,
	const v4 rot,
	const float scale,
	const v3 col,
	struct txt_buf *txt
) {
	v3 offset = v3_mul(v3_fwd(), -1e-3);

	struct sprite result = {
		.anch = v2_zero(),
		.scale = maxf(.005f, scale * .15f),
		.pos = v3_add(pos, qt_app(rot, offset)),
		.rot = rot,
		.col = col,
		.vfx = v3_right(),
		.asc = 1,
	};

	size_t char_count = txt->count++;
	assert(char_count < MAX_QUAD);
	txt->quads[char_count] = sprite_conv(result);
}
#endif

static int block_draw(
	struct sprite    *out,
	struct block_ctx *ctx,
	struct txt_buf   *txt
) {
	assert(out);

	struct sprite result = {
		.anch = v2_zero(),
		.scale = ctx->block.scale,
		.rot = ctx->block.rot,
		.bounds = BOUNDS_FONT,

		/* Finalized by user after iter return */

		.col = v3_one(),
		.vfx = v3_right(),
	};

	const float scale = ctx->block.scale;
	const float spacing = ctx->block.spacing;

	/* Render */

	while ('\n' == *ctx->ptr) {
		ctx->endl = ctx->ptr + 1;
		ctx->h -= ctx->nlspace * spacing;
		++ctx->lines;
		++ctx->ptr;
	}

	float dist;
	{
		dist = ctx->ptr - ctx->endl;
		float remf = 0.f;

		float just = clamp01f(ctx->block.justify * .5f + .5f);
		if (just > 0.f) {
			for (const char *j = ctx->ptr; *j && '\n' != *j; ++j)
				++remf;
			--remf;
		}

		dist = dist * spacing;
		remf = remf * spacing;

		dist = lerpf(
			dist,
			(ctx->extent.x - spacing) - remf,
			just
		);
	}

	v3 pos = {
		ctx->offset.x + dist,
		ctx->offset.y + ctx->h,
		0.f,
	};

	const v3 correct = {
		// Correct for center of rotation
		 .5f - .5f * PIX_WIDTH,
		-.5f,

		// Correct for packing
		1e-4 * (ctx->lines + ((ctx->ptr - ctx->endl) % 2)),
	};

	v3_addeq(&pos, correct);
	pos = v3_mul(pos, scale);

	result.pos = v3_add(ctx->block.pos, qt_app(ctx->block.rot, pos)),
	result.asc = *ctx->ptr++,
	*out = result;
	if (result.asc) return 1;

#ifdef DEBUG_UI
	v3 offset3 = { ctx->offset.x * scale, ctx->offset.y * scale, 0.f };
	v3 extent3 = { ctx->extent.x * scale, ctx->extent.y * scale, 0.f };

	draw_marker(
		v3_add(ctx->block.pos, qt_app(ctx->block.rot, offset3)),
		ctx->block.rot,
		ctx->block.scale,
		(v3) { 1.f, .2f, .2f },
		txt
	);

	draw_marker(
		v3_add(
			ctx->block.pos,
			qt_app(ctx->block.rot, v3_add(offset3, extent3))
		),
		ctx->block.rot,
		ctx->block.scale,
		(v3) { .2f, .2f, 1.f },
		txt
	);

	draw_marker(
		ctx->block.pos,
		ctx->block.rot,
		ctx->block.scale,
		(v3) { .2f, 1.f, .2f },
		txt
	);
#endif
	return 0;
}
