#define INP_SIZEOF(ID) (sizeof(ID) / sizeof(int))

struct inp_group {
	int equiv[4];
	u8 held : 1,
	     up : 1,
	     dn : 1;
};

static void inp_group_update(struct inp_group *buf, size_t len)
{
	for (size_t i = 0; i < len; ++i) {
		/* ... */
	}
}
