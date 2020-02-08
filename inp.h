#ifndef INP_H
#define INP_H

#include <GLFW/glfw3.h>

extern struct Input {
	int *handles;
	size_t count;
	unsigned char states[GLFW_KEY_LAST + 1];
} inp_data;

/*
 * Implemented by consumer, called by engine
 */
void inp_ev_text(unsigned int unicode);

/*
 * Implemented by engine, called by consumer
 */
void inp_init(int *handles, size_t count);

/*
 * Implemented by library
 */
void inp_update(GLFWwindow *win);

#define KEY(K) GLFW_KEY_ ## K
#define KEY_DOWN(K) (inp_data.states[KEY(K)] == 1)
#define KEY_UP(K)   (inp_data.states[KEY(K)] == 2)
#define KEY_HELD(K) (inp_data.states[KEY(K)] == 3)

#endif
