#ifndef INP_H
#define INP_H

#include <GLFW/glfw3.h>

#define INP_DATA_STRUCT(T)                             \
struct {                                               \
	int *handles;                                  \
	size_t count;                                  \
	unsigned char states[GLFW_ ## T ## _LAST + 1]; \
}

extern struct Input {
	INP_DATA_STRUCT(KEY) key;
	INP_DATA_STRUCT(MOUSE_BUTTON) but;
} inp_data;
#undef INP_DATA_STRUCT

/*
 * Implemented by consumer, called by engine
 */
void inp_ev_text(unsigned int unicode);
void inp_ev_mouse(float x, float y);

/*
 * Implemented by engine, called by consumer
 */
void inp_init(
	int *key_handles,
	size_t key_count,
	int *but_handles,
	size_t but_count
);

/*
 * Implemented by library
 */
void inp_update(GLFWwindow *win);

#define KEY(K) GLFW_KEY_ ## K
#define KEY_DOWN(K) (inp_data.key.states[KEY(K)] == 1)
#define KEY_UP(K)   (inp_data.key.states[KEY(K)] == 2)
#define KEY_HELD(K) (inp_data.key.states[KEY(K)] == 3)

#define BUT(B) GLFW_MOUSE_BUTTON_ ## B
#define BUT_DOWN(B) (inp_data.but.states[BUT(B)] == 1)
#define BUT_UP(B)   (inp_data.but.states[BUT(B)] == 2)
#define BUT_HELD(B) (inp_data.but.states[BUT(B)] == 3)

#endif
