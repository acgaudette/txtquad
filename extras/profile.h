#ifdef QUAD_MARK_COUNT

#include <string.h>
#include "acg/sys.h"

#define PROFILE_MARK(TXT) \
	profile_add(profile_hash(__func__), TXT)

extern struct quad_profile {
	const char *istr[QUAD_MARK_COUNT];
	u16 counters[QUAD_MARK_COUNT];
	u16 total;
} quad_profile;

static int profile_hash(const char *str)
{
	for (int i = 0; i < QUAD_MARK_COUNT; ++i) {
		const char **istr = quad_profile.istr + i;
		if (!*istr) *istr = str;
		if (*istr == str) return i;
	}

	panic_msg("out of profile counter space");
	return -1;
}

#ifdef TXT_DEBUG
#define SPACER "........................"
static void profile_log(struct txt_buf *txt)
{
	printf(SPACER " quad profile report " SPACER "\n");

	for (u16 i = 0; i < QUAD_MARK_COUNT; ++i) {
		const char *istr = *(quad_profile.istr + i);
		if (!istr) break;

		const u16 count = quad_profile.counters[i];
		const float pc = 100.f
			* count / (float)txt->count;
		const float pressure = 100.f
			* count / (float)MAX_QUAD;

		printf(
			"%2u.%16s%16u%16.2f%%%16.2f%%\n",
			i,
			istr,
			count,
			pc,
			pressure
		);
	}

	const float pressure = 100.f
		* quad_profile.total / (float)MAX_QUAD;
	printf(
		"   %16s%16u%16.2f%%%16.2f%%\n",
		"",
		quad_profile.total,
		100.f,
		pressure
	);

	printf(SPACER " quad profile report " SPACER "\n");
}
#endif

#else
#define PROFILE_MARK(_) ;
#endif

static void profile_add(int e, struct txt_buf *txt)
{
#ifdef QUAD_MARK_COUNT
	assert(e < QUAD_MARK_COUNT);
	++quad_profile.counters[e];
	++quad_profile.total;
/*
	if (quad_profile.total == txt->count) return;
	printf("quad_count=%u/%zu\n", quad_profile.total, txt->count);
	panic_msg("profiler count mismatch in profile_add()");
*/
#endif
}

static void profile_report(struct txt_frame frame, struct txt_buf *txt)
{
#ifdef QUAD_MARK_COUNT
	if (quad_profile.total != txt->count) {
		printf("quad_count=%u/%zu\n", quad_profile.total, txt->count);
		panic_msg("profiler count mismatch in profile_report()");
	}
#ifdef TXT_DEBUG
	if (frame.acc == 0.f) profile_log(txt);
#endif
	memset(&quad_profile.counters, 0, sizeof(u16) * (QUAD_MARK_COUNT + 1));
#endif
}
