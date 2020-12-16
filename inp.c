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

	/* Mouse delta */

	double mx, my;
	glfwGetCursorPos(win, &mx, &my);

	v2 pos = { mx, my * -1.f };
	inp_data.mouse.pos = pos;

	static v2 mouse_last;
	static float dirty;

	mouse_last = v2_lerp(inp_data.mouse.pos, mouse_last, dirty);
	dirty = 1.f;

	inp_data.mouse.delta = v2_sub(pos, mouse_last);
	mouse_last = pos;

	/* Joy */

	if (!glfwJoystickIsGamepad(GLFW_JOYSTICK_1)) return;

	GLFWgamepadstate pad;
	if (!glfwGetGamepadState(GLFW_JOYSTICK_1, &pad)) return;

	for (size_t i = 0; i < inp_data.pad.count; ++i) {
		int handle = inp_data.pad.handles[i];
		char s = inp_data.pad.states[handle];
		s = (s << 1) & 2;
		s |= pad.buttons[handle] == GLFW_PRESS;
		inp_data.pad.states[handle] = s;
	}

	inp_data.joy.stick_l = (v2) {
		 pad.axes[GLFW_GAMEPAD_AXIS_LEFT_X],
		-pad.axes[GLFW_GAMEPAD_AXIS_LEFT_Y],
	};

	inp_data.joy.stick_r = (v2) {
		 pad.axes[GLFW_GAMEPAD_AXIS_RIGHT_X],
		-pad.axes[GLFW_GAMEPAD_AXIS_RIGHT_Y],
	};

	inp_data.joy.trigg_l = .5f + .5f
		* pad.axes[GLFW_GAMEPAD_AXIS_LEFT_TRIGGER];
	inp_data.joy.trigg_r = .5f + .5f
		* pad.axes[GLFW_GAMEPAD_AXIS_RIGHT_TRIGGER];
}

#endif
