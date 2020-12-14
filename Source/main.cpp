#include "Renderer.hpp"
#include "Context.hpp"

#include <GLFW/glfw3.h>

int main() {
	auto ctxt = Context::create();

	auto renderer = std::make_optional<Renderer>();
	renderer->init(*ctxt);

	glfwSetWindowUserPointer(ctxt->window, &*renderer);
	glfwSetInputMode(ctxt->window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	glfwSetCursorPosCallback(ctxt->window, [](GLFWwindow* window, f64 x_pos, f64 y_pos) {
		Renderer* r = (Renderer*)glfwGetWindowUserPointer(window);
		r->mouse_event(x_pos, y_pos);
	});

	glfwSetKeyCallback(ctxt->window, [](GLFWwindow* window, i32 key, i32 scancode, i32 action, i32 mods) {
		if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
			glfwSetWindowShouldClose(window, GLFW_TRUE);
		}
	});

	while (!glfwWindowShouldClose(ctxt->window)) {
		glfwPollEvents();

		renderer->update();
		renderer->render();
	}

	renderer.reset();
	Context::cleanup(ctxt);
}
