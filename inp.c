#include "inp.h"

void inp_update(GLFWwindow *win)
{
	for (size_t i = 0; i < inp_data.count; ++i) {
		int handle = inp_data.handles[i];
		char s = inp_data.states[handle];
		s = (s << 1) & 2;
		s |= glfwGetKey(win, handle) == GLFW_PRESS;
		inp_data.states[handle] = s;
	}
}
