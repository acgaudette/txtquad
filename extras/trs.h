typedef struct {
	float scale;
	v3 pos;
	v4 rot;
} t3;

typedef struct {
	v2 pos;
	float scale;
	float angle;
} t2;

static v3 t3_app(t3 t, v3 v)
{
	return v3_add(t.pos, qt_app(t.rot, v));
}
