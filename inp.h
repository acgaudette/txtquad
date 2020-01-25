#ifndef INP_H
#define INP_H

#include <GLFW/glfw3.h>

extern struct Input {
	int *handles;
	size_t count;
	unsigned char states[GLFW_KEY_LAST + 1];
} inp_data;

void inp_ev_text(unsigned int unicode);
void inp_init(int *handles, size_t count);

#define KEY(K) GLFW_KEY_ ## K
#define KEY_DOWN(K) (inp_data.states[KEY(K)] == 1)
#define KEY_UP(K)   (inp_data.states[KEY(K)] == 2)
#define KEY_HELD(K) (inp_data.states[KEY(K)] == 3)

static void inp_update(GLFWwindow *win)
{
	for (size_t i = 0; i < inp_data.count; ++i) {
		int handle = inp_data.handles[i];
		char s = inp_data.states[handle];
		s = (s << 1) & 2;
		s |= glfwGetKey(win, handle) == GLFW_PRESS;
		inp_data.states[handle] = s;
	}
}

#endif
