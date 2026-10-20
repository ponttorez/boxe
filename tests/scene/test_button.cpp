/**************************************************************************/
/*  test_button.cpp                                                       */
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

TEST_FORCE_LINK(test_button)

#include "scene/2d/node_2d.h"
#include "scene/gui/button.h"
#include "scene/main/scene_tree.h"
#include "scene/main/window.h"
#include "tests/display_server_mock.h"
#include "tests/signal_watcher.h"

namespace TestButton {

TEST_CASE("[SceneTree][Button] is_hovered()") {
	// Create new button instance.
	Button *button = memnew(Button);
	CHECK(button != nullptr);
	Window *root = SceneTree::get_singleton()->get_root();
	root->add_child(button);

	// Set up button's size and position.
	button->set_size(Size2i(50, 50));
	button->set_position(Size2i(10, 10));

	// Button should initially be not hovered.
	CHECK(button->is_hovered() == false);

	// Simulate mouse entering the button.
	SEND_GUI_MOUSE_MOTION_EVENT(Point2i(25, 25), MouseButtonMask::NONE, Key::NONE);
	CHECK(button->is_hovered() == true);

	// Simulate mouse exiting the button.
	SEND_GUI_MOUSE_MOTION_EVENT(Point2i(150, 150), MouseButtonMask::NONE, Key::NONE);
	CHECK(button->is_hovered() == false);

	memdelete(button);
}

TEST_CASE("[SceneTree][Button] Press canceled by interactable") {
	Button *button = memnew(Button);
	button->set_focus_mode(Control::FOCUS_NONE);
	SceneTree::get_singleton()->get_root()->add_child(button);
	button->set_size(Size2i(50, 50));
	button->set_position(Size2i(10, 10));

	SEND_GUI_MOUSE_BUTTON_EVENT(Point2i(25, 25), MouseButton::LEFT, MouseButtonMask::LEFT, Key::NONE);
	CHECK(button->get_draw_mode() == BaseButton::DRAW_PRESSED);

	// The press can't be released anymore, the button doesn't receive input.
	button->set_interactable(false);
	CHECK(button->get_draw_mode() == BaseButton::DRAW_NORMAL);
	// Only hovered again, as the mouse is still over it.
	button->set_interactable(true);
	CHECK(button->get_draw_mode() == BaseButton::DRAW_HOVER);

	memdelete(button);
}

TEST_CASE("[SceneTree][Button] Touch canceled by interactable") {
	Button *button = memnew(Button);
	button->set_focus_mode(Control::FOCUS_NONE);
	SceneTree::get_singleton()->get_root()->add_child(button);
	button->set_size(Size2i(50, 50));
	button->set_position(Size2i(10, 10));
	SIGNAL_WATCH(button, SNAME("pressed"));

	SEND_GUI_TOUCH_EVENT(Point2i(25, 25), true, false);
	button->set_interactable(false);
	SEND_GUI_TOUCH_EVENT(Point2i(25, 25), false, false);
	button->set_interactable(true);
	SIGNAL_CHECK_FALSE(SNAME("pressed"));

	// The touch that was canceled doesn't block the next one.
	SEND_GUI_TOUCH_EVENT(Point2i(25, 25), true, false);
	SEND_GUI_TOUCH_EVENT(Point2i(25, 25), false, false);
	SIGNAL_CHECK(SNAME("pressed"), { {} });

	SIGNAL_UNWATCH(button, SNAME("pressed"));
	memdelete(button);
}

TEST_CASE("[SceneTree][Button] Interactable through other nodes") {
	Control *parent = memnew(Control);
	Node2D *node_2d = memnew(Node2D);
	Button *button = memnew(Button);
	parent->add_child(node_2d);
	node_2d->add_child(button);
	SceneTree::get_singleton()->get_root()->add_child(parent);

	parent->set_interactable(false);
	CHECK_FALSE(button->is_interactable_in_tree());

	// Moving the node in between to an interactable control unlocks the button.
	Control *other = memnew(Control);
	SceneTree::get_singleton()->get_root()->add_child(other);
	node_2d->reparent(other);
	CHECK(button->is_interactable_in_tree());
	other->set_interactable(false);
	CHECK_FALSE(button->is_interactable_in_tree());

	memdelete(parent);
	memdelete(other);
}

} // namespace TestButton
