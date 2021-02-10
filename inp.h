#ifndef INP_H
#define INP_H

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include "alg.h"

#include "txtquad.h"
/*
 * Public API
 */

#define KEY(K) GLFW_KEY_ ## K
#define KEY_DOWN(K) (inp_data.key.states[KEY(K)] == 1)
#define KEY_UP(K)   (inp_data.key.states[KEY(K)] == 2)
#define KEY_HELD(K) (inp_data.key.states[KEY(K)] == 3)

#define BTN(B) GLFW_MOUSE_BUTTON_ ## B
#define BTN_DOWN(B) (inp_data.btn.states[BTN(B)] == 1)
#define BTN_UP(B)   (inp_data.btn.states[BTN(B)] == 2)
#define BTN_HELD(B) (inp_data.btn.states[BTN(B)] == 3)

#define MOUSE_POS (inp_data.mouse.pos)
#define MOUSE_D   (inp_data.mouse.delta)

#define PAD(B) GLFW_GAMEPAD_BUTTON_ ## B
#define PAD_DOWN(B) (inp_data.pad.states[PAD(B)] == 1)
#define PAD_UP(B)   (inp_data.pad.states[PAD(B)] == 2)
#define PAD_HELD(B) (inp_data.pad.states[PAD(B)] == 3)

#define JOY_STICK_L (inp_data.joy.stick_l)
#define JOY_STICK_R (inp_data.joy.stick_r)
#define JOY_TRIGG_L (inp_data.joy.trigg_l)
#define JOY_TRIGG_R (inp_data.joy.trigg_r)

#define INP_DATA_STRUCT(T)                             \
struct {                                               \
	int *handles;                                  \
	size_t count;                                  \
	unsigned char states[GLFW_ ## T ## _LAST + 1]; \
}

extern struct Input {
	struct {
		v2 pos;
		v2 delta;
	} mouse;

	struct {
		   v2 stick_l;
		   v2 stick_r;
		float trigg_l;
		float trigg_r;
	} joy;

	INP_DATA_STRUCT(KEY) key;
	INP_DATA_STRUCT(MOUSE_BUTTON) but;
	INP_DATA_STRUCT(GAMEPAD_BUTTON) pad;
} inp_data;
#undef INP_DATA_STRUCT

/*
 * Implemented by consumer, called by engine
 */
__attribute__((weak))
void inp_ev_text(unsigned int unicode);
__attribute__((weak))
void inp_ev_mouse(float x, float y, struct Extent win_size);

/*
 * Implemented by engine, called by consumer
 */
void inp_init(
	int *key_handles,
	size_t key_count,
	int *but_handles,
	size_t but_count,
	int *pad_handles,
	size_t pad_count
);

/*
 * Implemented by library
 */
void inp_update(GLFWwindow *win);

#endif
