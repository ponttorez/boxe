/**************************************************************************/
/*  test_window.cpp                                                       */
/**************************************************************************/
/*                         This file is part of:                          */
/*                             GODOT ENGINE                               */
/*                        https://godotengine.org                         */
/**************************************************************************/
/* Copyright (c) 2014-present Godot Engine contributors (see AUTHORS.md). */
/* Copyright (c) 2007-2014 Juan Linietsky, Ariel Manzur.                  */
/*                                                                        */
/* Permission is hereby granted, free of charge, to any person obtaining  */
/* a copy of this software and associated documentation files (the        */
/* "Software"), to deal in the Software without restriction, including    */
/* without limitation the rights to use, copy, modify, merge, publish,    */
/* distribute, sublicense, and/or sell copies of the Software, and to     */
/* permit persons to whom the Software is furnished to do so, subject to  */
/* the following conditions:                                              */
/*                                                                        */
/* The above copyright notice and this permission notice shall be         */
/* included in all copies or substantial portions of the Software.        */
/*                                                                        */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,        */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF     */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. */
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY   */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,   */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE      */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                 */
/**************************************************************************/

#include "tests/test_macros.h"

TEST_FORCE_LINK(test_window)

#include "core/input/input_map.h" // IWYU pragma: keep // Used by `SEND_GUI_MOUSE_MOTION_EVENT` macro.
#include "scene/gui/control.h"
#include "scene/main/scene_tree.h"
#include "scene/main/window.h"
#include "tests/display_server_mock.h"

namespace TestWindow {

class NotificationControlWindow : public Control {
	GDCLASS(NotificationControlWindow, Control);

protected:
	void _notification(int p_what) {
		switch (p_what) {
			case NOTIFICATION_MOUSE_ENTER: {
				mouse_over = true;
			} break;

			case NOTIFICATION_MOUSE_EXIT: {
				mouse_over = false;
			} break;
		}
	}

public:
	bool mouse_over = false;
};

TEST_CASE("[SceneTree][Window]") {
	Window *root = SceneTree::get_singleton()->get_root();

	SUBCASE("Control-mouse-over within Window-black bars should not happen") {
		Window *w = memnew(Window);
		root->add_child(w);
		w->set_size(Size2i(400, 200));
		w->set_position(Size2i(0, 0));
		w->set_content_scale_size(Size2i(200, 200));
		w->set_content_scale_mode(Window::CONTENT_SCALE_MODE_CANVAS_ITEMS);
		w->set_content_scale_aspect(Window::CONTENT_SCALE_ASPECT_KEEP);
		NotificationControlWindow *c = memnew(NotificationControlWindow);
		w->add_child(c);
		c->set_size(Size2i(100, 100));
		c->set_position(Size2i(-50, -50));

		CHECK_FALSE(c->mouse_over);
		SEND_GUI_MOUSE_MOTION_EVENT(Point2i(110, 10), MouseButtonMask::NONE, Key::NONE);
		CHECK(c->mouse_over);
		SEND_GUI_MOUSE_MOTION_EVENT(Point2i(90, 10), MouseButtonMask::NONE, Key::NONE);
		CHECK_FALSE(c->mouse_over); // GH-80011

		/* TODO:
		SEND_GUI_MOUSE_BUTTON_EVENT(Point2i(90, 10), MouseButton::LEFT, MouseButtonMask::LEFT, Key::NONE);
		SEND_GUI_MOUSE_BUTTON_RELEASED_EVENT(Point2i(90, 10), MouseButton::LEFT, MouseButtonMask::NONE, Key::NONE);
		CHECK(Control was not pressed);
		*/

		memdelete(c);
		memdelete(w);
	}
}

TEST_CASE("[SceneTree][Window] Appearing windows take input where they're drawn") {
	Window *root = SceneTree::get_singleton()->get_root();
	Window *window = memnew(Window);
	window->set_visible(false);
	window->set_flag(Window::FLAG_BORDERLESS, true);
	NotificationControlWindow *control = memnew(NotificationControlWindow);
	control->set_size(Size2(100, 100));
	window->add_child(control);
	root->add_child(window);
	window->add_theme_constant_override("appear_duration", 100000);
	window->add_theme_constant_override("appear_offset", 100);
	window->set_position(Point2i(100, 200));
	window->set_size(Size2i(100, 100));
	window->show();
	CHECK(window->get_position() == Point2i(100, 200));

	// At the start of the animation, the window is drawn 100 px above its position.
	SEND_GUI_MOUSE_MOTION_EVENT(Point2(150, 150), MouseButtonMask::NONE, Key::NONE);
	CHECK(control->mouse_over);
	SEND_GUI_MOUSE_MOTION_EVENT(Point2(150, 250), MouseButtonMask::NONE, Key::NONE);
	CHECK_FALSE(control->mouse_over);

	// Turning the animation off midway puts the window in place at the next frame.
	window->add_theme_constant_override("appear_duration", 0);
	SceneTree::get_singleton()->emit_signal(SNAME("process_frame"));
	SEND_GUI_MOUSE_MOTION_EVENT(Point2(150, 250), MouseButtonMask::NONE, Key::NONE);
	CHECK(control->mouse_over);

	memdelete(window);
}

} // namespace TestWindow
