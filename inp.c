#ifdef INP_KEYS
#include "inp.h"

void inp_update(GLFWwindow *win)
{
	for (size_t i = 0; i < inp_data.key.count; ++i) {
		int handle = inp_data.key.handles[i];
		char s = inp_data.key.states[handle];
		s = (s << 1) & 2;
		s |= glfwGetKey(win, handle) == GLFW_PRESS;
		inp_data.key.states[handle] = s;
	}

	for (size_t i = 0; i < inp_data.but.count; ++i) {
		int handle = inp_data.but.handles[i];
		char s = inp_data.but.states[handle];
		s = (s << 1) & 2;
		s |= glfwGetMouseButton(win, handle) == GLFW_PRESS;
		inp_data.but.states[handle] = s;
	}
}

#endif
